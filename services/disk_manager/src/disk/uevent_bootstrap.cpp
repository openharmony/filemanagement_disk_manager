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
#include "block_info.h"
#include "partition_types.h"

#include "storage_daemon_adapter.h"
#include "disk.h"
#include "disk_manager.h"
#include "partition_table_parser.h"
#include "storage_spec_models.h"
#include "uevent_env_parser.h"
#include "disk_manager_errno.h"
#include "disk_manager_dfx_types.h"
#include "disk_manager_hilog.h"
#include "disk_manager_radar.h"
#include "disk_manager_utils.h"
#include "errors.h"
#include "notification/common_event_publisher.h"
#include "volume_core.h"

#include <cctype>
#include <cinttypes>
#include <cstdlib>
#include <algorithm>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <map>
#include <mutex>
#include <string_view>
#include <sys/stat.h>
#include <sys/sysmacros.h>

namespace OHOS {
namespace DiskManager {
std::list<DiskConfig> UeventBootstrap::diskConfigList_;
std::mutex UeventBootstrap::diskConfigListMutex_;
std::map<std::string, std::vector<PartitionRecord>> UeventBootstrap::diskPartsSnapshot_;
std::mutex UeventBootstrap::diskPartsSnapshotMutex_;

namespace {

constexpr const char *DEV_BLOCK = "/dev/block/";
constexpr bool AUTO_MOUNT_EXTERNAL_VOLUMES = true;
/** 与 DiskManager::Partition 下发 storage_daemon 的 partitionType 一致。 */
constexpr const char *PARTITION_TARGET_FS_TYPE = "f2fs";
constexpr uint32_t NODE_PERM = 0660u;
constexpr uint32_t K_DISK_BLOCK_DEVICE_NODE_MODE = NODE_PERM | static_cast<uint32_t>(S_IFBLK);
constexpr uint32_t K_VOLUME_BLOCK_DEVICE_NODE_MODE = static_cast<uint32_t>(S_IFBLK);
constexpr int DISK_MMC_MAJOR = 179;
constexpr int DISK_CD_MAJOR = 11;
constexpr int32_t MIN_LINES = 32;
constexpr int32_t MAJORID_BLKEXT = 259;
constexpr int32_t MAX_PARTITION = 16;
constexpr int32_t MAX_INTERVAL_PARTITION = 15;
constexpr int32_t MAX_SCSI_VOLUMES = 15;
constexpr int32_t VOL_LENGTH = 3;
constexpr uint64_t BYTES_PER_MB = 1024 * 1024;
constexpr uint64_t MIN_DISK_SIZE_MB = 4;

const int32_t CONFIG_PARAM_NUM = 6;
#ifdef CDC_STORAGE
// it will be decoupled to the car odm
const std::string CONFIG_PTAH = "/system/etc/disk_manager/disk_config";
#else
const std::string CONFIG_PTAH = "/system/etc/disk_manager/disk_config";
#endif
constexpr const char *BLOCK_PATH = "/dev/block";
constexpr int DEC_BASE = 10;

CdromState QueryCdromState(const std::string &devPath)
{
    int32_t status = 0;
    int32_t ret = StorageDaemonAdapter::GetInstance().QueryCDStatus(devPath, status);
    if (ret != ERR_OK) {
        LOGE("QueryCdromState QueryCDStatus failed ret=%{public}d", ret);
        return CdromState::QUERY_FAILED;
    }
    if ((static_cast<uint32_t>(status) & 0x01) == 0) {
        return CdromState::NO_DISC;
    }
    if ((static_cast<uint32_t>(status) & 0x02) == 0) {
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

/** diskId 由 DiskIdFrom 生成，调用方保证形如 disk-maj-min。 */
void ParseDiskIdToMajMin(const std::string &diskId, unsigned int &outMaj, unsigned int &outMin)
{
    constexpr size_t prefixLen = 5;
    const size_t lastDash = diskId.rfind('-');
    const std::string majStr = diskId.substr(prefixLen, lastDash - prefixLen);
    const std::string minStr = diskId.substr(lastDash + 1);
    outMaj = static_cast<unsigned int>(std::strtoul(majStr.c_str(), nullptr, DEC_BASE));
    outMin = static_cast<unsigned int>(std::strtoul(minStr.c_str(), nullptr, DEC_BASE));
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
    if (partIndex > MAX_SCSI_VOLUMES) {
        return makedev(MAJORID_BLKEXT, partIndex - MAX_PARTITION);
    }
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
    LOGI("DestroyALLVolume enter diskId=%{public}s", diskId.c_str());
    std::vector<VolumeExternal> vols;
    (void)DiskManager::GetInstance().GetAllVolumes(vols);
    for (const VolumeExternal &vol : vols) {
        // volID = 0 are generated by GetAllVolumes, do not need destroy
        if (vol.GetDiskId() != diskId || vol.GetId() == "0") {
            continue;
        }
        // Already safely ejected: skip ForceUnmount to avoid duplicate EJECT/UNMOUNTED.
        if (vol.GetState() != UNMOUNTED) {
            int32_t unmountRet = DiskManager::GetInstance().ForceUnmount(vol.GetId());
            if (unmountRet != E_OK) {
                LOGE("ForceUnmount failed, volId=%{public}s, ret=%{public}d", vol.GetId().c_str(), unmountRet);
            }
        }
        int32_t ret = StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(BlockPathForId(vol.GetId()));
        if (ret != E_OK) {
            LOGI("Destroy volume failed vol:%{public}s, ret:%{public}d", vol.GetId().c_str(), ret);
            continue;
        }
        // Fuse and non-Fuse share the same remove events: REMOVED after safe eject, BAD_REMOVAL otherwise.
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
                               bool &isUserData,
                               std::string &tableType)
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
        LOGE("ReadPartitionTable failed err=%{public}d, checking disk size for disk=%{public}s", err, diskId.c_str());
        uint64_t diskSize = 0;
        int32_t sizeErr = StorageDaemonAdapter::GetInstance().GetDiskSize(env.devName, diskSize);
        if (sizeErr == ERR_OK && diskSize > 0 && (diskSize / BYTES_PER_MB) > MIN_DISK_SIZE_MB) {
            LOGI("Disk size=%{public}" PRIu64 " bytes (>4MB), treat as valid storage device, disk=%{public}s",
                 diskSize, diskId.c_str());
            return E_STORAGE_VALID_NODE;
        } else {
            LOGE("GetDiskSize failed or size too small (size=%{public}" PRIu64 "), abandon disk=%{public}s",
                 diskSize, diskId.c_str());
            (void)StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(diskDevPath);
            return err;
        }
    } else {
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
        bool hasDiskLine = false;
        if (!rawDump.empty()) {
            hasDiskLine = PartitionTableParser::ParseSgdiskDump(rawDump, diskId, tableType, parts);
        }
        if (!hasDiskLine && env.major != DISK_CD_MAJOR) {
            LOGE("ReadPartitionTable output has no DISK line, abandon disk=%{public}s", diskId.c_str());
            (void)StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(diskDevPath);
            return DiskManagerErrNo::DISK_MGR_ERR;
        }
    }
    (void)DiskManager::GetInstance().ReplacePartitionsForDisk(diskId, parts);
    return DiskManagerErrNo::E_OK;
}

std::string BlockInfoToVolumeExtraInfo(const BlockInfo &blockInfo)
{
    return BlockInfoTable::ToJsonStringWithExtras(blockInfo,
        {{"vendor", blockInfo.vendor}, {"model", blockInfo.model}, {"devnum", blockInfo.devnum},
         {"busnum", blockInfo.busnum}, {"devNode", blockInfo.devNode}, {"scsiBusNum", blockInfo.scsiBusNum},
         {"fwVersion", blockInfo.fwVersion}});
}

void UpsertDiskAndPublishEvent(const UeventEnv &env,
                               const std::string &diskId,
                               bool publishNewDiskEvent,
                               const std::string &tableType)
{
    if (!publishNewDiskEvent) {
        return;
    }
    BlockInfo blockInfo {};
    const bool hasBlockInfo = BlockInfoTable::GetInstance().TryCopyByDiskId(diskId, blockInfo);
    Disk diskForEvent(diskId, hasBlockInfo ? static_cast<int64_t>(blockInfo.sizeBytes) : 0, env.devName,
                      ResolveInitialDiskFlag(env));
    if (hasBlockInfo) {
        diskForEvent.SetExtraInfo(BlockInfoTable::ToJsonStringWithExtras(blockInfo));
    } else {
        blockInfo.diskId = diskId;
        int32_t ret = BlockInfoTable::GetInstance().ReadExtDiskInfoFromDaemon(env.devName, blockInfo);
        if (ret == ERR_OK) {
            diskForEvent.SetSizeBytes(static_cast<int64_t>(blockInfo.sizeBytes));

            diskForEvent.SetExtraInfo(BlockInfoToVolumeExtraInfo(blockInfo));
        }
    }
    diskForEvent.SetVendor(blockInfo.vendor);
    diskForEvent.SetPartitionType(tableType);
    diskForEvent.RefreshClassificationFromSysfs(env.sysPath, blockInfo.rotational);
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

bool IsInternalDataDiskById(const std::string &diskId)
{
    Disk disk;
    return DiskManager::GetInstance().GetDiskById(diskId, disk) == DiskManagerErrNo::E_OK &&
           disk.IsInternalDataDisk();
}

bool TryLoadBlockInfoForVolume(const Disk &disk, BlockInfo &blockInfo)
{
    if (disk.IsInternalDataDisk()) {
        return BlockInfoTable::GetInstance().TryCopyByDiskId(disk.GetDiskId(), blockInfo);
    }
    blockInfo.diskId = disk.GetDiskId();
    return BlockInfoTable::GetInstance().ReadExtDiskInfoFromDaemon(disk.GetDevName(), blockInfo) == ERR_OK;
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
    Disk disk;
    if (DiskManager::GetInstance().GetDiskById(diskId, disk) != E_OK) {
        LOGE("Disk with id %{public}s not found", diskId.c_str());
        return E_NON_EXIST;
    }

    BlockInfo blockInfo {};
    if (TryLoadBlockInfoForVolume(disk, blockInfo)) {
        volExternal.SetExtraInfo(BlockInfoToVolumeExtraInfo(blockInfo));
    } else if (disk.IsInternalDataDisk()) {
        LOGW("CreateAndSetupVolume: internal data disk block info cache miss diskId=%{public}s", diskId.c_str());
    }
    (void)DiskManager::GetInstance().OnVolumeCreated(volExternal);
    return ERR_OK;
}

void ReadAndUpdateMetadata(const std::string &volId, const std::string &volDevPath,
                           std::string &uuid, std::string &type, std::string &label)
{
    int32_t err = StorageDaemonAdapter::GetInstance().ReadMetadata(volDevPath, uuid, type, label);
    LOGI("UUID: %{public}s, Type: %{public}s, Label: %{public}s",
         GetAnonyString(uuid).c_str(), type.c_str(), GetAnonyString(label).c_str());
    if (err == ERR_OK) {
        if (!IsUuidValid(uuid)) {
            LOGE("ReadAndUpdateMetadata: uuid is invalid volId=%{public}s uuid=%{public}s",
                 volId.c_str(), GetAnonyString(uuid).c_str());
            return;
        }
        (void)DiskManager::GetInstance().UpdateVolumeMetadata(volId, uuid, type, label);
    }
}

static uint64_t GetDevSectorSize(const std::string &devName)
{
    std::string sysfsPath = "/sys/class/block/" + devName + "/size";
    int fd = open(sysfsPath.c_str(), O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    char buf[32] = {};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        return 0;
    }
    return strtoull(buf, nullptr, DEC_BASE);
}

static dev_t CreateDmLinearForPartition(const std::string &devName, uint32_t partitionNumber)
{
    std::string partDevName = devName + std::to_string(partitionNumber);
    constexpr uint64_t DM_RESERVED_SECTORS = (20 * BYTES_PER_MB) / 512;  // 预留 20MB
    uint64_t totalSectors = GetDevSectorSize(partDevName);
    if (totalSectors <= DM_RESERVED_SECTORS) {
        LOGE("CreateDmLinearForPartition: totalSectors=%{public}llu too small, skip",
             static_cast<unsigned long long>(totalSectors));
        return makedev(0, 0);
    }
    uint64_t mappingSectors = totalSectors - DM_RESERVED_SECTORS;
    std::string partDevPath = std::string("/dev/block/") + partDevName;
    uint64_t dmDevVal = 0;
    int32_t err = StorageDaemonAdapter::GetInstance().CreateDmLinear(partDevPath, 0, mappingSectors, dmDevVal);
    if (err != ERR_OK || dmDevVal == 0) {
        LOGE("CreateDmLinearForPartition: CreateDmLinear failed, err=%{public}d, dmDev=0", err);
        return makedev(0, 0);
    }
    dev_t dmDev = static_cast<dev_t>(dmDevVal);
    LOGI("CreateDmLinearForPartition: success, partDevPath=%{public}s, dev=(%{public}u,%{public}u)",
         partDevPath.c_str(),
         major(dmDev),
         minor(dmDev));
 
    return dmDev;
}

static dev_t ResolvePartitionDev(const UeventEnv &env, const std::string &diskId, const PartitionRecord &p,
                                 bool isUserData)
{
    if (IsInternalDataDiskById(diskId)) {
        dev_t dmDev = CreateDmLinearForPartition(env.devName, p.partitionNumber);
        if (dmDev != makedev(0, 0)) {
            return dmDev;
        }
    }
 
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
 
    return pDev;
}

void DiscoverSinglePartitionVolume(const UeventEnv &env,
                                   const std::string &diskId,
                                   const PartitionRecord &p,
                                   const bool &isUserData)
{
    dev_t pDev = ResolvePartitionDev(env, diskId, p, isUserData);
    const std::string volId = VolIdFromDev(pDev);
    VolumeExternal vol;
    if (DiskManager::GetInstance().GetVolumeById(volId, vol) == E_OK && vol.GetState() == VolumeState::MOUNTED) {
        return;
    }
    if (CreateAndSetupVolume(diskId, pDev, isUserData, static_cast<int32_t>(p.partitionNumber)) !=
        ERR_OK) {
        return;
    }
    std::string uuid;
    std::string type;
    std::string label;
    const std::string volDevPath = BlockPathForId(volId);
    ReadAndUpdateMetadata(volId, volDevPath, uuid, type, label);
    if (DiskManager::GetInstance().IsPartitioning(diskId)) {
        const int32_t formatRet = DiskManager::GetInstance().Format(volId, PARTITION_TARGET_FS_TYPE);
        if (formatRet != ERR_OK) {
            LOGE("DiscoverSinglePartitionVolume Format failed volId=%{public}s ret=%{public}d", volId.c_str(),
                 formatRet);
        }
        return;
    }
    LOGI("AUTO_MOUNT_EXTERNAL_VOLUMES: %{public}d, type.empty(): %{public}d, uuid.empty(): %{public}d",
         AUTO_MOUNT_EXTERNAL_VOLUMES, type.empty(), uuid.empty());
    if (!AUTO_MOUNT_EXTERNAL_VOLUMES || type.empty() || uuid.empty()) {
        DiskManagerRadar::GetInstance().ReportDiscoverAutoMountSkipFault(
            {volId, diskId, volDevPath, type, uuid, AUTO_MOUNT_EXTERNAL_VOLUMES});
        return;
    }
    int32_t err = DiskManager::GetInstance().Mount(volId);
    if (err != ERR_OK) {
        LOGE("DiscoverSinglePartitionVolume Mount failed volId=%{public}s", volId.c_str());
        return;
    }
    LOGI("DiscoverSinglePartitionVolume EXIT SUCCESS");
}

void DiscoverWholeDiskVolume(const UeventEnv &env, const std::string &diskId)
{
    LOGI("DiscoverWholeDiskVolume enter diskId=%{public}s (no valid partition, fallback to whole-disk volume)",
         diskId.c_str());

    const dev_t wholeDev = makedev(env.major, env.minor);
    const std::string volId = VolIdFromDev(wholeDev);
    const std::string diskDevPath = BlockPathForId(diskId);
    const bool isPartitioning = DiskManager::GetInstance().IsPartitioning(diskId);

    std::string uuid;
    std::string type;
    std::string label;
    if (!isPartitioning) {
        int32_t metaRet = StorageDaemonAdapter::GetInstance().ReadMetadata(diskDevPath, uuid, type, label);
        if (metaRet != ERR_OK) {
            LOGI("DiscoverWholeDiskVolume ReadMetadata ret=%{public}d, no FS on whole-disk, skip creating volume",
                 metaRet);
            return;
        }
    }

    if (CreateAndSetupVolume(diskId, wholeDev, false, 0) != ERR_OK) {
        return;
    }

    if (isPartitioning) {
        const int32_t formatRet = DiskManager::GetInstance().Format(volId, PARTITION_TARGET_FS_TYPE);
        if (formatRet != ERR_OK) {
            LOGE("DiscoverWholeDiskVolume Format failed volId=%{public}s ret=%{public}d", volId.c_str(),
                 formatRet);
        }
        return;
    }

    (void)DiskManager::GetInstance().UpdateVolumeMetadata(volId, uuid, type, label);

    LOGI("DiscoverWholeDiskVolume AUTO_MOUNT=%{public}d type.empty=%{public}d uuid.empty=%{public}d",
         AUTO_MOUNT_EXTERNAL_VOLUMES, type.empty(), uuid.empty());
    if (!AUTO_MOUNT_EXTERNAL_VOLUMES || type.empty() || uuid.empty()) {
        return;
    }

    int32_t err = DiskManager::GetInstance().Mount(volId);
    if (err != ERR_OK) {
        LOGE("DiscoverWholeDiskVolume Mount failed volId=%{public}s", volId.c_str());
        return;
    }
    LOGI("DiscoverWholeDiskVolume EXIT SUCCESS");
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
        uuid = GenerateRandomUuid();
        type = "udf";
        VolumeExternal volume;
        if (DiskManager::GetInstance().GetVolumeById(volId, volume) == DiskManagerErrNo::E_OK) {
            const std::string extraInfo = volume.GetExtraInfo();
            label = DiskManager::GetInstance().GetDriverType(extraInfo);
            if (label.empty()) {
                label = "DVD RW";
            }
        } else {
            label = "DVD RW";
        }
    } else {
        ReadAndUpdateMetadata(volId, volDevPath, uuid, type, label);
    }
    (void)DiskManager::GetInstance().UpdateVolumeMetadata(volId, uuid, type, label);
    (void)DiskManager::GetInstance().SetVolumeDiscState(volId, state);

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
        const int32_t ret = DiskManager::GetInstance().Eject(diskId);
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

    if (state == CdromState::QUERY_FAILED) {
        // SCSI 查询失败，用 ReadMetadata 兜底判断是否有数据
        // 使用 diskDevPath（/dev/block/disk-11-0），该节点已在 DiscoverCdromVolumes 中创建
        std::string uuid;
        std::string type;
        std::string label;
        const std::string diskDevPath = BlockPathForId(diskId);
        int32_t metaRet = StorageDaemonAdapter::GetInstance().ReadMetadata(diskDevPath, uuid, type, label);
        if (metaRet == ERR_OK && !type.empty()) {
            LOGI("SCSI query failed but ReadMetadata succeeded, treat as NON_EMPTY_DISC");
            state = CdromState::NON_EMPTY_DISC;
        } else {
            LOGE("SCSI query and ReadMetadata both failed, treat as NO_DISC");
            DestroyALLVolume(diskId);
            return;
        }
    }

    Disk disk;
    if (DiskManager::GetInstance().GetDiskById(diskId, disk) == DiskManagerErrNo::E_OK) {
        disk.SetCdromState(state);
        (void)DiskManager::GetInstance().UpdateDisk(disk);
    }
    HandleAddCD(env, diskId, state);
}

int32_t DiscoverCdromVolumes(const UeventEnv &env, const std::string &diskId, bool publishNewDiskEvent)
{
    LOGI("DiscoverCdromVolumes enter diskId=%{public}s publishNew=%{public}d",
         diskId.c_str(), static_cast<int>(publishNewDiskEvent));

    const std::string diskDevPath = BlockPathForId(diskId);
    int32_t err = StorageDaemonAdapter::GetInstance().CreateBlockDeviceNode(
        diskDevPath, K_DISK_BLOCK_DEVICE_NODE_MODE, static_cast<int32_t>(env.major), static_cast<int32_t>(env.minor));
    if (err != ERR_OK) {
        LOGE("DiscoverCdromVolumes CreateBlockDeviceNode failed err=%{public}d", err);
        return err;
    }

    const bool publishNew = publishNewDiskEvent || !DiskManager::GetInstance().HasDisk(diskId);
    UpsertDiskAndPublishEvent(env, diskId, publishNew, "cd");
    DiscoverSinglePartitionVolume4CD(env, diskId);
    return DiskManagerErrNo::E_OK;
}

inline bool PartitionKeyEqual(const PartitionRecord &a, const PartitionRecord &b)
{
    return a.partitionNumber == b.partitionNumber &&
           a.partitionType == b.partitionType &&
           a.fsTypeRaw == b.fsTypeRaw;
}

bool ContainsPartition(const std::vector<PartitionRecord> &list, const PartitionRecord &target)
{
    return std::any_of(list.begin(), list.end(),
                       [&](const PartitionRecord &r) { return PartitionKeyEqual(r, target); });
}
} // namespace

void UeventBootstrap::ComputePartitionDiff(const std::string &diskId,
                                           const std::vector<PartitionRecord> &newParts,
                                           std::vector<PartitionRecord> &added,
                                           std::vector<PartitionRecord> &removed)
{
    std::lock_guard<std::mutex> lock(diskPartsSnapshotMutex_);
    auto it = diskPartsSnapshot_.find(diskId);
    if (it == diskPartsSnapshot_.end()) {
        added = newParts;
        diskPartsSnapshot_[diskId] = newParts;
        return;
    }
    const auto &old = it->second;
    for (const auto &np : newParts) {
        if (!ContainsPartition(old, np)) {
            added.push_back(np);
        }
    }
    for (const auto &op : old) {
        if (!ContainsPartition(newParts, op)) {
            removed.push_back(op);
        }
    }
    diskPartsSnapshot_[diskId] = newParts;
}

void UeventBootstrap::ClearPartitionSnapshot(const std::string &diskId)
{
    std::lock_guard<std::mutex> lock(diskPartsSnapshotMutex_);
    diskPartsSnapshot_.erase(diskId);
}

int32_t UeventBootstrap::OnBlockDiskUevent(const std::string &rawUeventMsg)
{
    LOGI("UeventBootstrap::OnBlockDiskUevent enter external=IDiskManager::OnBlockDiskUevent rawLen=%{public}zu",
         rawUeventMsg.size());

    UeventEnv env;
    if (!UeventEnvParser::Parse(rawUeventMsg, env)) {
        LOGE("OnBlockDiskUevent parse failed rawLen=%{public}zu preview=%{public}s", rawUeventMsg.size(),
             DfxTruncate(rawUeventMsg).c_str());
        VolumeReportInfo faultInfo;
        faultInfo.extra = "ueventMsg=" + DfxTruncate(rawUeventMsg);
        DiskManagerRadar::GetInstance().ReportUeventParseFault(faultInfo);
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
    ClearPartitionSnapshot(diskId);
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

    if (env.major == DISK_CD_MAJOR) {
        return DiscoverCdromVolumes(env, diskId, publishNewDiskEvent);
    }

    std::vector<PartitionRecord> parts;
    bool isUserData = false;
    std::string tableType;
    int32_t err = BuildAndSyncPartitions(env, diskId, diskDevPath, parts, isUserData, tableType);
    if (err == E_STORAGE_VALID_NODE) {
        UpsertDiskAndPublishEvent(env, diskId, publishNewDiskEvent, tableType);
        LOGI("UpsertDiskAndPublishEvent completed for disk ID: %{public}s, is a valid storage device", diskId.c_str());
        return DiskManagerErrNo::E_OK;
    }
    if (err != ERR_OK) {
        LOGE("BuildAndSyncPartitions failed with error: %{public}d", err);
        if (!publishNewDiskEvent) {
            DestroyALLVolume(diskId);
            ClearPartitionSnapshot(diskId);
        }
        return err;
    }
    LOGI("BuildAndSyncPartitions completed successfully");

    UpsertDiskAndPublishEvent(env, diskId, publishNewDiskEvent, tableType);
    LOGI("UpsertDiskAndPublishEvent completed for disk ID: %{public}s", diskId.c_str());

    // 内置数据盘 Partition() 仅恢复为单一 f2fs，无增删分区场景，直接 discover + Format。
    if (DiskManager::GetInstance().IsPartitioning(diskId) && IsInternalDataDiskById(diskId)) {
        for (const auto &p : parts) {
            DiscoverSinglePartitionVolume(env, diskId, p, isUserData);
        }
        return DiskManagerErrNo::E_OK;
    }

    std::vector<PartitionRecord> addedParts;
    std::vector<PartitionRecord> removedParts;
    ComputePartitionDiff(diskId, parts, addedParts, removedParts);

    for (const auto &rp : removedParts) {
        LOGI("Cleanup volume for removed partition %{public}u on disk %{public}s", rp.partitionNumber,
             diskId.c_str());
        (void)DiskManager::GetInstance().DestroyVolumeByDiskIdAndPartNum(
            diskId, static_cast<int32_t>(rp.partitionNumber));
    }

    for (const auto &p : addedParts) {
        LOGI("Discovering volume for added partition number: %{public}u", p.partitionNumber);
        DiscoverSinglePartitionVolume(env, diskId, p, isUserData);
    }

    if (parts.empty() && (publishNewDiskEvent || !removedParts.empty())) {
        DiscoverWholeDiskVolume(env, diskId);
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

    const std::string diskId = DiskIdFrom(env.major, env.minor);
    if (DiskManager::GetInstance().IsPartitioning(diskId) && IsInternalDataDiskById(diskId)) {
        LOGI("HandleDiskChange skipped diskId=%{public}s", diskId.c_str());
        return DiskManagerErrNo::E_OK;
    }

    const bool publishNew = !DiskManager::GetInstance().HasDisk(diskId);
    int32_t ret = DiscoverPartitionsAndVolumes(env, publishNew);
    DiskManager::GetInstance().NotifyPartitionDone(diskId);
    return ret;
}

int32_t UeventBootstrap::RediscoverDiskVolumes(const std::string &diskId)
{
    unsigned int maj = 0;
    unsigned int min = 0;
    ParseDiskIdToMajMin(diskId, maj, min);

    UeventEnv env;
    env.action = "change";
    env.major = maj;
    env.minor = min;
    LOGI("RediscoverDiskVolumes diskId=%{public}s maj=%{public}u min=%{public}u", diskId.c_str(), maj, min);
    return DiscoverPartitionsAndVolumes(env, false);
}

void UeventBootstrap::ResetPartitionSnapshotForTest()
{
    std::lock_guard<std::mutex> lock(diskPartsSnapshotMutex_);
    diskPartsSnapshot_.clear();
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
