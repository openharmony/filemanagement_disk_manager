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
#include "usb_fuse_adapter.h"

#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "disk_manager_napi_errno.h"
#include "notification/common_event_publisher.h"

#include <cinttypes>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>
#include <sys/statvfs.h>

namespace OHOS {
namespace DiskManager {

std::string DiskManager::BuildUsbFuseOptions(int32_t fuseFd)
{
    std::stringstream ss;
    ss << "fd=" << fuseFd << ",";
    ss << "rootmode=40000,";
    ss << "default_permissions,";
    ss << "allow_other,";
    ss << "user_id=0,group_id=0,";
    ss << "context=\"u:object_r:mnt_external_file:s0\",";
    ss << "fscontext=u:object_r:mnt_external_file:s0";
    return ss.str();
}

bool DiskManager::IsSafeFsUuid(const std::string &fsUuid)
{
    return !(fsUuid.find("..") != std::string::npos || fsUuid.find('/') != std::string::npos);
}

std::string DiskManager::GetVolumePath(const std::string &volumeUuid)
{
    VolumeExternal vol;
    if (GetVolumeByUuid(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
        LOGE("GetVolumePath fail.");
        return "";
    }
    return vol.GetPath();
}

bool DiskManager::IsOddDevice(const std::string &volumeUuid)
{
    VolumeExternal vol;
    if (GetVolumeByUuid(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
        LOGE("IsOddDevice: volExternalInfo is null");
        return false;
    }
    const std::string fsType = vol.GetFsTypeString();
    return (fsType == "udf" || fsType == "iso9660");
}

int32_t DiskManager::GetOddSize(const std::string &volumeUuid, int64_t &totalSize, int64_t &freeSize)
{
    VolumeExternal vol;
    if (GetVolumeByUuid(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
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

bool DiskManager::IsPathMounted(std::string path)
{
    if (path.empty()) {
        return true;
    }
    if (!path.empty() && path.back() == '/') {
        path.pop_back();
    }
    std::ifstream inputStream("/proc/mounts", std::ios::in);
    if (!inputStream.is_open()) {
        LOGE("IsPathMounted: open /proc/mounts failed");
        return true;
    }
    std::string tmpLine;
    while (std::getline(inputStream, tmpLine)) {
        std::stringstream ss(tmpLine);
        std::string dst;
        ss >> dst;
        ss >> dst;
        if (path == dst) {
            return true;
        }
    }
    return false;
}

int32_t DiskManager::EnsureFsUuidReady(VolumeExternal &volExternal, std::string &outFsUuid)
{
    outFsUuid = volExternal.GetUuid();
    if (!outFsUuid.empty()) {
        return DiskManagerErrNo::E_OK;
    }
    std::string uuid;
    std::string type;
    std::string label;
    const int32_t err =
        StorageDaemonAdapter::GetInstance().ReadMetadata("/dev/block/" + volExternal.GetId(), uuid, type, label);
    if (err != ERR_OK) {
        LOGE("EnsureFsUuidReady: ReadMetadata failed volId=%{public}s err=%{public}d", volExternal.GetId().c_str(),
             err);
        return err;
    }
    volExternal.SetFsUuid(uuid);
    outFsUuid = uuid;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::MountUsbFuseIfNeeded(const std::string &volumeId,
                                          VolumeExternal &volExternal,
                                          const std::string &fsType)
{
    if (!UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType(fsType)) {
        return DiskManagerErrNo::E_OK;
    }

    std::string fsUuid;
    int32_t err = EnsureFsUuidReady(volExternal, fsUuid);
    if (err != DiskManagerErrNo::E_OK) {
        return err;
    }
    if (!IsSafeFsUuid(fsUuid)) {
        LOGE("MountUsbFuseIfNeeded: invalid fsUuid for volumeId=%{public}s", volumeId.c_str());
        return E_PARAMS_INVALID;
    }

    const std::string mountPath = "/mnt/data/external/" + fsUuid;
    if (IsPathMounted(mountPath)) {
        return DiskManagerErrNo::E_OK;
    }

    err = StorageDaemonAdapter::GetInstance().EnsureMountPath(mountPath);
    if (err != ERR_OK) {
        LOGE("MountUsbFuseIfNeeded: EnsureMountPath failed path=%{public}s err=%{public}d", mountPath.c_str(), err);
        return err;
    }

    int32_t fuseFd = -1;
    err = StorageDaemonAdapter::GetInstance().OpenFuseDevice(fuseFd);
    if (err != ERR_OK) {
        LOGE("MountUsbFuseIfNeeded: OpenFuseDevice failed err=%{public}d", err);
        (void)StorageDaemonAdapter::GetInstance().RemoveMountPath(mountPath);
        return err;
    }

    const std::string options = BuildUsbFuseOptions(fuseFd);
    err = StorageDaemonAdapter::GetInstance().MountFuseDevice(fuseFd, mountPath, fsUuid, options);
    if (err != ERR_OK) {
        LOGE("MountUsbFuseIfNeeded: MountFuseDevice failed err=%{public}d", err);
        (void)StorageDaemonAdapter::GetInstance().RemoveMountPath(mountPath);
        return err;
    }

    err = UsbFuseAdapter::GetInstance().NotifyUsbFuseMount(fuseFd, volumeId, fsUuid);
    if (err != DiskManagerErrNo::E_OK) {
        LOGE("MountUsbFuseIfNeeded: NotifyUsbFuseMount failed err=%{public}d", err);
        return err;
    }
    return DiskManagerErrNo::E_OK;
}

DiskManager::DiskManager() = default;

DiskManager::~DiskManager() = default;

DiskManager &DiskManager::GetInstance()
{
    static DiskManager instance;
    return instance;
}

int32_t DiskManager::Mount(const std::string &volumeId)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;

    if ((volExternal.GetState() != VolumeState::UNMOUNTED) && (volExternal.GetState() != VolumeState::DECRYPTING)) {
        LOGE("Mount: volumeId=%{public}s state=%{public}d not allowed", volumeId.c_str(), volExternal.GetState());
        return E_VOL_MOUNT_ERR;
    }

    const std::string fsType = volExternal.GetFsTypeString();
    std::string fsUuid;
    int32_t uuidErr = EnsureFsUuidReady(volExternal, fsUuid);
    if (uuidErr != DiskManagerErrNo::E_OK) {
        volExternal.SetState(VolumeState::UNMOUNTED);
        return uuidErr;
    }
    if (!IsSafeFsUuid(fsUuid)) {
        LOGE("Mount: invalid fsUuid for volumeId=%{public}s", volumeId.c_str());
        volExternal.SetState(VolumeState::UNMOUNTED);
        return E_PARAMS_INVALID;
    }
    const std::string mountPath = "/mnt/data/external/" + fsUuid;

    if (volExternal.GetFsType() != FsType::MTP) {
        int32_t checkErr =
            StorageDaemonAdapter::GetInstance().Check("/dev/block/" + volExternal.GetId(), fsType, false);
        if (checkErr != ERR_OK) {
            LOGE("Mount: Check failed volId=%{public}s err=%{public}d", volumeId.c_str(), checkErr);
            volExternal.SetState(VolumeState::UNMOUNTED);
            return checkErr;
        }
    }

    const int32_t fuseErr = MountUsbFuseIfNeeded(volumeId, volExternal, fsType);
    if (fuseErr != DiskManagerErrNo::E_OK) {
        volExternal.SetState(VolumeState::UNMOUNTED);
        return fuseErr;
    }

    int32_t err = StorageDaemonAdapter::GetInstance().Mount("/dev/block/" + volExternal.GetId(), mountPath, fsType, "");
    if (err != ERR_OK) {
        LOGE("MountFs vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    volExternal.SetState(MOUNTED);
    CommonEventPublisher::PublishVolumeChange(MOUNTED, volExternal);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::Unmount(const std::string &volumeId)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;

    if (volExternal.GetState() != VolumeState::MOUNTED && volExternal.GetState() != VolumeState::DAMAGED_MOUNTED &&
        volExternal.GetState() != VolumeState::ENCRYPTED_AND_LOCKED &&
        volExternal.GetState() != VolumeState::ENCRYPTED_AND_UNLOCKED) {
        LOGE("Unmount: volumeId=%{public}s state=%{public}d not allowed", volumeId.c_str(), volExternal.GetState());
        return E_VOL_UMOUNT_ERR;
    }

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

int32_t DiskManager::Format(const std::string &volumeId, const std::string &fsType)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;

    if (volExternal.GetFsType() == FsType::MTP) {
        LOGE("MTP device not support to format.");
        return E_NOT_SUPPORT;
    }
    if (volExternal.GetState() != VolumeState::UNMOUNTED) {
        LOGE("Format: volumeId=%{public}s state=%{public}d not unmounted", volumeId.c_str(), volExternal.GetState());
        return E_VOL_STATE;
    }

    int32_t err = StorageDaemonAdapter::GetInstance().FormatVolume("/dev/block/" + volExternal.GetId(), fsType);
    if (err != ERR_OK) {
        LOGE("Format vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    if (UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType(volExternal.GetFsTypeString())) {
        (void)UsbFuseAdapter::GetInstance().NotifyUsbFuseUmount(volumeId);
    }
    volExternal.SetState(UNMOUNTED);
    volExternal.SetFsType(volExternal.GetFsTypeByStr(fsType));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::TryToFix(const std::string &volumeId)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("TryToFix: volumeId=%{public}s not found", volumeId.c_str());
        return E_NON_EXIST;
    }
    VolumeExternal &volExternal = it->second;

    if (volExternal.GetState() == VolumeState::DAMAGED_MOUNTED || volExternal.GetState() == VolumeState::MOUNTED) {
        const int32_t umErr = StorageDaemonAdapter::GetInstance().Unmount("/mnt/data/external/" + volExternal.GetUuid(),
                                                                          volExternal.GetFsTypeString(), true);
        if (umErr != ERR_OK) {
            LOGE("TryToFix: Unmount failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), umErr);
            return umErr;
        }
        volExternal.SetState(UNMOUNTED);
    }

    const std::string devPath = "/dev/block/" + volExternal.GetId();
    const std::string fsType = volExternal.GetFsTypeString();
    int32_t fixErr = StorageDaemonAdapter::GetInstance().Repair(devPath, fsType);
    if (fixErr != ERR_OK) {
        LOGE("TryToFix: Repair failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), fixErr);
        return fixErr;
    }

    int32_t checkErr = StorageDaemonAdapter::GetInstance().Check(devPath, fsType, false);
    if (checkErr != ERR_OK) {
        LOGE("TryToFix: Check failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), checkErr);
        return checkErr;
    }

    return Mount(volumeId);
}

int32_t DiskManager::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
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
    volExternal.SetDescription(description);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::Partition(const std::string &diskId, int32_t type)
{
    static constexpr const char *undefinedFsType = "undefined";
    if (UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType(undefinedFsType)) {
        LOGE("Partition: diskId=%{public}s is fuse, not support", diskId.c_str());
        return E_NOT_SUPPORT;
    }
    return StorageDaemonAdapter::GetInstance().Partition(diskId, type, 0);
}

int32_t DiskManager::OnDiskCreated(const Disk &disk)
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

int32_t DiskManager::UpsertDiskSnapshot(const DiskStoreRecord &rec)
{
    (void)rec;
    return DiskManagerErrNo::E_OK;
}

bool DiskManager::HasDisk(const std::string &diskId)
{
    (void)diskId;
    return false;
}

int32_t DiskManager::OnDiskDestroyed(const std::string &diskId)
{
    std::lock_guard<std::mutex> lock(diskMapMutex_);
    if (diskMap_.find(diskId) == diskMap_.end()) {
        LOGE("DiskManager::OnDiskDestroyed the disk %{public}s doesn't exist", diskId.c_str());
        return E_DISK_NOT_FOUND;
    }
    diskMap_.erase(diskId);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeCreated(const VolumeExternal &volExternal)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    volumeMap_.insert(make_pair(volExternal.GetId(), volExternal));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeMounted(const std::string &volumeId, const std::string &mountPath, int32_t state)
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

int32_t DiskManager::OnVolumeStateChanged(const std::string &volumeId, uint32_t state)
{
    if (state == VolumeState::FUSE_REMOVED) {
        (void)UsbFuseAdapter::GetInstance().NotifyUsbFuseUmount(volumeId);
        return DiskManagerErrNo::E_OK;
    }

    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("OnVolumeStateChanged: volumeId=%{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;
    volExternal.SetState(static_cast<int32_t>(state));
    CommonEventPublisher::PublishVolumeChange(static_cast<VolumeState>(state), volExternal);
    if (state == VolumeState::REMOVED || state == VolumeState::BAD_REMOVAL) {
        volumeMap_.erase(volumeId);
    }
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeDestroyed(const std::string &volumeId)
{
    std::lock_guard<std::mutex> lock(volumeMapMutex_);
    if (volumeMap_.find(volumeId) == volumeMap_.end()) {
        LOGE("DiskManager::OnVolumeDestroyed the vol %{public}s doesn't exist", volumeId.c_str());
        return E_VOLUME_NOT_FOUND;
    }
    volumeMap_.erase(volumeId);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeDamaged(const VolumeInfoStr &vis)
{
    (void)vis;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::ReplacePartitionsForDisk(const std::string &diskId, const std::vector<PartitionRecord> &partitions)
{
    (void)diskId;
    (void)partitions;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetAllDisks(std::vector<Disk> &out)
{
    out.clear();
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetDiskById(const std::string &diskId, Disk &out)
{
    std::lock_guard<std::mutex> lock(diskMapMutex_);
    if (diskMap_.find(diskId) == diskMap_.end()) {
        LOGE("DiskManager::GetDiskById id %{public}s not exists", diskId.c_str());
        return E_DISK_NOT_FOUND;
    }
    out = diskMap_[diskId];
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetAllVolumes(std::vector<VolumeExternal> &out)
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

int32_t DiskManager::GetVolumeById(const std::string &volumeId, VolumeExternal &out)
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

int32_t DiskManager::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &out)
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

int32_t DiskManager::UpdateVolumeMetadata(const std::string &volumeId,
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

int32_t DiskManager::GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize)
{
    freeSize = 0;
    std::string path = GetVolumePath(volumeUuid);
    LOGI("GetFreeSizeOfVolume path is %{public}s", path.c_str());
    if (path == "") {
        return DiskManagerErrNo::E_NON_EXIST;
    }
    struct statvfs diskInfo {};
    int ret = statvfs(path.c_str(), &diskInfo);
    if (ret != DiskManagerErrNo::E_OK) {
        return DiskManagerErrNo::E_STATVFS;
    }
    if (IsOddDevice(volumeUuid)) {
        int64_t totalSize = 0;
        int64_t startTotalSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_blocks);
        int64_t startFreeSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_bfree);
        const int32_t oddRet = GetOddSize(volumeUuid, totalSize, freeSize);
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

int32_t DiskManager::GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize)
{
    totalSize = 0;
    std::string path = GetVolumePath(volumeUuid);
    if (path == "") {
        return DiskManagerErrNo::E_NON_EXIST;
    }
    struct statvfs diskInfo {};
    int ret = statvfs(path.c_str(), &diskInfo);
    if (ret != DiskManagerErrNo::E_OK) {
        return DiskManagerErrNo::E_STATVFS;
    }
    if (IsOddDevice(volumeUuid)) {
        int64_t freeSize = 0;
        (void)GetOddSize(volumeUuid, totalSize, freeSize);
        return DiskManagerErrNo::E_OK;
    }
    totalSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_blocks);
    return DiskManagerErrNo::E_OK;
}
} // namespace DiskManager
} // namespace OHOS
