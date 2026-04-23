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

#include "disk/disk_manager.h"

#include "storage_daemon_adapter.h"

#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "notification/common_event_publisher.h"

#include <cinttypes>
#include <cstdint>
#include <mutex>
#include <sys/statvfs.h>

namespace OHOS {
namespace DiskManager {

namespace {

/** 对齐 storage_manager::VolumeStorageStatusService::GetVolumePath */
std::string GetVolumePath(DiskDataManager &dm, const std::string &volumeUuid)
{
    VolumeExternal vol;
    if (dm.GetVolumeByUuid(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
        LOGE("GetVolumePath fail.");
        return "";
    }
    return vol.GetPath();
}

/** 对齐 VolumeStorageStatusService::IsOddDevice */
bool IsOddDevice(DiskDataManager &dm, const std::string &volumeUuid)
{
    VolumeExternal vol;
    if (dm.GetVolumeByUuid(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
        LOGE("IsOddDevice: volExternalInfo is null");
        return false;
    }
    const std::string fsType = vol.GetFsTypeString();
    if (fsType == "udf" || fsType == "iso9660") {
        return true;
    }
    return false;
}

int32_t GetOddSize(DiskDataManager &dm, const std::string &volumeUuid, int64_t &totalSize, int64_t &freeSize)
{
    VolumeExternal vol;
    if (dm.GetVolumeByUuid(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
        LOGE("GetOddSize: volExternalInfo is null");
        return DiskManagerErrNo::DISK_MGR_ERR;
    }
    const std::string &mountPath = vol.GetPath();
    LOGI("get odd size : mountPath is %{public}s", mountPath.c_str());
    if (mountPath.empty()) {
        return DiskManagerErrNo::DISK_MGR_ERR;
    }
    return StorageDaemonAdapter::GetInstance().GetCapacity(mountPath, totalSize, freeSize);
}

} // namespace

DiskDataManager::DiskDataManager() = default;

DiskDataManager::~DiskDataManager() = default;

DiskDataManager &DiskDataManager::GetInstance()
{
    static DiskDataManager instance;
    return instance;
}

int32_t DiskDataManager::Mount(const std::string &volumeId)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;

    int32_t err = StorageDaemonAdapter::GetInstance().Mount("/dev/block/" + volExternal.GetId(),
                                                            "/mnt/data/external/" + volExternal.GetUuid(),
                                                            volExternal.GetFsTypeString(), "");
    if (err != ERR_OK) {
        LOGE("MountFs vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    volExternal.SetState(MOUNTED);
    CommonEventPublisher::PublishVolumeChange(MOUNTED, volExternal);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::Unmount(const std::string &volumeId)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;

    int32_t err = StorageDaemonAdapter::GetInstance().Unmount("/mnt/data/external/" + volExternal.GetUuid(),
                                                              volExternal.GetFsTypeString(), true);
    if (err != ERR_OK) {
        LOGE("Unmount vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    volExternal.SetState(UNMOUNTED);
    CommonEventPublisher::PublishVolumeChange(UNMOUNTED, volExternal);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::Format(const std::string &volumeId, const std::string &fsType)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;

    int32_t err = StorageDaemonAdapter::GetInstance().FormatVolume("/dev/block/" + volExternal.GetId(), fsType);
    if (err != ERR_OK) {
        LOGE("Format vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    volExternal.SetState(UNMOUNTED);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::TryToFix(const std::string &volumeId)
{
    (void)volumeId;
    return DiskManagerErrNo::DISK_MGR_ERR;
}

int32_t DiskDataManager::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = std::find_if(volumeMap_.begin(), volumeMap_.end(),
                           [&fsUuid](const auto &pair) { return pair.second.GetUuid() == fsUuid; });
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", fsUuid.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;
    int32_t err = StorageDaemonAdapter::GetInstance().SetLabel("/dev/block/" + volExternal.GetId(),
                                                               volExternal.GetFsTypeString(), description);
    if (err != ERR_OK) {
        LOGE("SetLabel vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    volExternal.SetState(UNMOUNTED);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::Partition(const std::string &diskId, int32_t type)
{
    return StorageDaemonAdapter::GetInstance().Partition(diskId, type, 0);
}

int32_t DiskDataManager::OnDiskCreated(const Disk &disk)
{
    std::lock_guard<std::mutex> lock(diskMapMutex_);
    if (diskMap_.find(disk.GetDiskId()) != diskMap_.end()) {
        LOGE("DiskManagerService::OnDiskCreated the disk %{public}s already exists", disk.GetDiskId().c_str());
        return DiskManagerErrNo::E_DISK_HAS_EXIST;
    }
    diskMap_.insert(make_pair(disk.GetDiskId(), disk));
    LOGI("DiskManagerService::OnDiskCreated successfully recorded disk %{public}s", disk.GetDiskId().c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::UpsertDiskSnapshot(const DiskStoreRecord &rec)
{
    (void)rec;
    return DiskManagerErrNo::E_OK;
}

bool DiskDataManager::HasDisk(const std::string &diskId)
{
    (void)diskId;
    return false;
}

int32_t DiskDataManager::OnDiskDestroyed(const std::string &diskId)
{
    std::lock_guard<std::mutex> lock(diskMapMutex_);
    if (diskMap_.find(diskId) == diskMap_.end()) {
        LOGE("DiskDataManager::OnDiskDestroyed the disk %{public}s doesn't exist", diskId.c_str());
        return E_DISK_NOT_FOUND;
    }
    diskMap_.erase(diskId);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeCreated(const VolumeExternal &volExternal)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    volumeMap_.insert(make_pair(volExternal.GetId(), volExternal));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeMounted(const std::string &volumeId, const std::string &mountPath, int32_t state)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;
    volExternal.SetPath(mountPath);
    volExternal.SetState(MOUNTED);
    LOGI("OnVolumeMounted-  diskId:%{public}s, volId:%{public}s, state:%{public}d", volExternal.GetDiskId().c_str(),
         volExternal.GetId().c_str(), volExternal.GetState());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeStateChanged(const std::string &volumeId, uint32_t state)
{
    (void)volumeId;
    (void)state;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeDestroyed(const std::string &volumeId)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    if (volumeMap_.find(volumeId) == volumeMap_.end()) {
        LOGE("DiskDataManager::OnVolumeDestroyed the vol %{public}s doesn't exist", volumeId.c_str());
        return E_VOLUME_NOT_FOUND;
    }
    volumeMap_.erase(volumeId);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::OnVolumeDamaged(const VolumeInfoStr &vis)
{
    (void)vis;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::ReplacePartitionsForDisk(const std::string &diskId,
                                                  const std::vector<PartitionRecord> &partitions)
{
    (void)diskId;
    (void)partitions;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetAllDisks(std::vector<Disk> &out)
{
    out.clear();
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetDiskById(const std::string &diskId, Disk &out)
{
    std::lock_guard<std::mutex> lock(diskMapMutex_);
    if (diskMap_.find(diskId) == diskMap_.end()) {
        LOGE("DiskDataManager::GetDiskById id %{public}s not exists", diskId.c_str());
        return E_DISK_NOT_FOUND;
    }
    out = diskMap_[diskId];
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetAllVolumes(std::vector<VolumeExternal> &out)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    std::vector<VolumeExternal> result;
    for (auto it = volumeMap_.begin(); it != volumeMap_.end(); ++it) {
        VolumeExternal volExternal = it->second;
        result.push_back(volExternal);
    }
    out = result;
    LOGI("GetAllVolumes - Found %{public}zu volumes:", out.size());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetVolumeById(const std::string &volumeId, VolumeExternal &out)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    for (auto it = volumeMap_.begin(); it != volumeMap_.end(); ++it) {
        const VolumeExternal &volExternal = it->second;
        if (volExternal.GetId() == volumeId) {
            LOGI("VolumeManagerService::GetVolumeByUuid volumeUuid %{public}s exists", volumeId.c_str());
            out = volExternal;
            return DiskManagerErrNo::E_OK;
        }
    }
    return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
}

int32_t DiskDataManager::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &out)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    for (auto it = volumeMap_.begin(); it != volumeMap_.end(); ++it) {
        const VolumeExternal &volExternal = it->second;
        if (volExternal.GetUuid() == fsUuid) {
            LOGI("VolumeManagerService::GetVolumeByUuid volumeUuid %{public}s exists", fsUuid.c_str());
            out = volExternal;
            return DiskManagerErrNo::E_OK;
        }
    }
    return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
}

int32_t DiskDataManager::UpdateVolumeMetadata(const std::string &volumeId,
                                              const std::string &fsUuid,
                                              const std::string &fsTypeStr,
                                              const std::string &description)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;
    volExternal.SetFsUuid(fsUuid);
    volExternal.SetFsType(volExternal.GetFsTypeByStr(fsTypeStr));
    volExternal.SetDescription(description);
    LOGI("Updated metadata for volume %{public}s: uuid=%{public}s, fsType=%{public}d, description=%{public}s",
         volumeId.c_str(), fsUuid.c_str(), volExternal.GetFsType(), description.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize)
{
    freeSize = 0;
    std::string path = GetVolumePath(*this, volumeUuid);
    LOGI("GetFreeSizeOfVolume path is %{public}s", path.c_str());
    if (path == "") {
        return DiskManagerErrNo::E_NON_EXIST;
    }
    struct statvfs diskInfo {};
    int ret = statvfs(path.c_str(), &diskInfo);
    if (ret != DiskManagerErrNo::E_OK) {
        return DiskManagerErrNo::E_STATVFS;
    }
    if (IsOddDevice(*this, volumeUuid)) {
        int64_t totalSize = 0;
        int64_t startTotalSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_blocks);
        int64_t startFreeSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_bfree);
        const int32_t oddRet = GetOddSize(*this, volumeUuid, totalSize, freeSize);
        LOGI("totalSize is %{public}" PRIu64 " freeSize is %{public}" PRIu64 ", ret val is %{public}d",
             static_cast<uint64_t>(totalSize), static_cast<uint64_t>(freeSize), oddRet);
        if (freeSize != 0) {
            return oddRet;
        }
        if (startFreeSize == 0) {
            freeSize = totalSize - startTotalSize;
            return DiskManagerErrNo::E_OK;
        }
    }
    freeSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_bfree);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskDataManager::GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize)
{
    totalSize = 0;
    std::string path = GetVolumePath(*this, volumeUuid);
    if (path == "") {
        return DiskManagerErrNo::E_NON_EXIST;
    }
    struct statvfs diskInfo {};
    int ret = statvfs(path.c_str(), &diskInfo);
    if (ret != DiskManagerErrNo::E_OK) {
        return DiskManagerErrNo::E_STATVFS;
    }
    if (IsOddDevice(*this, volumeUuid)) {
        int64_t freeSize = 0;
        (void)GetOddSize(*this, volumeUuid, totalSize, freeSize);
        return DiskManagerErrNo::E_OK;
    }
    totalSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_blocks);
    return DiskManagerErrNo::E_OK;
}
} // namespace DiskManager
} // namespace OHOS
