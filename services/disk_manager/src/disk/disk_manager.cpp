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
#include "disk_manager.h"
#include "voldata_uuid_store.h"

#include "storage_daemon_adapter.h"
#include "usb_fuse_adapter.h"

#include "errors.h"
#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "disk_manager_napi_errno.h"
#include "notification/common_event_publisher.h"

#include <cctype>
#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>

#include "parameter.h"

namespace OHOS {
namespace DiskManager {

namespace {
constexpr const char *EXTERNAL_MOUNT_ROOT = "/mnt/data/external/";
constexpr const char *EXTERNAL_FUSE_DATA_ROOT = "/mnt/data/external_fuse/";
constexpr const char *FUSE_UMOUNT_FS_TYPE = "fuse";

constexpr int32_t TRUE_LEN = 5;
constexpr int32_t RD_ENABLE_LENGTH = 255;
constexpr int DISK_MMC_MAJOR = 179;
constexpr int DISK_CD_MAJOR = 11;
const int32_t MTP_DEVICE_NAME_LEN = 512;
constexpr uint32_t HMFS_FLAG = 0x8000;
constexpr const char *PERSIST_FILEMANAGEMENT_USB_READONLY = "persist.filemanagement.usb.readonly";

std::string NormalizeFsTypeAsciiLower(const std::string &fs)
{
    std::string out;
    out.reserve(fs.size());
    for (unsigned char ch : fs) {
        out += static_cast<char>(std::tolower(ch));
    }
    return out;
}

bool StartsWith(const std::string &value, const std::string &prefix)
{
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

/** useFuseData 分支下不套用 USB 只读持久化参数（与历史行为保持一致）。 */
uint32_t ReadPersistUsbReadonlyMountFlagBits(bool useFuseData)
{
    uint32_t mountFlag = 0;
    if (useFuseData) {
        return mountFlag;
    }
    int handle = static_cast<int>(FindParameter(PERSIST_FILEMANAGEMENT_USB_READONLY));
    if (handle == -1) {
        return mountFlag;
    }
    char rdOnlyEnable[RD_ENABLE_LENGTH] = {"false"};
    auto res = GetParameterValue(handle, rdOnlyEnable, RD_ENABLE_LENGTH);
    if (res >= 0 && strncmp(rdOnlyEnable, "true", TRUE_LEN) == 0) {
        mountFlag |= static_cast<uint32_t>(MS_RDONLY);
    } else {
        mountFlag &= ~static_cast<uint32_t>(MS_RDONLY);
    }
    return mountFlag;
}

void ApplyDefaultVolumeDescriptionIfUnset(VolumeExternal &volExternal, int32_t flag)
{
    if (!volExternal.GetDescription().empty()) {
        return;
    }
    std::string label;
    if (flag == SD_FLAG) {
        label = "MySDCard";
    } else if (flag == USB_FLAG) {
        label = "MyUSB";
    } else if (flag == CD_FLAG) {
        label = "MyDVD";
    } else {
        label = "Default";
    }
    volExternal.SetDescription(label);
}

static void AttachVolumeIdsToDisk(const std::map<std::string, VolumeExternal> &volumeMap, Disk &disk)
{
    std::vector<std::string> ids;
    for (const auto &kv : volumeMap) {
        if (kv.second.GetDiskId() == disk.GetDiskId()) {
            ids.push_back(kv.second.GetId());
        }
    }
    disk.SetVolumeIds(std::move(ids));
}

std::string ResolveVoldataMountPath(const VolumeExternal &volExternal,
                                    const std::string &fsUuid,
                                    bool *voldataMappingCreated)
{
    if (voldataMappingCreated != nullptr) {
        *voldataMappingCreated = false;
    }
    auto &mappingStore = VoldataUuidStore::GetInstance();
    if (mappingStore.Init() != DiskManagerErrNo::E_OK) {
        LOGE("Mount path voldata Init failed vol=%{public}s", volExternal.GetId().c_str());
        return "";
    }
    std::string dataMountPath;
    if (mappingStore.TryGetMountPath(fsUuid, dataMountPath)) {
        LOGI("Mount path voldata existing: path=%{public}s vol=%{public}s uuid=%{public}s",
             dataMountPath.c_str(), volExternal.GetId().c_str(), fsUuid.c_str());
        return dataMountPath;
    }
    bool created = false;
    const int32_t resolveErr = mappingStore.ResolveMountPath(fsUuid, dataMountPath, created);
    if (resolveErr != DiskManagerErrNo::E_OK) {
        LOGE("Mount path voldata ResolveMountPath failed vol=%{public}s err=%{public}d",
             volExternal.GetId().c_str(), resolveErr);
        return "";
    }
    if (voldataMappingCreated != nullptr) {
        *voldataMappingCreated = created;
    }
    LOGI("Mount path voldata new: path=%{public}s vol=%{public}s uuid=%{public}s created=%{public}d",
         dataMountPath.c_str(), volExternal.GetId().c_str(), fsUuid.c_str(), created ? 1 : 0);
    return dataMountPath;
}
} // namespace

bool DiskManager::ShouldUseVoldataMountPathForDiskUnlocked(const std::string &diskId,
                                                           const std::string &fsNormLower) const
{
    auto dit = diskMap_.find(diskId);
    if (dit == diskMap_.end()) {
        return false;
    }
    if (!dit->second.IsInternalDataDisk()) {
        return false;
    }
    if (fsNormLower != "hmfs") {
        return false;
    }

    uint32_t volumeCountOnDisk = 0;
    for (const auto &kv : volumeMap_) {
        if (kv.second.GetDiskId() != diskId) {
            continue;
        }
        ++volumeCountOnDisk;
        if (volumeCountOnDisk > 1U) {
            return false;
        }
    }
    return volumeCountOnDisk == 1U;
}

DiskManager::VolumeMountPolicy DiskManager::ComputeVolumeMountPolicy(const std::string &diskId,
                                                                     const std::string &fsType) const
{
    const std::string fsNormLower = NormalizeFsTypeAsciiLower(fsType);
    std::shared_lock<std::shared_mutex> diskLock(diskMapMutex_);
    std::shared_lock<std::shared_mutex> volLock(volumeMapMutex_);

    VolumeMountPolicy policy;
    policy.useVoldataPath = ShouldUseVoldataMountPathForDiskUnlocked(diskId, fsNormLower);
    bool isInternalDisk = false;
    const auto dit = diskMap_.find(diskId);
    if (dit != diskMap_.end()) {
        isInternalDisk = dit->second.IsInternalDataDisk();
    }
    const bool isDataFs = (fsNormLower == "f2fs" || fsNormLower == "hmfs");
    const bool bypassTobForPcNonDataFs = isInternalDisk && !isDataFs;
    policy.useFuseData = !policy.useVoldataPath && !bypassTobForPcNonDataFs &&
                         UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType(fsType);
    return policy;
}

bool DiskManager::IsSafeFsUuid(const std::string &fsUuid)
{
    return !(fsUuid.find("..") != std::string::npos || fsUuid.find('/') != std::string::npos);
}

int32_t DiskManager::LookupVolumeByUuidUnlocked(const std::string &fsUuid, VolumeExternal &out) const
{
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

std::string DiskManager::GetVolumePath(const std::string &volumeUuid)
{
    std::shared_lock<std::shared_mutex> readlock(volumeMapMutex_);
    VolumeExternal vol;
    if (LookupVolumeByUuidUnlocked(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
        LOGE("GetVolumePath fail.");
        return "";
    }
    return vol.GetPath();
}

bool DiskManager::IsOddFsType(const std::string &fsType)
{
    return fsType == "udf" || fsType == "iso9660";
}

int32_t DiskManager::GetOddCapacityAtMountPath(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize)
{
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
                                          const std::string &fsType,
                                          const std::string &fsUuid,
                                          bool useFuseData)
{
    if (!useFuseData) {
        return DiskManagerErrNo::E_OK;
    }
    if (!UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType(fsType)) {
        return DiskManagerErrNo::E_OK;
    }
    if (!IsSafeFsUuid(fsUuid)) {
        LOGE("MountUsbFuseIfNeeded: invalid fsUuid for volumeId=%{public}s", volumeId.c_str());
        return E_PARAMS_INVALID;
    }

    const std::string mountPath = std::string(EXTERNAL_MOUNT_ROOT) + fsUuid;
    if (IsPathMounted(mountPath)) {
        return DiskManagerErrNo::E_OK;
    }

    int32_t fuseFd = -1;
    int32_t err = StorageDaemonAdapter::GetInstance().MountFuseDevice(mountPath, fuseFd);
    if (err != ERR_OK) {
        LOGE("MountUsbFuseIfNeeded: MountFuseDevice failed err=%{public}d", err);
        return err;
    }
    err = UsbFuseAdapter::GetInstance().NotifyUsbFuseMount(fuseFd, volumeId, fsUuid);
    if (err != DiskManagerErrNo::E_OK) {
        LOGE("MountUsbFuseIfNeeded: NotifyUsbFuseMount failed err=%{public}d", err);
        return err;
    }
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::UnmountVolumeMountPoints(const VolumeExternal &volExternal, bool force)
{
    const std::string fsType = volExternal.GetFsTypeString();
    const std::string &uuid = volExternal.GetUuid();
    const std::string &volumeId = volExternal.GetId();
    const std::string mountedPath = volExternal.GetPath();
    if (!mountedPath.empty() && StartsWith(mountedPath, std::string(EXTERNAL_FUSE_DATA_ROOT))) {
        int32_t err = StorageDaemonAdapter::GetInstance().Unmount(mountedPath, fsType, force);
        if (err != ERR_OK) {
            return err;
        }
        err = StorageDaemonAdapter::GetInstance().Unmount(std::string(EXTERNAL_MOUNT_ROOT) + uuid, FUSE_UMOUNT_FS_TYPE,
                                                          force);
        if (err != ERR_OK) {
            return err;
        }
        return UsbFuseAdapter::GetInstance().NotifyUsbFuseUmount(volExternal.GetId());
    }
    if (!mountedPath.empty()) {
        return StorageDaemonAdapter::GetInstance().Unmount(mountedPath, fsType, force);
    }
    std::string mountPath;
    if (fsType == "mtp" || fsType == "ptp") {
        mountPath = std::string(EXTERNAL_MOUNT_ROOT) + volumeId;
    } else {
        mountPath = std::string(EXTERNAL_MOUNT_ROOT) + uuid;
    }
    LOGI("UnmountVolumeMountPoints, fsType=%{public}s, volumeId=%{public}s, mountPath=%{public}s", fsType.c_str(),
         volumeId.c_str(), mountPath.c_str());
    return StorageDaemonAdapter::GetInstance().Unmount(mountPath, fsType, force);
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
    VolumeExternal volExternal;
    {
        std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
        }
        if ((it->second.GetState() != VolumeState::UNMOUNTED) &&
            (it->second.GetState() != VolumeState::DECRYPTING)) {
            LOGE("Mount: volumeId=%{public}s state=%{public}d not allowed", volumeId.c_str(),
                 it->second.GetState());
            return E_VOL_MOUNT_ERR;
        }
        volExternal = it->second;
    }

    const int32_t mountErr = MountVolumeEntry(volExternal, volumeId);

    {
        std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
        }
        if (mountErr == DiskManagerErrNo::E_OK &&
            it->second.GetState() != VolumeState::UNMOUNTED && it->second.GetState() != VolumeState::DECRYPTING) {
            LOGE("Mount: volumeId=%{public}s state changed during mount", volumeId.c_str());
            return E_VOL_MOUNT_ERR;
        }
        it->second = volExternal;
    }
    return mountErr;
}

int32_t DiskManager::MountVolumeEntry(VolumeExternal &volExternal, const std::string &volumeId)
{
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

    if (volExternal.GetFsType() != FsType::MTP) {
        int32_t checkErr =
            StorageDaemonAdapter::GetInstance().Check("/dev/block/" + volExternal.GetId(), fsType, false);
        if (checkErr != ERR_OK) {
            LOGE("Mount: Check failed volId=%{public}s err=%{public}d", volumeId.c_str(), checkErr);
            volExternal.SetState(VolumeState::UNMOUNTED);
            return checkErr;
        }
    }

    int32_t mountErr = MountVolumeFilesystem(volExternal, fsType, fsUuid);
    if (mountErr != DiskManagerErrNo::E_OK) {
        volExternal.SetState(VolumeState::UNMOUNTED);
    }
    return mountErr;
}

std::string DiskManager::BuildMountDataPath(const MountDataPathParams &params)
{
    if (params.policy.useVoldataPath) {
        return ResolveVoldataMountPath(params.volExternal, params.fsUuid, params.voldataMappingCreated);
    }
    if (params.policy.useFuseData) {
        return std::string(EXTERNAL_FUSE_DATA_ROOT) + params.fsUuid;
    }
    return std::string(EXTERNAL_MOUNT_ROOT) + params.fsUuid;
}

int32_t DiskManager::MountVolumeFilesystem(VolumeExternal &volExternal,
                                           const std::string &fsType,
                                           const std::string &fsUuid)
{
    if ((fsType == "hmfs" || fsType == "f2fs") && !volExternal.GetUserData()) {
        LOGE("MountVolumeFilesystem fsType is %{public}s, but is not userdata", fsType.c_str());
        return DiskManagerErrNo::E_OTHER_MOUNT;
    }
    const VolumeMountPolicy policy = ComputeVolumeMountPolicy(volExternal.GetDiskId(), fsType);

    int32_t fuseErr = MountUsbFuseIfNeeded(volExternal.GetId(), fsType, fsUuid, policy.useFuseData);
    if (fuseErr != DiskManagerErrNo::E_OK) {
        return fuseErr;
    }

    bool voldataMappingCreated = false;
    const MountDataPathParams pathParams {volExternal, fsUuid, policy, &voldataMappingCreated};
    std::string dataMountPath = BuildMountDataPath(pathParams);
    if (dataMountPath.empty()) {
        volExternal.SetState(VolumeState::UNMOUNTED);
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    uint32_t mountFlag = ReadPersistUsbReadonlyMountFlagBits(policy.useFuseData);
    if ((fsType == "hmfs" || fsType == "f2fs") && volExternal.GetUserData()) {
        mountFlag = HMFS_FLAG;
    }

    int32_t err = StorageDaemonAdapter::GetInstance().Mount("/dev/block/" + volExternal.GetId(), dataMountPath, fsType,
                                                            mountFlag);
    if (err != ERR_OK) {
        LOGE("MountFs vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        if (policy.useVoldataPath && voldataMappingCreated) {
            (void)VoldataUuidStore::GetInstance().RemoveByFsUuid(fsUuid);
        }
        volExternal.SetState(VolumeState::UNMOUNTED);
        return err;
    }
    volExternal.SetPath(dataMountPath);
    volExternal.SetState(MOUNTED);
    const int32_t flag = GetFlagFromMajorInfo(volExternal.GetId());
    volExternal.SetFlags(flag);
    ApplyDefaultVolumeDescriptionIfUnset(volExternal, flag);
    CommonEventPublisher::PublishVolumeChange(MOUNTED, volExternal);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetFlagFromMajorInfo(const std::string &volumeId)
{
    size_t firstDash = volumeId.find('-');
    if (firstDash == std::string::npos) {
        LOGE("GetFlagFromMajorInfo can not find first split symbol");
        return 0;
    }

    size_t secondDash = volumeId.find('-', firstDash + 1);
    if (secondDash == std::string::npos) {
        LOGE("GetFlagFromMajorInfo canot find second split symbol");
        return 0;
    }

    std::string majorStr = volumeId.substr(firstDash + 1, secondDash - firstDash - 1);
    int32_t flag = 0;
    int32_t major = stoi(majorStr);
    LOGI("GetFlagFromMajorInfo major=%{public}d", major);
    if (major == DISK_MMC_MAJOR) {
        flag |= SD_FLAG;
    } else if (major == DISK_CD_MAJOR) {
        flag |= CD_FLAG;
    } else {
        flag |= USB_FLAG;
    }
    return flag;
}

int32_t DiskManager::Unmount(const std::string &volumeId)
{
    VolumeExternal volExternal;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
        }
        if (it->second.GetState() != VolumeState::MOUNTED &&
            it->second.GetState() != VolumeState::DAMAGED_MOUNTED &&
            it->second.GetState() != VolumeState::ENCRYPTED_AND_LOCKED &&
            it->second.GetState() != VolumeState::ENCRYPTED_AND_UNLOCKED) {
            LOGE("Unmount: volumeId=%{public}s state=%{public}d not allowed", volumeId.c_str(),
                 it->second.GetState());
            return E_VOL_UMOUNT_ERR;
        }
        volExternal = it->second;
    }

    const int32_t err = UnmountVolumeMountPoints(volExternal, true);
    if (err != ERR_OK) {
        LOGE("Unmount vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }

    volExternal.SetPath("");
    volExternal.SetState(UNMOUNTED);
    CommonEventPublisher::PublishVolumeChange(UNMOUNTED, volExternal);

    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    it->second.SetPath("");
    it->second.SetState(UNMOUNTED);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::Format(const std::string &volumeId, const std::string &fsType)
{
    std::string blockVolId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
        }
        if (it->second.GetFsType() == FsType::MTP) {
            LOGE("MTP device not support to format.");
            return E_NOT_SUPPORT;
        }
        if (it->second.GetState() != VolumeState::UNMOUNTED) {
            LOGE("Format: volumeId=%{public}s state=%{public}d not unmounted", volumeId.c_str(),
                 it->second.GetState());
            return E_VOL_STATE;
        }
        blockVolId = it->second.GetId();
    }

    int32_t err = StorageDaemonAdapter::GetInstance().FormatVolume("/dev/block/" + blockVolId, fsType);
    if (err != ERR_OK) {
        LOGE("Format vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }
    std::string uuid;
    std::string type;
    std::string label;
    StorageDaemonAdapter::GetInstance().ReadMetadata("/dev/block/" + blockVolId, uuid, type, label);

    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    VolumeExternal &volExternal = it->second;
    volExternal.SetFsUuid(uuid);
    volExternal.SetDescription(label);
    volExternal.SetState(UNMOUNTED);
    volExternal.SetFsType(volExternal.GetFsTypeByStr(fsType));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::TryToFix(const std::string &volumeId)
{
    VolumeExternal volExternal;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("TryToFix: volumeId=%{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
        }
        volExternal = it->second;
    }

    if (volExternal.GetState() == VolumeState::DAMAGED_MOUNTED || volExternal.GetState() == VolumeState::MOUNTED) {
        const int32_t umErr = UnmountVolumeMountPoints(volExternal, true);
        if (umErr != ERR_OK) {
            LOGE("TryToFix: Unmount failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), umErr);
            return umErr;
        }
        volExternal.SetPath("");
        volExternal.SetState(UNMOUNTED);
        std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it != volumeMap_.end()) {
            it->second.SetPath("");
            it->second.SetState(UNMOUNTED);
        }
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

    const int32_t mountErr = MountVolumeEntry(volExternal, volumeId);
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        return E_NON_EXIST;
    }
    it->second = volExternal;
    return mountErr;
}

int32_t DiskManager::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    std::string volumeId;
    std::string blockVolId;
    std::string fsTypeStr;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = std::find_if(volumeMap_.begin(), volumeMap_.end(),
                                     [&fsUuid](const auto &pair) { return pair.second.GetUuid() == fsUuid; });
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", fsUuid.c_str());
            return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
        }
        volumeId = it->first;
        blockVolId = it->second.GetId();
        fsTypeStr = it->second.GetFsTypeString();
    }

    const int32_t err =
        StorageDaemonAdapter::GetInstance().SetLabel("/dev/block/" + blockVolId, fsTypeStr, description);
    if (err != ERR_OK) {
        LOGE("SetLabel vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }

    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    it->second.SetState(UNMOUNTED);
    it->second.SetDescription(description);
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
    Disk diskToStore = disk;
    BlockInfo blockInfo {};
    const bool hasBlockInfo = BlockInfoTable::GetInstance().TryCopyByDiskId(diskToStore.GetDiskId(), blockInfo);
    if (diskToStore.GetExtraInfo().empty() && hasBlockInfo) {
        diskToStore.SetExtraInfo(BlockInfoTable::ToJsonStringWithExtras(blockInfo));
    } else if (diskToStore.GetExtraInfo().empty()) {
        LOGW("OnDiskCreated block info cache miss diskId=%{public}s", diskToStore.GetDiskId().c_str());
    }

    std::unique_lock<std::shared_mutex> diskWriteLock(diskMapMutex_);
    if (diskMap_.find(diskToStore.GetDiskId()) != diskMap_.end()) {
        LOGE("DiskManagerService::OnDiskCreated the disk %{public}s already exists", diskToStore.GetDiskId().c_str());
        return DiskManagerErrNo::E_DISK_HAS_EXIST;
    }
    diskMap_.insert(make_pair(diskToStore.GetDiskId(), diskToStore));
    LOGI("DiskManagerService::OnDiskCreated successfully recorded disk %{public}s", diskToStore.GetDiskId().c_str());
    return DiskManagerErrNo::E_OK;
}

bool DiskManager::HasDisk(const std::string &diskId)
{
    std::shared_lock<std::shared_mutex> diskReadLock(diskMapMutex_);
    return diskMap_.find(diskId) != diskMap_.end();
}

int32_t DiskManager::OnDiskDestroyed(const std::string &diskId)
{
    std::unique_lock<std::shared_mutex> diskWriteLock(diskMapMutex_);
    if (diskMap_.find(diskId) == diskMap_.end()) {
        LOGE("DiskManager::OnDiskDestroyed the disk %{public}s doesn't exist", diskId.c_str());
        return E_DISK_NOT_FOUND;
    }
    diskMap_.erase(diskId);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeCreated(const VolumeExternal &volExternal)
{
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    volumeMap_.insert(make_pair(volExternal.GetId(), volExternal));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeDestroyed(const std::string &volumeId)
{
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    if (volumeMap_.find(volumeId) == volumeMap_.end()) {
        LOGE("DiskManager::OnVolumeDestroyed the vol %{public}s doesn't exist", volumeId.c_str());
        return E_VOLUME_NOT_FOUND;
    }
    volumeMap_.erase(volumeId);
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
    std::vector<Disk> snaps;
    {
        std::shared_lock<std::shared_mutex> diskReadLock(diskMapMutex_);
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        for (auto &kv : diskMap_) {
            snaps.push_back(kv.second);
        }
        for (auto &disk : snaps) {
            AttachVolumeIdsToDisk(volumeMap_, disk);
        }
    }
    out = std::move(snaps);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetDiskById(const std::string &diskId, Disk &out)
{
    std::shared_lock<std::shared_mutex> diskReadLock(diskMapMutex_);
    std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
    if (diskMap_.find(diskId) == diskMap_.end()) {
        LOGE("DiskManager::GetDiskById id %{public}s not exists", diskId.c_str());
        return E_DISK_NOT_FOUND;
    }
    out = diskMap_[diskId];
    AttachVolumeIdsToDisk(volumeMap_, out);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetAllVolumes(std::vector<VolumeExternal> &out)
{
    std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
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
    std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    LOGI("VolumeManagerService::GetVolumeByUuid volumeUuid %{public}s exists", volumeId.c_str());
    out = it->second;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &out)
{
    std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
    return LookupVolumeByUuidUnlocked(fsUuid, out);
}

int32_t DiskManager::UpdateVolumeMetadata(const std::string &volumeId,
                                          const std::string &fsUuid,
                                          const std::string &fsTypeStr,
                                          const std::string &description)
{
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
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
    std::string path;
    std::string fsType;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        VolumeExternal vol;
        if (LookupVolumeByUuidUnlocked(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
            return DiskManagerErrNo::E_NON_EXIST;
        }
        path = vol.GetPath();
        fsType = vol.GetFsTypeString();
        LOGI("GetFreeSizeOfVolume path is %{public}s", path.c_str());
    }
    if (path.empty()) {
        return DiskManagerErrNo::E_NON_EXIST;
    }
    struct statvfs diskInfo {};
    const int ret = statvfs(path.c_str(), &diskInfo);
    if (ret != DiskManagerErrNo::E_OK) {
        return DiskManagerErrNo::E_STATVFS;
    }
    if (IsOddFsType(fsType)) {
        int64_t totalSize = 0;
        int64_t startTotalSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_blocks);
        int64_t startFreeSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_bfree);
        const int32_t oddRet = GetOddCapacityAtMountPath(path, totalSize, freeSize);
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
    std::string path;
    std::string fsType;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        VolumeExternal vol;
        if (LookupVolumeByUuidUnlocked(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
            return DiskManagerErrNo::E_NON_EXIST;
        }
        path = vol.GetPath();
        fsType = vol.GetFsTypeString();
    }
    if (path.empty()) {
        return DiskManagerErrNo::E_NON_EXIST;
    }
    struct statvfs diskInfo {};
    const int ret = statvfs(path.c_str(), &diskInfo);
    if (ret != DiskManagerErrNo::E_OK) {
        return DiskManagerErrNo::E_STATVFS;
    }
    if (IsOddFsType(fsType)) {
        int64_t freeSize = 0;
        (void)GetOddCapacityAtMountPath(path, totalSize, freeSize);
        return DiskManagerErrNo::E_OK;
    }
    totalSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_blocks);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::EraseVolume(const std::string &volumeId)
{
    (void)volumeId;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::EjectVolume(const std::string &volumeId)
{
    (void)volumeId;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::CreateIsoImage(const std::string &volumeId, const std::string &filePath)
{
    (void)volumeId;
    (void)filePath;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::BurnVolume(const std::string &volumeId, const std::string &burnOptions)
{
    (void)volumeId;
    (void)burnOptions;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetVolumeOpProcess(const std::string &volumeId, int32_t &progressPct)
{
    (void)volumeId;
    progressPct = 0;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::VerifyBurnData(const std::string &volumeId, int32_t verifyType)
{
    (void)volumeId;
    (void)verifyType;
    return DiskManagerErrNo::E_OK;
}

void DiskManager::NotifyMtpMounted(const std::string &id,
                                   const std::string &path,
                                   const std::string &desc,
                                   const std::string &uuid,
                                   const std::string &fsType)
{
    LOGI("DiskManager NotifyMtpMounted");
    std::string key = "user.getfriendlyname";
    char *value = (char *)malloc(sizeof(char) * (MTP_DEVICE_NAME_LEN + 1));
    int32_t len = 0;
    if (value != nullptr) {
        len = getxattr(path.c_str(), key.c_str(), value, MTP_DEVICE_NAME_LEN);
        if (len >= 0 && len <= MTP_DEVICE_NAME_LEN) {
            value[len] = '\0';
            LOGI("MTP get namelen=%{public}d, name=%{public}s", len, value);
        }
    }

    VolumeExternal volExternal(VolumeCore(id, 0, ""));
    volExternal.SetPath(path);
    if (fsType == "mtpfs") {
        volExternal.SetFsType(volExternal.GetFsTypeByStr("mtp"));
    } else if (fsType == "gphotofs") {
        volExternal.SetFsType(volExternal.GetFsTypeByStr("ptp"));
    } else {
        LOGI("Unknown type:%{public}s", fsType.c_str());
    }
    volExternal.SetDescription(desc);
    if (len > 0) {
        LOGI("set MTP device name:%{public}s", value);
        volExternal.SetDescription(std::string(value));
    }
    if (value != nullptr) {
        free(value);
        value = nullptr;
    }
    volExternal.SetState(MOUNTED);
    volExternal.SetFsUuid(uuid);
    {
        std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
        volumeMap_.insert(make_pair(volExternal.GetId(), volExternal));
    }
    CommonEventPublisher::PublishVolumeChange(VolumeState::MOUNTED, volExternal);
}

void DiskManager::NotifyMtpUnmounted(const std::string &id, const bool isBadRemove)
{
    LOGI("DiskManager NotifyMtpUnmounted");
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    auto it = volumeMap_.find(id);
    if (it == volumeMap_.end()) {
        LOGE("DiskManager::Unmount id %{public}s not exists", id.c_str());
        return;
    }
    VolumeExternal &volExternal = it->second;
    if (!isBadRemove) {
        CommonEventPublisher::PublishVolumeChange(VolumeState::UNMOUNTED, volExternal);
    } else {
        CommonEventPublisher::PublishVolumeChange(VolumeState::BAD_REMOVAL, volExternal);
    }
    volumeMap_.erase(id);
}
} // namespace DiskManager
} // namespace OHOS
