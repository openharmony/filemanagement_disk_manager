/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "block_info_table.h"
#include "uevent_bootstrap.h"

#include "adapter/storage_daemon_adapter.h"
#include "disk.h"
#include "disk_manager.h"
#include "partition_table_parser.h"
#include "storage_spec_models.h"
#include "uevent_env_parser.h"
#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "errors.h"
#include "notification/common_event_publisher.h"
#include "volume_core.h"

#include <cctype>
#include <dirent.h>
#include <fstream>
#include <string_view>
#include <sys/stat.h>
#include <sys/sysmacros.h>

namespace OHOS {
namespace DiskManager {
std::list<DiskConfig> UeventBootstrap::diskConfigList_;
std::mutex UeventBootstrap::diskConfigListMutex_;

namespace {

constexpr const char *DEV_BLOCK = "/dev/block/";
constexpr bool AUTO_MOUNT_EXTERNAL_VOLUMES = true;
constexpr uint32_t NODE_PERM = 0660u;
constexpr uint32_t K_DISK_BLOCK_DEVICE_NODE_MODE = NODE_PERM | static_cast<uint32_t>(S_IFBLK);
constexpr uint32_t K_VOLUME_BLOCK_DEVICE_NODE_MODE = static_cast<uint32_t>(S_IFBLK);
constexpr int DISK_MMC_MAJOR = 179;
constexpr int DISK_CD_MAJOR = 11;
constexpr int32_t MIN_LINES = 32;
constexpr int32_t MAJORID_BLKEXT = 259;
constexpr int32_t MAX_PARTITION = 16;
constexpr int32_t MAX_INTERVAL_PARTITION = 15;
constexpr int32_t VOL_LENGTH = 3;
const int32_t CONFIG_PARAM_NUM = 6;
#ifdef CDC_STORAGE
// it will be decoupled to the car odm
const std::string CONFIG_PTAH = "/system/etc/disk_manager/disk_config";
#else
const std::string CONFIG_PTAH = "/system/etc/disk_manager/disk_config";
#endif
constexpr const char *BLOCK_PATH = "/dev/block";

enum class CdromState {
    NO_DISC,
    NON_EMPTY_DISC,
    EMPTY_DISC,
};

CdromState QueryCdromState(const std::string &devPath)
{
    int32_t status = 0;
    int32_t ret = StorageDaemonAdapter::GetInstance().QueryCDStatus(devPath, status);
    if (ret != ERR_OK) {
        LOGE("QueryCdromState QueryCDStatus failed ret=%{public}d", ret);
        return CdromState::NO_DISC;
    }
    if ((status & 0x01) == 0) {
        return CdromState::NO_DISC;
    }
    if ((status & 0x02) == 0) {
        return CdromState::NON_EMPTY_DISC;
    }
    return CdromState::EMPTY_DISC;
}

constexpr uint32_t ActionHash(std::string_view sv)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < sv.size(); ++i) {
        h ^= static_cast<uint32_t>(static_cast<unsigned char>(sv[i]));
        h *= 16777619u;
    }
    return h;
}

inline uint32_t ActionHashRuntime(const std::string &action)
{
    return ActionHash(std::string_view(action.data(), action.size()));
}

std::string DiskIdFrom(unsigned int maj, unsigned int min)
{
    return std::string("disk-") + std::to_string(maj) + "-" + std::to_string(min);
}

std::string VolIdFromDev(dev_t d)
{
    return std::string("vol-") + std::to_string(major(d)) + "-" + std::to_string(minor(d));
}

std::string BlockPathForId(const std::string &id)
{
    return std::string(DEV_BLOCK) + id;
}

dev_t PartitionDev(unsigned int diskMaj, unsigned int diskMin, uint32_t partIndex)
{
    return makedev(diskMaj, diskMin + partIndex);
}

int32_t LegacyDiskFlagFromDevPath(const UeventEnv &env)
{
    if (env.major == DISK_CD_MAJOR) {
        return CD_FLAG;
    }
    if (env.devPath.find("usb") != std::string::npos) {
        return USB_FLAG;
    }
    return SD_FLAG;
}

int32_t ResolveInitialDiskFlag(const UeventEnv &env)
{
    uint32_t matchedFlag = UeventBootstrap::MatchConfig(env);
    if (matchedFlag != 0) {
        return static_cast<int32_t>(matchedFlag);
    }
    return LegacyDiskFlagFromDevPath(env);
}

void DestroyALLVolume(const std::string &diskId)
{
    LOGI("DestroyALLVolume::DestroyALLVolume enter");
    std::vector<VolumeExternal> vols;
    (void)DiskManager::GetInstance().GetAllVolumes(vols);
    for (const VolumeExternal &vol : vols) {
        if (vol.GetDiskId() != diskId) {
            continue;
        }
        int32_t unmountRet = DiskManager::GetInstance().Unmount(vol.GetId());
        if (unmountRet != E_OK) {
            LOGE("Unmount failed, volId=%{public}s, ret=%{public}d", vol.GetId().c_str(), unmountRet);
        }
        int32_t ret = StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(BlockPathForId(vol.GetId()));
        if (ret != E_OK) {
            LOGI("Destroy volume failed vol:%{public}s, ret:%{public}d", vol.GetId().c_str(), ret);
            continue;
        }
        CommonEventPublisher::PublishVolumeChange((vol.GetState() == UNMOUNTED) ? REMOVED : BAD_REMOVAL, vol);
        (void)DiskManager::GetInstance().OnVolumeDestroyed(vol.GetId());
    }
}

int32_t DestroyALLDisk(const std::string &diskId)
{
    LOGI("DestroyALLDisk::DestroyALLDisk enter");
    int32_t ret = StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(BlockPathForId(diskId));
    if (ret != E_OK) {
        LOGE("DestroyBlockDeviceNode failed, ret=%{public}d", ret);
        return DiskManagerErrNo::E_DESTROY_DEVICE_NODE;
    }

    Disk diskSnap;
    const bool hadDisk = DiskManager::GetInstance().GetDiskById(diskId, diskSnap) == DiskManagerErrNo::E_OK;
    (void)DiskManager::GetInstance().OnDiskDestroyed(diskId);
    if (hadDisk) {
        CommonEventPublisher::PublishDiskChange(DiskEventKind::REMOVED, diskSnap);
    }
    return DiskManagerErrNo::E_OK;
}

void LogUeventEnvForHandler(const char *handler, const UeventEnv &env)
{
    LOGI("%{public}s uevent: action=%{public}s subsystem=%{public}s devtype=%{public}s major=%{public}u "
         "minor=%{public}u devpath=%{public}s devname=%{public}s ejectRequest=%{public}d sysPath=%{public}s",
         handler, env.action.c_str(), env.subsystem.c_str(), env.devType.c_str(), env.major, env.minor,
         env.devPath.c_str(), env.devName.c_str(), static_cast<int>(env.ejectRequest), env.sysPath.c_str());
}

std::vector<std::string> SplitRawDumpToLines(const std::string &rawDump)
{
    std::vector<std::string> lines;
    if (rawDump.empty()) {
        return lines;
    }

    std::string::size_type start = 0;
    std::string::size_type end = rawDump.find('\n');

    while (end != std::string::npos) {
        if (start < end) {
            lines.push_back(rawDump.substr(start, end - start));
        }
        start = end + 1;
        end = rawDump.find('\n', start);
    }

    if (start < rawDump.size()) {
        lines.push_back(rawDump.substr(start));
    }

    for (auto &i : lines) {
        LOGI("SplitRawDumpToLines lines info:%{public}s", i.c_str());
    }
    return lines;
}

int32_t BuildAndSyncPartitions(const UeventEnv &env,
                               const std::string &diskId,
                               const std::string &diskDevPath,
                               std::vector<PartitionRecord> &parts,
                               bool &isUserData)
{
    int32_t err = StorageDaemonAdapter::GetInstance().CreateBlockDeviceNode(
        diskDevPath, K_DISK_BLOCK_DEVICE_NODE_MODE, static_cast<int32_t>(env.major), static_cast<int32_t>(env.minor));
    if (err != ERR_OK) {
        LOGE("CreateDiskBlockDeviceNode disk failed err=%{public}d", err);
        return err;
    }

    std::string rawDump;
    int32_t maxVolume = 0;
    err = StorageDaemonAdapter::GetInstance().ReadPartitionTable(diskDevPath, rawDump, maxVolume);
    if (err != ERR_OK) {
        LOGE("ReadPartitionTable failed err=%{public}d", err);
    }

    std::vector<std::string> lines = SplitRawDumpToLines(rawDump);
    if (lines.size() > MIN_LINES) {
        auto userdataIt = std::find_if(lines.begin(), lines.end(), [](const std::string &str) {
            return str.find("userdata") != std::string::npos;
        });
        if (userdataIt != lines.end()) {
            isUserData = true;
            LOGI("BuildAndSyncPartitions: detected userdata partition in disk=%{public}s lines=%{public}zu",
                 diskId.c_str(), lines.size());
            auto diskIt = std::find_if(lines.begin(), lines.end(), [](const std::string &str) {
                return str.find("DISK") != std::string::npos;
            });
            rawDump.clear();
            rawDump += (diskIt != lines.end() ? *diskIt : "") + "\n";
            rawDump += *userdataIt;
        }
    }
    if (!rawDump.empty()) {
        std::string tableType;
        (void)PartitionTableParser::ParseSgdiskDump(rawDump, diskId, tableType, parts);
    }
    (void)DiskManager::GetInstance().ReplacePartitionsForDisk(diskId, parts);
    return DiskManagerErrNo::E_OK;
}

void UpsertDiskAndPublishEvent(const UeventEnv &env, const std::string &diskId, bool publishNewDiskEvent)
{
    BlockInfo blockInfo {};
    const bool hasBlockInfo = BlockInfoTable::GetInstance().TryCopyByDiskId(diskId, blockInfo);

    if (!publishNewDiskEvent) {
        return;
    }
    Disk diskForEvent(diskId, hasBlockInfo ? static_cast<int64_t>(blockInfo.sizeBytes) : 0, BlockPathForId(diskId),
                      ResolveInitialDiskFlag(env));
    if (hasBlockInfo) {
        diskForEvent.SetExtraInfo(BlockInfoTable::ToJsonStringWithExtras(
            blockInfo, {{"diskId", diskId}, {"diskName", env.devName}}));
    } else {
        LOGW("UpsertDiskAndPublishEvent block info cache miss diskId=%{public}s", diskId.c_str());
    }
    diskForEvent.RefreshClassificationFromSysfs(env.sysPath);
    if (!diskForEvent.GetExtraInfo().empty()) {
        LOGI("UpsertDiskAndPublishEvent extraInfo len=%{public}zu diskId=%{public}s",
             diskForEvent.GetExtraInfo().size(),
             diskId.c_str());
    }
    CommonEventPublisher::PublishDiskChange(DiskEventKind::MOUNTED, diskForEvent);
    (void)DiskManager::GetInstance().OnDiskCreated(diskForEvent);
}

int32_t GetMaxMinor(int32_t major)
{
    LOGD("[L3:DiskInfo] GetMaxMinor: >>> ENTER <<< major=%{public}d", major);
    DIR* dir;
    struct dirent* entry;
    int32_t maxMinor = -1;
    if ((dir = opendir(BLOCK_PATH)) == nullptr) {
        LOGE("[L3:DiskInfo] GetMaxMinor: <<< EXIT FAILED <<< open=%{public}s failed, errno=%{public}d",
             BLOCK_PATH, errno);
        return maxMinor;
    }
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.' || strncmp(entry->d_name, "vol", VOL_LENGTH) != 0) {
            continue;
        }
        std::string devicePath = std::string(BLOCK_PATH) + "/" + entry->d_name;
        struct stat statbuf;
        if (stat(devicePath.c_str(), &statbuf) == 0) {
            int32_t majorNum = static_cast<int32_t>major(statbuf.st_rdev);
            int32_t minorNum = static_cast<int32_t>minor(statbuf.st_rdev);

            if (majorNum == major) {
                maxMinor = minorNum > maxMinor ? minorNum : maxMinor;
            }
        }
    }
    closedir(dir);
    LOGD("[L3:DiskInfo] GetMaxMinor: <<< EXIT SUCCESS <<< maxMinor=%{public}d", maxMinor);
    return maxMinor;
}

int32_t CreateAndSetupVolume(const std::string &diskId,
                             dev_t pDev,
                             const bool &isUserData,
                             int32_t partitionNUm)
{
    const std::string volId = VolIdFromDev(pDev);
    const std::string volDevPath = BlockPathForId(volId);

    int32_t err = StorageDaemonAdapter::GetInstance().CreateBlockDeviceNode(volDevPath,
                                                                            K_VOLUME_BLOCK_DEVICE_NODE_MODE,
                                                                            static_cast<int32_t>(major(pDev)),
                                                                            static_cast<int32_t>(minor(pDev)));
    if (err != ERR_OK) {
        LOGE("CreateVolumeBlockDeviceNode vol %{public}s err=%{public}d", volId.c_str(), err);
        return err;
    }
    VolumeExternal volExternal(VolumeCore(volId, EXTERNAL, diskId));
    volExternal.SetUserData(isUserData);
    volExternal.SetPartitionNum(partitionNUm);
    (void)DiskManager::GetInstance().OnVolumeCreated(volExternal);
    return ERR_OK;
}

void ReadAndUpdateMetadata(const std::string &volId, const std::string &volDevPath,
                           std::string &uuid, std::string &type, std::string &label)
{
    int32_t err = StorageDaemonAdapter::GetInstance().ReadMetadata(volDevPath, uuid, type, label);
    LOGI("UUID: %{public}s, Type: %{public}s, Label: %{public}s", uuid.c_str(), type.c_str(), label.c_str());
    if (err == ERR_OK) {
        (void)DiskManager::GetInstance().UpdateVolumeMetadata(volId, uuid, type, label);
    }
}

void DiscoverSinglePartitionVolume(const UeventEnv &env,
                                   const std::string &diskId,
                                   const PartitionRecord &p,
                                   const bool &isUserData)
{
    dev_t pDev = makedev(0, 0);
    if (isUserData) {
        int32_t maxMinor = GetMaxMinor(MAJORID_BLKEXT);
        if (maxMinor == -1) {
            pDev = makedev(MAJORID_BLKEXT, static_cast<uint32_t>(p.partitionNumber) - MAX_PARTITION);
        } else {
            pDev = makedev(MAJORID_BLKEXT, static_cast<uint32_t>(maxMinor) + static_cast<uint32_t>(p.partitionNumber) -
                                               MAX_INTERVAL_PARTITION);
        }
    } else {
        pDev = PartitionDev(env.major, env.minor, p.partitionNumber);
    }

    const std::string volId = VolIdFromDev(pDev);
    if (CreateAndSetupVolume(diskId, pDev, isUserData, static_cast<int32_t>(p.partitionNumber)) != ERR_OK) {
        return;
    }

    std::string uuid;
    std::string type;
    std::string label;
    const std::string volDevPath = BlockPathForId(volId);
    ReadAndUpdateMetadata(volId, volDevPath, uuid, type, label);
    LOGI("AUTO_MOUNT_EXTERNAL_VOLUMES: %{public}d, type.empty(): %{public}d, uuid.empty(): %{public}d",
         AUTO_MOUNT_EXTERNAL_VOLUMES, type.empty(), uuid.empty());
    if (!AUTO_MOUNT_EXTERNAL_VOLUMES || type.empty() || uuid.empty()) {
        return;
    }

    int32_t err = DiskManager::GetInstance().Mount(volId);
    if (err != ERR_OK) {
        LOGE("DiscoverSinglePartitionVolume Mount failed volId=%{public}s", volId.c_str());
        return;
    }
    LOGI("DiscoverSinglePartitionVolume EXIT SUCCESS");
}

void HandleAddCD(const UeventEnv &env, const std::string &diskId, CdromState state)
{
    LOGI("HandleAddCD CD exists");
    dev_t pDev = PartitionDev(env.major, env.minor, 0);
    const std::string volId = VolIdFromDev(pDev);
    const std::string volDevPath = BlockPathForId(volId);

    if (CreateAndSetupVolume(diskId, pDev, false, 0) != ERR_OK) {
        return;
    }

    std::string uuid;
    std::string type;
    std::string label;
    if (state == CdromState::EMPTY_DISC) {
        uuid = " ";
        type = "udf";
        label = "DVD RW";
    } else {
        ReadAndUpdateMetadata(volId, volDevPath, uuid, type, label);
    }
    (void)DiskManager::GetInstance().UpdateVolumeMetadata(volId, uuid, type, label);

    if (type.empty()) {
        LOGE("HandleAddCD type.empty()=%{public}d, uuid.empty()=%{public}d", type.empty(), uuid.empty());
        return;
    }

    int32_t err = DiskManager::GetInstance().Mount(volId);
    if (err != ERR_OK) {
        LOGE("Mount failed volId=%{public}s", volId.c_str());
        return;
    }
    LOGI("HandleAddCD EXIT SUCCESS");
}

void DiscoverSinglePartitionVolume4CD(const UeventEnv &env, const std::string &diskId)
{
    LOGI("Diskid=%{public}s, ejectRequest=%{public}d", diskId.c_str(), env.ejectRequest);
    if (env.ejectRequest == true) {
        DestroyALLVolume(diskId);
        const int32_t ret = StorageDaemonAdapter::GetInstance().Eject(BlockPathForId(diskId));
        if (ret != ERR_OK) {
            LOGE("Eject err=%{public}d", ret);
            return;
        }
        LOGI("Disk ejected");
        return;
    }

    CdromState state = QueryCdromState(BlockPathForId(DiskIdFrom(env.major, env.minor)));
    if (state == CdromState::NO_DISC) {
        DestroyALLVolume(diskId);
        LOGI("CD not exist, cleared");
        return;
    }
    HandleAddCD(env, diskId, state);
}
} // namespace

int32_t UeventBootstrap::OnBlockDiskUevent(const std::string &rawUeventMsg)
{
    LOGI("UeventBootstrap::OnBlockDiskUevent enter external=IDiskManager::OnBlockDiskUevent rawLen=%{public}zu",
         rawUeventMsg.size());

    UeventEnv env;
    if (!UeventEnvParser::Parse(rawUeventMsg, env)) {
        LOGE("OnBlockDiskUevent parse failed");
        return DiskManagerErrNo::E_UEVENT_PARSE_FAILED;
    }
    if (!env.IsBlockDiskEvent()) {
        LOGW("OnBlockDiskUevent skip non-block-disk action=%{public}s", env.action.c_str());
        return DiskManagerErrNo::E_OK;
    }

    LogUeventEnvForHandler("OnBlockDiskUevent", env);

    switch (ActionHashRuntime(env.action)) {
        case ActionHash("remove"):
            return HandleDiskRemove(env);
        case ActionHash("add"):
            return HandleDiskAdd(env);
        case ActionHash("change"):
            return HandleDiskChange(env);
        default:
            LOGI("OnBlockDiskUevent action=%{public}s ignored", env.action.c_str());
            return DiskManagerErrNo::E_OK;
    }
}

int32_t UeventBootstrap::HandleDiskRemove(const UeventEnv &env)
{
    LOGI("UeventBootstrap::HandleDiskRemove enter external=IDiskManager::OnBlockDiskUevent branch=remove");

    const std::string diskId = DiskIdFrom(env.major, env.minor);
    DestroyALLVolume(diskId);
    int32_t ret = DestroyALLDisk(diskId);
    if (ret != E_OK) {
        LOGE("HandleDiskRemove: DestroyALLDisk failed, err=%{public}d", ret);
        return ret;
    }
    return DiskManagerErrNo::E_OK;
}

int32_t UeventBootstrap::DiscoverPartitionsAndVolumes(const UeventEnv &env, bool publishNewDiskEvent)
{
    LOGI("UeventBootstrap::DiscoverPartitionsAndVolumes enter "
         "external=IDiskManager::OnBlockDiskUevent publishNewDisk=%{public}d",
         static_cast<int>(publishNewDiskEvent));

    const std::string diskId = DiskIdFrom(env.major, env.minor);
    const std::string diskDevPath = BlockPathForId(diskId);

    LOGI("ID:%{public}s, Path:%{public}s, action:%{public}s", diskId.c_str(), diskDevPath.c_str(), env.action.c_str());

    std::vector<PartitionRecord> parts;
    bool isUserData = false;
    int32_t err = BuildAndSyncPartitions(env, diskId, diskDevPath, parts, isUserData);
    if (err != ERR_OK) {
        LOGE("BuildAndSyncPartitions failed with error: %{public}d", err);
        return err;
    }
    LOGI("BuildAndSyncPartitions completed successfully");

    UpsertDiskAndPublishEvent(env, diskId, publishNewDiskEvent);
    LOGI("UpsertDiskAndPublishEvent completed for disk ID: %{public}s", diskId.c_str());
    if (env.major == DISK_CD_MAJOR) {
        DiscoverSinglePartitionVolume4CD(env, diskId);
        return DiskManagerErrNo::E_OK;
    }
    if (parts.empty() && !publishNewDiskEvent) {
        LOGI("publishNewDiskEvent is %{public}d, and no partion for disk ID: %{public}s", publishNewDiskEvent,
             diskId.c_str());
        DestroyALLVolume(diskId);
    }
    for (const auto &p : parts) {
        LOGI("Discovering volume for partition number: %{public}d", p.partitionNumber);
        DiscoverSinglePartitionVolume(env, diskId, p, isUserData);
    }

    return DiskManagerErrNo::E_OK;
}

int32_t UeventBootstrap::HandleDiskAdd(const UeventEnv &env)
{
    LOGI("UeventBootstrap::HandleDiskAdd enter external=IDiskManager::OnBlockDiskUevent branch=add");

    const std::string diskId = DiskIdFrom(env.major, env.minor);
    const bool publishNew = !DiskManager::GetInstance().HasDisk(diskId);
    return DiscoverPartitionsAndVolumes(env, publishNew);
}

int32_t UeventBootstrap::HandleDiskChange(const UeventEnv &env)
{
    LOGI("UeventBootstrap::HandleDiskChange enter external=IDiskManager::OnBlockDiskUevent branch=change");

    const std::string diskDevPath = BlockPathForId(DiskIdFrom(env.major, env.minor));
    const std::string diskId = DiskIdFrom(env.major, env.minor);
    const bool publishNew = !DiskManager::GetInstance().HasDisk(diskId);
    return DiscoverPartitionsAndVolumes(env, publishNew);
}

void UeventBootstrap::Init()
{
    LOGI("UeventBootstrap::Init enter");
    ParasConfig();
}

std::vector<std::string> UeventBootstrap::SplitLine(std::string &line, std::string &token)
{
    std::vector<std::string> result;
    std::string::size_type start;
    std::string::size_type end;

    start = 0;
    end = line.find(token);
    while (std::string::npos != end) {
        result.push_back(line.substr(start, end - start));
        start = end + token.size();
        end = line.find(token, start);
    }

    if (start != line.length()) {
        result.push_back(line.substr(start));
    }

    return result;
}

bool UeventBootstrap::ParasConfig()
{
    LOGI("UeventBootstrap::ParasConfig enter");
    std::ifstream infile;
    infile.open(CONFIG_PTAH);
    if (!infile) {
        LOGE("Cannot open config");
        return false;
    }

    while (infile) {
        std::string line;
        std::getline(infile, line);
        if (line.empty()) {
            LOGI("Param config complete");
            break;
        }

        std::string token = " ";
        auto split = SplitLine(line, token);
        if (split.size() != CONFIG_PARAM_NUM) {
            LOGE("Invalids config line: number of parameters is incorrect");
            continue;
        }

        auto it = split.begin();
        if (*it != "sysPattern") {
            LOGE("Invalids config line: no sysPattern");
            continue;
        }

        auto sysPattern = *(++it);
        if (*(++it) != "label") {
            LOGE("Invalids config line: no label");
            continue;
        }

        auto label = *(++it);
        if (*(++it) != "flag") {
            LOGE("Invalids config line: no flag");
            continue;
        }

        it++;
        int flag = std::atoi((*it).c_str());
        auto diskConfig = std::make_shared<DiskConfig>(sysPattern, label, flag);
        {
            std::lock_guard<std::mutex> lock(diskConfigListMutex_);
            diskConfigList_.push_back(*diskConfig);
        }
    }

    infile.close();
    return true;
}

uint32_t UeventBootstrap::MatchConfig(const UeventEnv &env)
{
    LOGI("DiskManager::MatchConfig enter");
    std::string devPath = env.devPath;
    unsigned int major = (unsigned int)env.major;
    uint32_t flag = 0;
    std::lock_guard<std::mutex> lock(diskConfigListMutex_);
    for (auto config : diskConfigList_) {
        if (config.IsMatch(devPath)) {
            LOGI("DiskManager::MatchConfig: devPath=%{public}s, matched", devPath.c_str());
            uint32_t flag = static_cast<uint32_t>(config.GetFlag());
            if (major == DISK_MMC_MAJOR) {
                flag |= SD_FLAG;
            } else if (major == DISK_CD_MAJOR) {
                flag |= CD_FLAG;
            } else {
                flag = ((flag != static_cast<uint32_t>(DVR_USB)) ? (flag | USB_FLAG) : flag);
            }
            return flag;
        }
    }

    LOGI("DiskManager::MatchConfig: No matching configuration found");
    return flag;
}
} // namespace DiskManager
} // namespace OHOS
