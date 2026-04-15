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

#include "disk/uevent_bootstrap.h"

#include "adapter/storage_daemon_adapter.h"
#include "disk.h"
#include "disk/disk_manager.h"
#include "disk/partition_table_parser.h"
#include "disk/storage_spec_models.h"
#include "disk/uevent_env_parser.h"
#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "errors.h"
#include "notification/common_event_publisher.h"
#include "volume_core.h"

#include <string_view>
#include <sys/stat.h>
#include <sys/sysmacros.h>

namespace OHOS {
namespace DiskManager {

namespace {

constexpr const char *DEV_BLOCK = "/dev/block/";
constexpr const char *EXTERNAL_MOUNT_ROOT = "/mnt/data/external/";
constexpr bool AUTO_MOUNT_EXTERNAL_VOLUMES = true;
constexpr uint32_t NODE_PERM = 0660u;
constexpr uint32_t kDiskBlockDeviceNodeMode = NODE_PERM | static_cast<uint32_t>(S_IFBLK);
constexpr uint32_t kVolumeBlockDeviceNodeMode = static_cast<uint32_t>(S_IFBLK);

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

DiskStoreDiskType DiskTypeFromDevPath(const std::string &devPath)
{
    if (devPath.find("usb") != std::string::npos) {
        return DiskStoreDiskType::USB_FLASH;
    }
    return DiskStoreDiskType::SD_CARD;
}

int32_t LegacyDiskFlagFromDevPath(const std::string &devPath)
{
    if (devPath.find("usb") != std::string::npos) {
        return USB_FLAG;
    }
    return SD_FLAG;
}

void LogUeventEnvForHandler(const char *handler, const UeventEnv &env)
{
    LOGI("%{public}s uevent: action=%{public}s subsystem=%{public}s devtype=%{public}s major=%{public}u "
         "minor=%{public}u devpath=%{public}s devname=%{public}s ejectRequest=%{public}d sysPath=%{public}s",
         handler, env.action.c_str(), env.subsystem.c_str(), env.devType.c_str(), env.major, env.minor,
         env.devPath.c_str(), env.devName.c_str(), static_cast<int>(env.ejectRequest), env.sysPath.c_str());
}

int32_t BuildAndSyncPartitions(const UeventEnv &env,
                               const std::string &diskId,
                               const std::string &diskDevPath,
                               std::vector<PartitionRecord> &parts)
{
    int32_t err = StorageDaemonAdapter::GetInstance().CreateBlockDeviceNode(
        diskDevPath, kDiskBlockDeviceNodeMode, static_cast<int32_t>(env.major), static_cast<int32_t>(env.minor));
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
    if (!rawDump.empty()) {
        std::string tableType;
        (void)PartitionTableParser::ParseSgdiskDump(rawDump, diskId, tableType, parts);
    }
    (void)DiskDataManager::GetInstance().ReplacePartitionsForDisk(diskId, parts);
    return DiskManagerErrNo::E_OK;
}

void UpsertDiskAndPublishEvent(const UeventEnv &env, const std::string &diskId, bool publishNewDiskEvent)
{
    DiskStoreRecord drec;
    drec.diskId = diskId;
    drec.diskName = env.devName;
    drec.sysPath = env.sysPath;
    drec.removable = true;
    drec.diskType = DiskTypeFromDevPath(env.devPath);
    drec.mediaType = DiskStoreMediaType::UNKNOWN;
    drec.sizeBytes = 0;
    (void)DiskDataManager::GetInstance().UpsertDiskSnapshot(drec);

    if (!publishNewDiskEvent) {
        return;
    }
    Disk diskForEvent(diskId, 0, env.sysPath, "", LegacyDiskFlagFromDevPath(env.devPath));
    CommonEventPublisher::PublishDiskChange(DiskEventKind::MOUNTED, diskForEvent);
}

void DiscoverSinglePartitionVolume(const UeventEnv &env, const std::string &diskId, const PartitionRecord &p)
{
    const dev_t pDev = PartitionDev(env.major, env.minor, p.partitionNumber);
    const std::string volId = VolIdFromDev(pDev);
    const std::string volDevPath = BlockPathForId(volId);

    int32_t err = StorageDaemonAdapter::GetInstance().CreateBlockDeviceNode(
        volDevPath,
        kVolumeBlockDeviceNodeMode,
        static_cast<int32_t>(major(pDev)),
        static_cast<int32_t>(minor(pDev)));
    if (err != ERR_OK) {
        LOGE("CreateVolumeBlockDeviceNode vol %{public}s err=%{public}d", volId.c_str(), err);
        return;
    }

    VolumeCore core(volId, EXTERNAL, diskId, UNMOUNTED);
    (void)DiskDataManager::GetInstance().OnVolumeCreated(core);

    std::string uuid;
    std::string type;
    std::string label;
    err = StorageDaemonAdapter::GetInstance().ReadMetadata(volDevPath, uuid, type, label);
    if (err == ERR_OK) {
        (void)DiskDataManager::GetInstance().UpdateVolumeMetadata(volId, uuid, type, label);
    }
    if (!AUTO_MOUNT_EXTERNAL_VOLUMES || type.empty() || uuid.empty()) {
        return;
    }

    const std::string mountPath = std::string(EXTERNAL_MOUNT_ROOT) + uuid;
    err = StorageDaemonAdapter::GetInstance().Mount(volDevPath, mountPath, type, "");
    if (err != ERR_OK) {
        LOGE("MountFs vol %{public}s err=%{public}d", volId.c_str(), err);
        return;
    }

    VolumeInfoStr vis(volId, type, uuid, mountPath, label, false);
    (void)DiskDataManager::GetInstance().OnVolumeMounted(vis);
    VolumeExternal ve;
    if (DiskDataManager::GetInstance().GetVolumeById(volId, ve) == DiskManagerErrNo::E_OK) {
        CommonEventPublisher::PublishVolumeChange(MOUNTED, ve);
    }
}

} // namespace

int32_t UeventBootstrap::OnBlockDiskUevent(const std::string &rawUeventMsg)
{
    LOGI("UeventBootstrap::OnBlockDiskUevent enter external=IDiskManager::OnBlockDiskUevent "
         "rawLen=%{public}zu",
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
    LogUeventEnvForHandler("HandleDiskRemove", env);

    const std::string diskId = DiskIdFrom(env.major, env.minor);
    std::vector<VolumeExternal> vols;
    (void)DiskDataManager::GetInstance().GetAllVolumes(vols);
    for (const auto &v : vols) {
        if (v.GetDiskId() != diskId) {
            continue;
        }
        (void)StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(BlockPathForId(v.GetId()));
    }
    (void)StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(BlockPathForId(diskId));

    Disk diskSnap;
    const bool hadDisk = DiskDataManager::GetInstance().GetDiskById(diskId, diskSnap) == DiskManagerErrNo::E_OK;
    (void)DiskDataManager::GetInstance().OnDiskDestroyed(diskId);
    if (hadDisk) {
        CommonEventPublisher::PublishDiskChange(DiskEventKind::REMOVED, diskSnap);
    }
    return DiskManagerErrNo::E_OK;
}

int32_t UeventBootstrap::DiscoverPartitionsAndVolumes(const UeventEnv &env, bool publishNewDiskEvent)
{
    LOGI("UeventBootstrap::DiscoverPartitionsAndVolumes enter "
         "external=IDiskManager::OnBlockDiskUevent publishNewDisk=%{public}d",
         static_cast<int>(publishNewDiskEvent));
    LogUeventEnvForHandler("DiscoverPartitionsAndVolumes", env);

    const std::string diskId = DiskIdFrom(env.major, env.minor);
    const std::string diskDevPath = BlockPathForId(diskId);

    std::vector<PartitionRecord> parts;
    int32_t err = BuildAndSyncPartitions(env, diskId, diskDevPath, parts);
    if (err != ERR_OK) {
        return err;
    }

    UpsertDiskAndPublishEvent(env, diskId, publishNewDiskEvent);
    for (const auto &p : parts) {
        DiscoverSinglePartitionVolume(env, diskId, p);
    }

    return DiskManagerErrNo::E_OK;
}

int32_t UeventBootstrap::HandleDiskAdd(const UeventEnv &env)
{
    LOGI("UeventBootstrap::HandleDiskAdd enter external=IDiskManager::OnBlockDiskUevent branch=add");
    LogUeventEnvForHandler("HandleDiskAdd", env);

    const std::string diskId = DiskIdFrom(env.major, env.minor);
    const bool publishNew = !DiskDataManager::GetInstance().HasDisk(diskId);
    return DiscoverPartitionsAndVolumes(env, publishNew);
}

int32_t UeventBootstrap::HandleDiskChange(const UeventEnv &env)
{
    LOGI("UeventBootstrap::HandleDiskChange enter external=IDiskManager::OnBlockDiskUevent branch=change");
    LogUeventEnvForHandler("HandleDiskChange", env);

    const std::string diskDevPath = BlockPathForId(DiskIdFrom(env.major, env.minor));
    if (env.ejectRequest) {
        LOGI("HandleDiskChange ejectRequest IStorageDaemon::Eject devPath=%{public}s", diskDevPath.c_str());
        const int32_t ej = StorageDaemonAdapter::GetInstance().Eject(diskDevPath);
        if (ej != ERR_OK) {
            LOGW("Eject err=%{public}d", ej);
        }
        return DiskManagerErrNo::E_OK;
    }
    const std::string diskId = DiskIdFrom(env.major, env.minor);
    const bool publishNew = !DiskDataManager::GetInstance().HasDisk(diskId);
    return DiscoverPartitionsAndVolumes(env, publishNew);
}

} // namespace DiskManager
} // namespace OHOS
