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
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <sys/mount.h>
#include <sys/statvfs.h>

#include "parameter.h"

namespace OHOS {
namespace DiskManager {

namespace {
constexpr const char *EXTERNAL_MOUNT_ROOT = "/mnt/data/external/";
constexpr const char *EXTERNAL_FUSE_DATA_ROOT = "/mnt/data/external_fuse/";
constexpr const char *FUSE_UMOUNT_FS_TYPE = "fuse";
/** SSD/HDD 上 f2fs/hmfs 卷的统一挂载前缀，后缀为从 1 递增的序号：/mnt/data/voldata/data1 … */
constexpr const char *VOLDATA_MOUNT_PREFIX = "/mnt/data/voldata/data";

constexpr int32_t TRUE_LEN = 5;
constexpr int32_t RD_ENABLE_LENGTH = 255;
constexpr int DISK_MMC_MAJOR = 179;
constexpr int DISK_CD_MAJOR = 11;
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

bool UiTypeIsInternalSsdOrHdd(const std::string &ui)
{
    return ui == "SSD" || ui == "HDD";
}

/** useFuseData 分支下不套用 USB 只读持久化参数（与原先 MountVolumeFilesystemLocked 语义一致）。 */
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
} // namespace

bool DiskManager::EffectiveUsbStackForVolumeDiskUnlocked(const std::string &diskId,
                                                         const std::string &fsTypeRaw) const
{
    if (!UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType(fsTypeRaw)) {
        return false;
    }
    auto dit = diskMap_.find(diskId);
    if (dit == diskMap_.end()) {
        return true;
    }
    return !UiTypeIsInternalSsdOrHdd(dit->second.GetUiType());
}

bool DiskManager::ShouldUseVoldataMountPathForDiskUnlocked(const std::string &diskId,
                                                           const std::string &fsNormLower) const
{
    auto dit = diskMap_.find(diskId);
    if (dit == diskMap_.end()) {
        return false;
    }
    if (!UiTypeIsInternalSsdOrHdd(dit->second.GetUiType())) {
        return false;
    }
    return fsNormLower == "f2fs" || fsNormLower == "hmfs";
}

bool DiskManager::IsSafeFsUuid(const std::string &fsUuid)
{
    return !(fsUuid.find("..") != std::string::npos || fsUuid.find('/') != std::string::npos);
}

int32_t DiskManager::LookupVolumeByUuidUnlocked(const std::string &fsUuid, VolumeExternal &out)
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
    std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
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
                                          VolumeExternal &volExternal,
                                          const std::string &fsType,
                                          bool useEnterpriseFuseStack)
{
    if (!useEnterpriseFuseStack) {
        return DiskManagerErrNo::E_OK;
    }
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

    const std::string mountPath = std::string(EXTERNAL_MOUNT_ROOT) + fsUuid;
    if (IsPathMounted(mountPath)) {
        return DiskManagerErrNo::E_OK;
    }

    int32_t fuseFd = -1;
    err = StorageDaemonAdapter::GetInstance().MountFuseDevice(mountPath, fuseFd);
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
    if (EffectiveUsbStackForVolumeDiskUnlocked(volExternal.GetDiskId(), fsType)) {
        const std::string dataPath = std::string(EXTERNAL_FUSE_DATA_ROOT) + uuid;
        int32_t err = StorageDaemonAdapter::GetInstance().Unmount(dataPath, fsType, force);
        if (err != ERR_OK) {
            return err;
        }
        const std::string fusePath = std::string(EXTERNAL_MOUNT_ROOT) + uuid;
        err = StorageDaemonAdapter::GetInstance().Unmount(fusePath, FUSE_UMOUNT_FS_TYPE, force);
        if (err != ERR_OK) {
            return err;
        }
        return UsbFuseAdapter::GetInstance().NotifyUsbFuseUmount(volExternal.GetId());
    }
    const std::string mountedPath = volExternal.GetPath();
    if (!mountedPath.empty()) {
        return StorageDaemonAdapter::GetInstance().Unmount(mountedPath, fsType, force);
    }
    return StorageDaemonAdapter::GetInstance().Unmount(std::string(EXTERNAL_MOUNT_ROOT) + uuid, fsType, force);
}

DiskManager::DiskManager() = default;

DiskManager::~DiskManager() = default;

DiskManager &DiskManager::GetInstance()
{
    static DiskManager instance;
    return instance;
}

uint32_t DiskManager::AllocateVoldataMountIndexLocked()
{
    std::set<uint32_t> used;
    const std::string prefix(VOLDATA_MOUNT_PREFIX);
    for (const auto &kv : volumeMap_) {
        const std::string &p = kv.second.GetPath();
        if (p.size() <= prefix.size()) {
            continue;
        }
        if (p.compare(0, prefix.size(), prefix) != 0) {
            continue;
        }
        bool allDigit = true;
        for (size_t i = prefix.size(); i < p.size(); ++i) {
            if (!std::isdigit(static_cast<unsigned char>(p[i]))) {
                allDigit = false;
                break;
            }
        }
        if (!allDigit || p.size() == prefix.size()) {
            continue;
        }
        errno = 0;
        char *end = nullptr;
        const unsigned long parsed = std::strtoul(p.c_str() + prefix.size(), &end, 10); // NOLINT(cert-str34-c)
        if (errno != 0 || end == p.c_str() + prefix.size()) {
            continue;
        }
        if (parsed >= 1UL && parsed <= static_cast<unsigned long>(UINT32_MAX)) {
            used.insert(static_cast<uint32_t>(parsed));
        }
    }
    uint32_t idx = 1;
    while (used.count(idx) != 0U) {
        ++idx;
    }
    return idx;
}

int32_t DiskManager::Mount(const std::string &volumeId)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("Volume with id %{public}s not found", volumeId.c_str());
        return DiskManagerErrNo::E_VOLUME_NOT_FOUND;
    }
    return MountVolumeEntryUnlocked(it->second, volumeId);
}

int32_t DiskManager::MountVolumeEntryUnlocked(VolumeExternal &volExternal, const std::string &volumeId)
{
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

    if (volExternal.GetFsType() != FsType::MTP) {
        int32_t checkErr =
            StorageDaemonAdapter::GetInstance().Check("/dev/block/" + volExternal.GetId(), fsType, false);
        if (checkErr != ERR_OK) {
            LOGE("Mount: Check failed volId=%{public}s err=%{public}d", volumeId.c_str(), checkErr);
            volExternal.SetState(VolumeState::UNMOUNTED);
            return checkErr;
        }
    }

    const bool useEnterpriseFuse = EffectiveUsbStackForVolumeDiskUnlocked(volExternal.GetDiskId(), fsType);
    const int32_t fuseErr = MountUsbFuseIfNeeded(volumeId, volExternal, fsType, useEnterpriseFuse);
    if (fuseErr != DiskManagerErrNo::E_OK) {
        volExternal.SetState(VolumeState::UNMOUNTED);
        return fuseErr;
    }

    return MountVolumeFilesystemLocked(volExternal, fsType, fsUuid);
}

std::string DiskManager::BuildMountDataPathForFilesystemLocked(VolumeExternal &volExternal,
                                                               const std::string &fsUuid,
                                                               bool useFuseData,
                                                               const std::string &fsNormLower)
{
    const std::string &diskId = volExternal.GetDiskId();
    std::string dataMountPath;
    if (ShouldUseVoldataMountPathForDiskUnlocked(diskId, fsNormLower)) {
        const uint32_t slot = AllocateVoldataMountIndexLocked();
        dataMountPath = std::string(VOLDATA_MOUNT_PREFIX) + std::to_string(slot);
        LOGI("Mount path voldata: slot=%{public}u path=%{public}s vol=%{public}s", slot, dataMountPath.c_str(),
             volExternal.GetId().c_str());
    } else if (useFuseData) {
        dataMountPath = std::string(EXTERNAL_FUSE_DATA_ROOT) + fsUuid;
    } else {
        dataMountPath = std::string(EXTERNAL_MOUNT_ROOT) + fsUuid;
    }
    return dataMountPath;
}

int32_t DiskManager::MountVolumeFilesystemLocked(VolumeExternal &volExternal,
                                                 const std::string &fsType,
                                                 const std::string &fsUuid)
{
    const std::string fsNormLower = NormalizeFsTypeAsciiLower(fsType);
    const std::string &diskId = volExternal.GetDiskId();
    const bool useFuseData = EffectiveUsbStackForVolumeDiskUnlocked(diskId, fsType);
    std::string dataMountPath =
        BuildMountDataPathForFilesystemLocked(volExternal, fsUuid, useFuseData, fsNormLower);

    const uint32_t mountFlag = ReadPersistUsbReadonlyMountFlagBits(useFuseData);

    int32_t err = StorageDaemonAdapter::GetInstance().Mount("/dev/block/" + volExternal.GetId(), dataMountPath, fsType,
                                                            mountFlag);
    if (err != ERR_OK) {
        LOGE("MountFs vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
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
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
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

    int32_t err = UnmountVolumeMountPoints(volExternal, true);
    if (err != ERR_OK) {
        LOGE("Unmount vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    volExternal.SetPath("");
    volExternal.SetState(UNMOUNTED);
    CommonEventPublisher::PublishVolumeChange(UNMOUNTED, volExternal);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::Format(const std::string &volumeId, const std::string &fsType)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
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

    if (EffectiveUsbStackForVolumeDiskUnlocked(volExternal.GetDiskId(), volExternal.GetFsTypeString())) {
        const std::string &mountPath = volExternal.GetPath();
        if (!mountPath.empty() && IsPathMounted(mountPath)) {
            int32_t umErr =
                StorageDaemonAdapter::GetInstance().Unmount(mountPath, volExternal.GetFsTypeString(), true);
            if (umErr != ERR_OK) {
                LOGE("Format: usb fuse pre-unmount failed path=%{public}s err=%{public}d", mountPath.c_str(), umErr);
                return umErr;
            }
            volExternal.SetPath("");
        }
    }

    int32_t err = StorageDaemonAdapter::GetInstance().FormatVolume("/dev/block/" + volExternal.GetId(), fsType);
    if (err != ERR_OK) {
        LOGE("Format vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        return err;
    }
    if (EffectiveUsbStackForVolumeDiskUnlocked(volExternal.GetDiskId(), volExternal.GetFsTypeString())) {
        (void)UsbFuseAdapter::GetInstance().NotifyUsbFuseUmount(volumeId);
    }
    std::string uuid;
    std::string type;
    std::string label;
    StorageDaemonAdapter::GetInstance().ReadMetadata("/dev/block/" + volExternal.GetId(), uuid, type, label);
    volExternal.SetFsUuid(uuid);
    volExternal.SetDescription(label);
    volExternal.SetState(UNMOUNTED);
    volExternal.SetFsType(volExternal.GetFsTypeByStr(fsType));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::TryToFix(const std::string &volumeId)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        LOGE("TryToFix: volumeId=%{public}s not found", volumeId.c_str());
        return E_NON_EXIST;
    }
    VolumeExternal &volExternal = it->second;

    if (volExternal.GetState() == VolumeState::DAMAGED_MOUNTED || volExternal.GetState() == VolumeState::MOUNTED) {
        const int32_t umErr = UnmountVolumeMountPoints(volExternal, true);
        if (umErr != ERR_OK) {
            LOGE("TryToFix: Unmount failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), umErr);
            return umErr;
        }
        volExternal.SetPath("");
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

    return MountVolumeEntryUnlocked(volExternal, volumeId);
}

int32_t DiskManager::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
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
    Disk diskToStore = disk;
    if (diskToStore.GetExtInfo().empty()) {
        std::string blockInfos;
        const int32_t biErr =
            StorageDaemonAdapter::GetInstance().GetBlockInfoByType(diskToStore.GetDiskId(), blockInfos);
        if (biErr == ERR_OK && !blockInfos.empty()) {
            diskToStore.SetExtInfo(std::move(blockInfos));
        } else if (biErr != ERR_OK) {
            LOGW("OnDiskCreated GetBlockInfoByType diskId=%{public}s err=%{public}d", diskToStore.GetDiskId().c_str(),
                 biErr);
        }
    }

    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
    if (diskMap_.find(diskToStore.GetDiskId()) != diskMap_.end()) {
        LOGE("DiskManagerService::OnDiskCreated the disk %{public}s already exists", diskToStore.GetDiskId().c_str());
        return DiskManagerErrNo::E_DISK_HAS_EXIST;
    }
    diskMap_.insert(make_pair(diskToStore.GetDiskId(), diskToStore));
    LOGI("DiskManagerService::OnDiskCreated successfully recorded disk %{public}s", diskToStore.GetDiskId().c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::UpsertDiskSnapshot(const DiskStoreRecord &rec)
{
    (void)rec;
    return DiskManagerErrNo::E_OK;
}

bool DiskManager::HasDisk(const std::string &diskId)
{
    std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
    return diskMap_.find(diskId) != diskMap_.end();
}

int32_t DiskManager::OnDiskDestroyed(const std::string &diskId)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
    if (diskMap_.find(diskId) == diskMap_.end()) {
        LOGE("DiskManager::OnDiskDestroyed the disk %{public}s doesn't exist", diskId.c_str());
        return E_DISK_NOT_FOUND;
    }
    diskMap_.erase(diskId);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeCreated(const VolumeExternal &volExternal)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
    volumeMap_.insert(make_pair(volExternal.GetId(), volExternal));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeDestroyed(const std::string &volumeId)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
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
        std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
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
    std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
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
    std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
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
    std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
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
    std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
    return LookupVolumeByUuidUnlocked(fsUuid, out);
}

int32_t DiskManager::UpdateVolumeMetadata(const std::string &volumeId,
                                          const std::string &fsUuid,
                                          const std::string &fsTypeStr,
                                          const std::string &description)
{
    std::unique_lock<std::shared_mutex> writelock(mapsRwMutex_);
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
        std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
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
        std::shared_lock<std::shared_mutex> readlock(mapsRwMutex_);
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

} // namespace DiskManager
} // namespace OHOS
