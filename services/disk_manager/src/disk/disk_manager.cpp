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

#include "accesstoken_kit.h"
#include "block_info_table.h"
#include "disk_manager.h"
#include "event_info.h"
#include "ipc_skeleton.h"
#include "partition_table_parser.h"
#include "sg_collect_client.h"
#include "uevent_bootstrap.h"
#include "voldata_uuid_store.h"

#include "storage_daemon_adapter.h"
#include "usb_fuse_adapter.h"

#include "errors.h"
#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "disk_manager_napi_errno.h"
#include "notification/common_event_publisher.h"

#include <nlohmann/json.hpp>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <regex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>

#include "parameter.h"

namespace OHOS {
namespace DiskManager {
using json = nlohmann::json;

namespace {
constexpr const char *EXTERNAL_MOUNT_ROOT = "/mnt/data/external/";
constexpr const char *EXTERNAL_FUSE_DATA_ROOT = "/mnt/data/external_fuse/";
constexpr const char *EXTERNAL_DVR_ROOT = "/mnt/data/dvr/";
constexpr const char *FUSE_UMOUNT_FS_TYPE = "fuse";
/** SSD/HDD 上 f2fs 分区挂载至 /mnt/data/voldata/dataX 时的 SELinux context。 */
constexpr const char *VOLDATA_MOUNT_SELINUX_CONTEXT = "context=u:object_r:mnt_external_file:s0";
constexpr const char *DEV_BLOCK_PREFIX = "/dev/block/";
constexpr const char *PERSIST_ENTERPRISE_SPACE_ENABLE = "persist.space_mgr_space.enterprise_space_enable";
constexpr int64_t BURN_REPORT_EVENT_ID = 0x30000101;
constexpr const char *BURN_REPORT_VERSION = "1.0";

int32_t ReportBurnSecurityInfo(int32_t userId, const std::string &appId, const std::string &fsType)
{
    LOGI("ReportBurnSecurityInfo start");
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    int64_t happenTime = ms.count();

    json contentJson;
    contentJson["userId"] = userId;
    contentJson["appId"] = appId;
    contentJson["burningType"] = fsType;
    contentJson["happenTime"] = happenTime;

    std::string content = contentJson.dump();

    OHOS::Security::SecurityGuard::EventInfo eventInfo(BURN_REPORT_EVENT_ID, BURN_REPORT_VERSION, content);
    LOGI("ReportBurnSecurityInfo: eventId=%{public}" PRId64 " version=%{public}s content=%{public}s",
        BURN_REPORT_EVENT_ID, BURN_REPORT_VERSION, content.c_str());
    auto wrappedInfo = std::make_shared<OHOS::Security::SecurityGuard::EventInfo>(eventInfo);
    OHOS::Security::SecurityGuard::NativeDataCollectKit nativeDataCollectKit;
    int32_t reportResult = nativeDataCollectKit.ReportSecurityInfo(wrappedInfo);
    LOGI("ReportBurnSecurityInfo result: reportResult=%{public}d", reportResult);
    return reportResult;
}

std::string NormalizeDiskBlockPath(const std::string &diskPath)
{
    if (diskPath.size() > std::char_traits<char>::length(DEV_BLOCK_PREFIX) &&
        diskPath.compare(0, std::char_traits<char>::length(DEV_BLOCK_PREFIX), DEV_BLOCK_PREFIX) == 0) {
        return diskPath;
    }
    return std::string(DEV_BLOCK_PREFIX) + diskPath;
}

constexpr int32_t TRUE_LEN = 5;
constexpr int32_t RD_ENABLE_LENGTH = 255;
const int32_t MTP_DEVICE_NAME_LEN = 512;
constexpr uint64_t HMFS_FLAG = 0x8000;
constexpr uint32_t BASE_DECIMAL = 10;
constexpr int64_t VFAT_TYPECODE_MIN_SIZE = 16 * 1024 * 1024;
constexpr int64_t EXFAT_TYPECODE_MIN_SIZE = 32 * 1024 * 1024;
constexpr int64_t NTFS_TYPECODE_MIN_SIZE = 4 * 1024 * 1024;
constexpr int64_t EXT4_TYPECODE_MIN_SIZE = 16 * 1024 * 1024;
constexpr int64_t F2FS_TYPECODE_MIN_SIZE = 16 * 1024 * 1024;
constexpr int32_t WAIT_UEVENT_TIMEOUT = 1 * 60 * 1000;
constexpr const char *PERSIST_FILEMANAGEMENT_USB_READONLY = "persist.filemanagement.usb.readonly";
const std::map<std::string, std::string> typeCodeMap_ = {
    {"vfat", "0x0700"},
    {"exfat", "0x0700"},
    {"ntfs", "0x0700"},
    {"ext4", "0x8300"},
    {"f2fs", "0x8300"},
    {"hmfs", "0x8300"},
};

const std::set<std::string> LABEL_SUPPORTED_FS_TYPES = {
    "ntfs",
    "exfat",
    "hmfs",
    "f2fs"
};

bool ConvertStringToInt(const std::string &str, int64_t &value)
{
    if (str.empty()) {
        return false;
    }

    errno = 0;
    char* endptr = nullptr;

    int64_t result = std::strtoll(str.c_str(), &endptr, BASE_DECIMAL);

    if (endptr == str.c_str()) {
        return false;
    }
    if (errno == ERANGE && (result == LLONG_MAX || result == LLONG_MIN)) {
        return false;
    }
    if (*endptr != '\0') {
        return false;
    }
    value = result;
    return true;
}

bool ConvertStringToInt32(const std::string &context, int32_t &value)
{
    if (context.empty()) {
        return false;
    }
    std::regex pattern(R"(^([1-9]\d{0,9})$)");
    if (!std::regex_match(context, pattern)) {
        return false;
    }
    char *endptr;
    errno = 0;
    int64_t tollRes = strtoll(context.c_str(), &endptr, BASE_DECIMAL);
    if (errno != 0 || endptr != context.c_str() + context.size()) {
        return false;
    }
    if (tollRes <= 0 || tollRes >= INT32_MAX) {
        return false;
    }
    value = static_cast<int32_t>(tollRes);
    return true;
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
    return lines;
}

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
uint64_t ReadPersistUsbReadonlyMountFlagBits(bool useFuseData)
{
    uint64_t mountFlag = 0;
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
        mountFlag |= static_cast<uint64_t>(MS_RDONLY);
    } else {
        mountFlag &= ~static_cast<uint64_t>(MS_RDONLY);
    }
    return mountFlag;
}

int32_t IsDiskSupported(const std::string &diskId)
{
    Disk disk;
    int32_t ret = DiskManager::GetInstance().GetDiskById(diskId, disk);
    if (ret != E_OK || disk.GetDiskType() == CD_FLAG) {
        LOGE("GetDiskById failed or CD not support, diskId=%{public}s ret=%{public}d", diskId.c_str(), ret);
        return E_NOT_SUPPORT;
    }
    return E_OK;
}

void ApplyDefaultVolumeDescriptionIfUnset(VolumeExternal &volExternal, int32_t flag)
{
    if (!volExternal.GetDescription().empty()) {
        return;
    }
    if (flag == DATA_DISK_SSD || flag == DATA_DISK_HDD) {
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

std::string BuildMountDataOptions(bool useVoldataPath)
{
    if (useVoldataPath) {
        return VOLDATA_MOUNT_SELINUX_CONTEXT;
    }
    return "";
}

bool IsSafeFsUuidForPath(const std::string &fsUuid)
{
    return !fsUuid.empty() && fsUuid.find("..") == std::string::npos && fsUuid.find('/') == std::string::npos;
}

/** QueryUsbIsInUse 入参：已挂载用实际 path，否则按策略推导 voldata/external 路径。 */
std::string ResolveQueryUsbIsInUseMountPath(const VolumeExternal &volExternal, bool isInternalDataDisk)
{
    if (!volExternal.GetPath().empty()) {
        return volExternal.GetPath();
    }
    const std::string fsNormLower = NormalizeFsTypeAsciiLower(volExternal.GetFsTypeString());
    const std::string fsUuid = volExternal.GetUuid();
    if (isInternalDataDisk && fsNormLower == "f2fs" && IsSafeFsUuidForPath(fsUuid)) {
        std::string voldataPath;
        if (VoldataUuidStore::GetInstance().TryGetMountPath(fsUuid, voldataPath)) {
            return voldataPath;
        }
    }
    if (IsSafeFsUuidForPath(fsUuid)) {
        return std::string(EXTERNAL_MOUNT_ROOT) + fsUuid;
    }
    return "";
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
    return fsNormLower == "f2fs";
}

void DiskManager::UpdateVoldataMappingAfterFormat(const std::string &diskId,
                                                  const std::string &oldFsUuid,
                                                  const std::string &newFsUuid,
                                                  const std::string &fsType)
{
    const std::string fsNormLower = NormalizeFsTypeAsciiLower(fsType);
    std::shared_lock<std::shared_mutex> diskLock(diskMapMutex_);
    std::shared_lock<std::shared_mutex> volLock(volumeMapMutex_);
    const bool useVoldataPath = ShouldUseVoldataMountPathForDiskUnlocked(diskId, fsNormLower);

    auto &store = VoldataUuidStore::GetInstance();
    if (!useVoldataPath) {
        if (!oldFsUuid.empty() && IsSafeFsUuid(oldFsUuid)) {
            (void)store.RemoveByFsUuid(oldFsUuid);
        }
        return;
    }
    if (oldFsUuid.empty() || newFsUuid.empty() || !IsSafeFsUuid(oldFsUuid) || !IsSafeFsUuid(newFsUuid)) {
        return;
    }
    std::string unused;
    if (!store.TryGetMountPath(oldFsUuid, unused)) {
        return;
    }
    (void)store.ReplaceFsUuid(oldFsUuid, newFsUuid);
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
        policy.useDvrPath = (dit->second.GetDiskType() == static_cast<int32_t>(DVR_USB));
    }
    const bool isDataFs = (fsNormLower == "f2fs");
    const bool bypassTobForPcNonDataFs = isInternalDisk && !isDataFs;
    policy.useFuseData = !policy.useVoldataPath && !policy.useDvrPath && !bypassTobForPcNonDataFs &&
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
    return E_NON_EXIST;
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

int32_t DiskManager::GetOddCapacity(const std::string &devPath, int64_t &totalSize, int64_t &freeSize)
{
    LOGI("get odd size : devPath is %{public}s", devPath.c_str());
    if (devPath.empty()) {
        return DiskManagerErrNo::DISK_MGR_ERR;
    }
    return StorageDaemonAdapter::GetInstance().GetCapacity(devPath, totalSize, freeSize);
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
    volExternal.SetDescription(label);
    volExternal.SetFsType(volExternal.GetFsTypeByStr(type));
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
        force ? CommonEventPublisher::PublishVolumeChange(BAD_REMOVAL, volExternal)
              : CommonEventPublisher::PublishVolumeChange(REMOVED, volExternal);
        LOGI("UnmountVolumeMountPoints: umount fuse notify");
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

int32_t DiskManager::ResolveUnmountForceFlag(const VolumeExternal &volExternal, bool &forceUnmount)
{
    forceUnmount = true;
    std::shared_lock<std::shared_mutex> diskReadLock(diskMapMutex_);
    const auto dit = diskMap_.find(volExternal.GetDiskId());
    if (dit == diskMap_.end() || !dit->second.IsInternalDataDisk()) {
        return DiskManagerErrNo::E_OK;
    }
    forceUnmount = false;
    const std::string queryPath = ResolveQueryUsbIsInUseMountPath(volExternal, true);
    if (queryPath.empty()) {
        LOGE("Unmount: QueryUsbIsInUse path unresolved volumeId=%{public}s", volExternal.GetId().c_str());
        return E_PARAMS_INVALID;
    }
    bool isInUse = false;
    const int32_t queryErr = StorageDaemonAdapter::GetInstance().QueryUsbIsInUse(queryPath, isInUse);
    if (queryErr != ERR_OK) {
        LOGE("Unmount: QueryUsbIsInUse failed mountPath=%{public}s err=%{public}d", queryPath.c_str(), queryErr);
        return queryErr;
    }
    if (isInUse) {
        LOGE("Unmount: internal data disk in use mountPath=%{public}s volumeId=%{public}s", queryPath.c_str(),
             volExternal.GetId().c_str());
        return E_UMOUNT_BUSY;
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
    VolumeExternal volExternal;
    {
        std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
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
            return E_NON_EXIST;
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
    std::string fsUuid;
    int32_t uuidErr = EnsureFsUuidReady(volExternal, fsUuid);
    const std::string fsType = volExternal.GetFsTypeString();
    if (uuidErr != DiskManagerErrNo::E_OK) {
        volExternal.SetState(VolumeState::UNMOUNTED);
        return uuidErr;
    }
    if (!IsSafeFsUuid(fsUuid)) {
        LOGE("Mount: invalid fsUuid for volumeId=%{public}s", volumeId.c_str());
        volExternal.SetState(VolumeState::UNMOUNTED);
        return E_PARAMS_INVALID;
    }

    if (ComputeVolumeMountPolicy(volExternal.GetDiskId(), fsType).useVoldataPath) {
        volExternal.SetState(CHECKING);
        SetVolumeStateLocked(volumeId, CHECKING);
        CommonEventPublisher::PublishVolumeResult(CHECKING, volExternal);
        int32_t checkErr = StorageDaemonAdapter::GetInstance().Check("/dev/block/" + volExternal.GetId(), fsType,
            true);
        if (checkErr != ERR_OK) {
            LOGE("Mount: Check failed volId=%{public}s err=%{public}d", volumeId.c_str(), checkErr);
            volExternal.SetState(REPAIR_FINISH_FAIL);
            SetVolumeStateLocked(volumeId, REPAIR_FINISH_FAIL);
            CommonEventPublisher::PublishVolumeResult(REPAIR_FINISH_FAIL, volExternal);
            return checkErr;
        }
        CommonEventPublisher::PublishVolumeResult(REPAIR_FINISH_SUCCESS, volExternal);
        volExternal.SetState(UNMOUNTED);
        SetVolumeStateLocked(volumeId, UNMOUNTED);
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
    if (params.policy.useDvrPath) {
        return std::string(EXTERNAL_DVR_ROOT) + params.fsUuid;
    }
    if (params.policy.useFuseData) {
        return std::string(EXTERNAL_FUSE_DATA_ROOT) + params.fsUuid;
    }
    return std::string(EXTERNAL_MOUNT_ROOT) + params.fsUuid;
}

bool DiskManager::CheckSSDAndHDDWhenEnterpriseSpaceEnable(int32_t flag)
{
    int32_t handle = static_cast<int32_t>(FindParameter(PERSIST_ENTERPRISE_SPACE_ENABLE));
    if (handle == -1) {
        return false;
    }

    char spaceEnable[RD_ENABLE_LENGTH] = {"false"};
    auto res = GetParameterValue(handle, spaceEnable, RD_ENABLE_LENGTH);
    if (res >= 0 && strncmp(spaceEnable, "true", TRUE_LEN) != 0) {
        return false;
    }

    if (flag == DATA_DISK_SSD || flag == DATA_DISK_HDD) {
        LOGW("Enterprise space enable, disk type is SSD or HDD, skipping mount operation.");
        return true;
    }

    return false;
}

int32_t DiskManager::MountVolumeSetPath(VolumeExternal &volExternal, std::string& dataMountPath)
{
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volExternal.GetId());
    if (it == volumeMap_.end()) {
        return E_NON_EXIST;
    }

    it->second.SetPath(dataMountPath);
    return ERR_OK;
}

int32_t DiskManager::MountVolumeFilesystem(VolumeExternal &volExternal,
                                           const std::string &fsType,
                                           const std::string &fsUuid)
{
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

    uint64_t mountFlag = ReadPersistUsbReadonlyMountFlagBits(policy.useFuseData);
    if ((fsType == "hmfs" || fsType == "f2fs") && volExternal.GetUserData()) {
        mountFlag = HMFS_FLAG;
    }

    int32_t flag = 0;
    {
        std::shared_lock<std::shared_mutex> diskReadLock(diskMapMutex_);
        flag = ResolveVolumeFlagsUnlocked(volExternal.GetDiskId());
    }

    // 企业空间使能时，不支持挂载数据盘
    if (CheckSSDAndHDDWhenEnterpriseSpaceEnable(flag)) {
        return DiskManagerErrNo::E_OK;
    }

    const std::string mountData = BuildMountDataOptions(policy.useVoldataPath);
    int32_t err = StorageDaemonAdapter::GetInstance().Mount("/dev/block/" + volExternal.GetId(), dataMountPath, fsType,
                                                            mountFlag, mountData);
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
    err = MountVolumeSetPath(volExternal, dataMountPath);
    if (err != ERR_OK) {
        return err;
    }

    volExternal.SetFlags(flag);
    ApplyDefaultVolumeDescriptionIfUnset(volExternal, flag);
    CommonEventPublisher::PublishVolumeChange(MOUNTED, volExternal);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::ResolveVolumeFlagsUnlocked(const std::string &diskId) const
{
    const auto dit = diskMap_.find(diskId);
    if (dit != diskMap_.end()) {
        const int32_t diskType = dit->second.GetDiskType();
        if (diskType != static_cast<int32_t>(DISK_TYPE_UNKNOWN)) {
            return diskType;
        }
    }
    return USB_FLAG;
}

int32_t DiskManager::Unmount(const std::string &volumeId)
{
    VolumeExternal volExternal;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
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
    SaveVolumeFreeSize(volExternal);
    bool forceUnmount = true;
    const int32_t prepErr = ResolveUnmountForceFlag(volExternal, forceUnmount);
    if (prepErr != DiskManagerErrNo::E_OK) {
        return prepErr;
    }

    const int32_t previousState = NotifyVolumeEjecting(volumeId, volExternal);

    const int32_t err = UnmountVolumeMountPoints(volExternal, forceUnmount);
    if (err != ERR_OK) {
        LOGE("Unmount vol %{public}s err=%{public}d", volExternal.GetId().c_str(), err);
        RestoreVolumeState(volumeId, volExternal, previousState);
        return err;
    }

    volExternal.SetPath("");
    volExternal.SetState(UNMOUNTED);
    CommonEventPublisher::PublishVolumeChange(UNMOUNTED, volExternal);

    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        return E_NON_EXIST;
    }
    it->second.SetPath("");
    it->second.SetState(UNMOUNTED);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::Format(const std::string &volumeId, const std::string &fsType)
{
    std::string blockVolId;
    std::string diskId;
    std::string oldFsUuid;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
        }
        if (it->second.GetFsType() == FsType::MTP) {
            LOGE("MTP device not support to format.");
            return E_NOT_SUPPORT;
        }
        if (IsDiskSupported(it->second.GetDiskId()) != E_OK) {
            LOGE("Not support file system, volumeId=%{public}s", volumeId.c_str());
            return E_NOT_SUPPORT;
        }
        if (it->second.GetState() != VolumeState::UNMOUNTED) {
            LOGE("Format: volumeId=%{public}s state=%{public}d not unmounted",
                 volumeId.c_str(), it->second.GetState());
            return E_VOL_STATE;
        }
        blockVolId = it->second.GetId();
        diskId = it->second.GetDiskId();
        oldFsUuid = it->second.GetUuid();
    }
    int32_t err = StorageDaemonAdapter::GetInstance().FormatVolume("/dev/block/" + blockVolId, fsType);
    if (err != ERR_OK) {
        LOGE("Format vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        PublishFormatFailEvent(volumeId);
        return err;
    }
    return UpdateVolumeAfterFormat(volumeId, fsType, diskId, oldFsUuid, blockVolId);
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
        const int32_t previousState = NotifyVolumeEjecting(volumeId, volExternal);
        const int32_t umErr = UnmountVolumeMountPoints(volExternal, true);
        if (umErr != ERR_OK) {
            LOGE("TryToFix: Unmount failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), umErr);
            RestoreVolumeState(volumeId, volExternal, previousState);
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

    int32_t rcErr = RepairAndCheckVolume(volExternal, volumeId);
    if (rcErr != E_OK) {
        return rcErr;
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
    std::string blockVolId;
    std::string fsTypeStr;
    std::string diskId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        VolumeExternal volExternal;
        if (LookupVolumeByUuidUnlocked(fsUuid, volExternal) != E_OK) {
            LOGE("Volume with id %{public}s not found", fsUuid.c_str());
            return E_NON_EXIST;
        }
        if (volExternal.GetState() != VolumeState::UNMOUNTED) {
            LOGE("SetVolumeDescription: fsUuid=%{public}s state=%{public}d not unmounted",
                 fsUuid.c_str(), volExternal.GetState());
            return E_VOL_STATE;
        }
        blockVolId = volExternal.GetId();
        fsTypeStr = volExternal.GetFsTypeString();
        diskId = volExternal.GetDiskId();
    }

    if (IsDiskSupported(diskId) != E_OK) {
        LOGE("SetVolumeDescription: disk not support, fsUuid=%{public}s diskId=%{public}s",
             fsUuid.c_str(), diskId.c_str());
        return E_NOT_SUPPORT;
    }
    
    if (LABEL_SUPPORTED_FS_TYPES.count(fsTypeStr) == 0) {
        LOGE("SetVolumeDescription: fsType not support, fsUuid=%{public}s fsType=%{public}s",
             fsUuid.c_str(), fsTypeStr.c_str());
        return E_NOT_SUPPORT;
    }

    const int32_t err =
        StorageDaemonAdapter::GetInstance().SetLabel("/dev/block/" + blockVolId, fsTypeStr, description);
    if (err != ERR_OK) {
        LOGE("SetLabel vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }

    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(blockVolId);
    if (it == volumeMap_.end()) {
        return E_NON_EXIST;
    }
    it->second.SetDescription(description);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::PurgeVolumesForDisk(const std::string &diskId)
{
    if (!HasDisk(diskId)) {
        return E_DISK_NOT_FOUND;
    }

    std::vector<std::string> volIds;
    std::vector<std::string> fsUuids;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        for (const auto &kv : volumeMap_) {
            if (kv.second.GetDiskId() != diskId) {
                continue;
            }
            volIds.push_back(kv.first);
            const std::string &fsUuid = kv.second.GetUuid();
            if (IsSafeFsUuid(fsUuid)) {
                fsUuids.push_back(fsUuid);
            } else if (!fsUuid.empty()) {
                LOGW("PurgeVolumesForDisk skip invalid fsUuid volId=%{public}s diskId=%{public}s",
                     kv.first.c_str(), diskId.c_str());
            }
        }
    }

    {
        std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
        for (const std::string &volId : volIds) {
            volumeMap_.erase(volId);
        }
    }

    int32_t ret = DiskManagerErrNo::E_OK;
    for (const std::string &fsUuid : fsUuids) {
        const int32_t removeRet = VoldataUuidStore::GetInstance().RemoveByFsUuid(fsUuid);
        if (removeRet != DiskManagerErrNo::E_OK) {
            LOGE("PurgeVolumesForDisk RemoveByFsUuid failed diskId=%{public}s uuid=%{public}s ret=%{public}d",
                 diskId.c_str(), fsUuid.c_str(), removeRet);
            ret = removeRet;
        }
    }
    LOGI("PurgeVolumesForDisk diskId=%{public}s volumes=%{public}zu voldataRemoved=%{public}zu", diskId.c_str(),
         volIds.size(), fsUuids.size());
    return ret;
}

void DiskManager::AddPartitioningDisk(const std::string &diskId)
{
    std::lock_guard<std::mutex> lock(partitionLock_);
    partitioningDiskIds_.insert(diskId);
}

void DiskManager::RemovePartitioningDisk(const std::string &diskId)
{
    std::lock_guard<std::mutex> lock(partitionLock_);
    partitioningDiskIds_.erase(diskId);
}

void DiskManager::NotifyPartitionDone(const std::string &diskId)
{
    std::lock_guard<std::mutex> lock(partitionLock_);
    if (partitionDoneMap_.find(diskId) == partitionDoneMap_.end()) {
        return;
    }
    partitionDoneMap_[diskId] = true;
    partitionCv_.notify_all();
}

void DiskManager::WaitForPartitionDone(const std::string &diskId, int32_t timeoutMs)
{
    std::unique_lock<std::mutex> lock(partitionLock_);
    partitionDoneMap_[diskId] = false;
    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!partitionDoneMap_[diskId]) {
        if (partitionCv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            LOGE("WaitForPartitionDone timeout diskId=%{public}s timeoutMs=%{public}d", diskId.c_str(), timeoutMs);
            break;
        }
    }
    partitionDoneMap_.erase(diskId);
}

bool DiskManager::IsPartitioning(const std::string &diskId) const
{
    std::lock_guard<std::mutex> lock(partitionLock_);
    return partitioningDiskIds_.find(diskId) != partitioningDiskIds_.end();
}

int32_t DiskManager::Partition(const std::string &diskId, int32_t type)
{
    (void)type;
    int32_t ret = IsDiskSupported(diskId);
    if (ret != E_OK) {
        LOGE("Partition failed, not support diskId=%{public}s, ret=%{public}d", diskId.c_str(), ret);
        return ret;
    }
    if (IsDiskNotReady(diskId)) {
        LOGE("Partition failed, disk has mounted volume, diskId=%{public}s", diskId.c_str());
        return E_VOL_STATE;
    }

    AddPartitioningDisk(diskId);
    ret = PurgeVolumesForDisk(diskId);
    if (ret != E_OK) {
        RemovePartitioningDisk(diskId);
        LOGE("Partition failed, purge volumes diskId=%{public}s ret=%{public}d", diskId.c_str(), ret);
        return ret;
    }

    static constexpr const char *partitionFsType = "hmfs";
    const std::string diskPath = NormalizeDiskBlockPath(diskId);
    ret = StorageDaemonAdapter::GetInstance().Partition(diskPath, partitionFsType);
    if (ret != E_OK) {
        RemovePartitioningDisk(diskId);
        LOGE("Partition storage_daemon failed diskId=%{public}s ret=%{public}d", diskId.c_str(), ret);
        return ret;
    }

    ret = UeventBootstrap::RediscoverDiskVolumes(diskId);
    RemovePartitioningDisk(diskId);
    if (ret != E_OK) {
        LOGE("Partition RediscoverDiskVolumes failed diskId=%{public}s ret=%{public}d", diskId.c_str(), ret);
        return ret;
    }
    LOGI("Partition success diskId=%{public}s", diskId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnDiskCreated(const Disk &disk)
{
    Disk diskToStore = disk;
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
    VolumeExternal vol = volExternal;
    {
        std::shared_lock<std::shared_mutex> diskReadLock(diskMapMutex_);
        vol.SetFlags(ResolveVolumeFlagsUnlocked(volExternal.GetDiskId()));
    }
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    volumeMap_.insert(make_pair(vol.GetId(), vol));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::OnVolumeDestroyed(const std::string &volumeId)
{
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    if (volumeMap_.find(volumeId) == volumeMap_.end()) {
        LOGE("DiskManager::OnVolumeDestroyed the vol %{public}s doesn't exist", volumeId.c_str());
        return E_NON_EXIST;
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
        return E_NON_EXIST;
    }
    out = diskMap_[diskId];
    AttachVolumeIdsToDisk(volumeMap_, out);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetAllVolumes(std::vector<VolumeExternal> &out)
{
    std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
    std::vector<VolumeExternal> result;
    std::vector<std::string> dvdDiskIds;
    for (auto it = volumeMap_.begin(); it != volumeMap_.end(); ++it) {
        VolumeExternal volExternal = it->second;
        if (volExternal.GetFsType() == UDF || volExternal.GetFsType() == ISO9660) {
            dvdDiskIds.push_back(volExternal.GetDiskId());
        }
        result.push_back(volExternal);
    }

    for (auto it = diskMap_.begin(); it != diskMap_.end(); ++it) {
        const Disk &disk = it->second;
        if (disk.GetDiskType() != CD_FLAG) {
            continue;
        }
        auto iter = std::find(dvdDiskIds.begin(), dvdDiskIds.end(), disk.GetDiskId());
        if (iter != dvdDiskIds.end()) {
            LOGE("This disk has real volume, diskId: %{public}s", disk.GetDiskId().c_str());
            continue;
        }
        VolumeCore core("0", CD_FLAG, disk.GetDiskId(), MOUNTED, "udf", disk.GetExtraInfo());
        VolumeExternal volExternal(core);
        volExternal.SetFsType(volExternal.GetFsTypeByStr("udf"));
        std::string discType = GetDriverType(disk.GetExtraInfo());
        volExternal.SetDescription(discType.empty() ? "DVD RW" : discType);
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
        return E_NON_EXIST;
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
        return E_NON_EXIST;
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
    std::string blockVolId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        VolumeExternal vol;
        if (LookupVolumeByUuidUnlocked(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
            return DiskManagerErrNo::E_NON_EXIST;
        }
        path = vol.GetPath();
        fsType = vol.GetFsTypeString();
        blockVolId = vol.GetId();
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
        std::unique_lock<std::shared_mutex> volWriteLock(oddMutex_);
        const int32_t oddRet = GetOddCapacity("/dev/block/" + blockVolId, totalSize, freeSize);
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
    std::string blockVolId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        VolumeExternal vol;
        if (LookupVolumeByUuidUnlocked(volumeUuid, vol) != DiskManagerErrNo::E_OK) {
            return DiskManagerErrNo::E_NON_EXIST;
        }
        path = vol.GetPath();
        fsType = vol.GetFsTypeString();
        blockVolId = vol.GetId();
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
        std::unique_lock<std::shared_mutex> volWriteLock(oddMutex_);
        (void)GetOddCapacity("/dev/block/" + blockVolId, totalSize, freeSize);
        return DiskManagerErrNo::E_OK;
    }
    totalSize = static_cast<int64_t>(diskInfo.f_bsize) * static_cast<int64_t>(diskInfo.f_blocks);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::Erase(const std::string &volumeId)
{
    std::string blockVolId;
    std::string diskId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
        }
        blockVolId = it->second.GetId();
        diskId = it->second.GetDiskId();
    }
    int32_t err = StorageDaemonAdapter::GetInstance().Erase("/dev/block/" + blockVolId);
    if (err != ERR_OK) {
        LOGE("Erase vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }
    err = Eject(diskId);
    if (err != ERR_OK) {
        LOGE("Erase-Eject vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::Eject(const std::string &diskId)
{
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        LOGE("Disk with id %{public}s not found", diskId.c_str());
        return E_NON_EXIST;
    }
    std::string devName = disk.GetDevName();
    int32_t err = StorageDaemonAdapter::GetInstance().Eject(devName);
    if (err != ERR_OK) {
        LOGE("Eject diskId %{public}s err=%{public}d", diskId.c_str(), err);
        return err;
    }
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::CreateIsoImage(const std::string &volumeId,
                                    const std::string &filePath)
{
    std::string blockVolId;
    std::string fsType;
    std::string mountedPath;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
        }
        blockVolId = it->second.GetId();
        fsType = it->second.GetFsTypeString();
        mountedPath = it->second.GetPath();
    }
    int32_t err = StorageDaemonAdapter::GetInstance().CreateIsoImage("/dev/block/" + blockVolId,
                                                                     filePath, fsType, mountedPath);
    if (err != ERR_OK) {
        LOGE("CreateIsoImage vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }
    return DiskManagerErrNo::E_OK;
}

std::string DiskManager::GetDiscType(const std::string &extraInfo)
{
    LOGI("GetDiscType: extraInfo=%{public}s", extraInfo.c_str());
    if (extraInfo.empty()) {
        return "";
    }
    
    json extraInfoJson = json::parse(extraInfo, nullptr, false);
    if (extraInfoJson.is_discarded() || !extraInfoJson.is_object()) {
        LOGW("GetDiscType: Failed to parse extraInfo JSON");
        return "";
    }
    
    if (!extraInfoJson.contains("ODD_INFO") || !extraInfoJson["ODD_INFO"].is_object()) {
        return "";
    }
    
    const auto& oddInfo = extraInfoJson["ODD_INFO"];
    if (!oddInfo.contains("DISC_TYPE") || !oddInfo["DISC_TYPE"].is_string()) {
        return "";
    }
    
    std::string discType = oddInfo["DISC_TYPE"].get<std::string>();
    LOGI("GetDiscType: discType=%{public}s", discType.c_str());
    return discType;
}

std::string DiskManager::GetDriverType(const std::string &extraInfo)
{
    LOGI("GetDriverType: extraInfo=%{public}s", extraInfo.c_str());
    if (extraInfo.empty()) {
        return "";
    }
    
    json extraInfoJson = json::parse(extraInfo, nullptr, false);
    if (extraInfoJson.is_discarded() || !extraInfoJson.is_object()) {
        LOGW("GetDriverType: Failed to parse extraInfo JSON");
        return "";
    }
    
    if (!extraInfoJson.contains("ODD_INFO") || !extraInfoJson["ODD_INFO"].is_object()) {
        return "";
    }
    
    const auto& oddInfo = extraInfoJson["ODD_INFO"];
    if (!oddInfo.contains("DRIVE_TYPE") || !oddInfo["DRIVE_TYPE"].is_string()) {
        return "";
    }
    
    std::string driverType = oddInfo["DRIVE_TYPE"].get<std::string>();
    LOGI("GetDriverType: driverType=%{public}s", driverType.c_str());
    return driverType;
}

int32_t DiskManager::Burn(const std::string &volumeId, const std::string &burnOptions,
                          const std::string &callerBundle, int32_t callerUserId)
{
    std::string blockVolId;
    std::string fsType;
    std::string extraInfo;
    std::string diskId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
        }
        blockVolId = it->second.GetId();
        fsType = it->second.GetFsTypeString();
        extraInfo = it->second.GetExtraInfo();
        diskId = it->second.GetDiskId();
    }

    int32_t err = StorageDaemonAdapter::GetInstance().Burn("/dev/block/" + blockVolId, burnOptions, fsType);
    LOGI("Burn completed: callerBundle=%{public}s callerUserId=%{public}d fsType=%{public}s",
        callerBundle.c_str(), callerUserId, fsType.c_str());
    int32_t reportRet = ReportBurnSecurityInfo(callerUserId, callerBundle, fsType);
    LOGI("Burn result: reportResult=%{public}d", reportRet);
    if (err != ERR_OK) {
        LOGE("Burn vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::GetVolumeOpProcess(const std::string &volumeId, int32_t &progressPct)
{
    std::string blockVolId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
        }
        blockVolId = it->second.GetId();
    }
    int32_t err = StorageDaemonAdapter::GetInstance().GetVolumeOpProcess(blockVolId, progressPct);
    if (err != ERR_OK) {
        LOGE("GetVolumeOpProcess vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }
    LOGI("DiskManager GetVolumeOpProcess volumeId = %{public}s, progressPct = %{public}d",
        volumeId.c_str(), progressPct);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::VerifyBurnData(const std::string &volumeId, int32_t verifyType)
{
    std::string blockVolId;
    {
        std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it == volumeMap_.end()) {
            LOGE("Volume with id %{public}s not found", volumeId.c_str());
            return E_NON_EXIST;
        }
        blockVolId = it->second.GetId();
    }
    int32_t err = StorageDaemonAdapter::GetInstance().VerifyBurnData("/dev/block/" + blockVolId, verifyType);
    if (err != ERR_OK) {
        LOGE("VerifyBurnData vol %{public}s err=%{public}d", blockVolId.c_str(), err);
        return err;
    }
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

int32_t DiskManager::GetPartitionTable(const std::string &diskId, PartitionTableInfo &info)
{
    if (!HasDisk(diskId)) {
        LOGE("diskId not exist.");
        return E_NON_EXIST;
    }
    std::lock_guard<std::mutex> lock(partitionLock_);
    std::string execRet;
    std::string devPath = "/dev/block/" + diskId;
    int32_t ret = StorageDaemonAdapter::GetInstance().GetPartitionTableInfo(devPath, execRet);
    if (ret != E_OK || execRet.empty()) {
        LOGE("get partition table info failed.");
        return E_GET_PARTITION_ERROR;
    }
    std::vector<std::string> tempInfo = SplitRawDumpToLines(execRet);
    if (!SetSectorSize(tempInfo, info)) {
        return E_GET_PARTITION_ERROR;
    }
    if (!SetAlignSector(tempInfo, info)) {
        return E_GET_PARTITION_ERROR;
    }
    if (!SetUsableSector(tempInfo, info)) {
        return E_GET_PARTITION_ERROR;
    }
    info.SetDiskId(diskId);
    SetTableType(tempInfo, info);
    SetPartitions(tempInfo, info);
    info.SetPartitionCount(static_cast<int32_t>(info.GetPartitions().size()));
    partitionTableMap_[info.GetDiskId()] = info;
    LOGI("GetPartitionTable success");
    return DiskManagerErrNo::E_OK;
}

void DiskManager::SetPartitions(std::vector<std::string> &content, PartitionTableInfo &tableInfo)
{
    auto count = static_cast<int32_t>(content.size());
    int32_t partitionIndex = -1;
    for (int32_t i = 0; i < count; i++) {
        std::string buf = content[i];
        if (buf.find("Number") == 0 && buf.find("Start") != std::string::npos) {
            partitionIndex = i + 1;
            break;
        }
    }
    if (partitionIndex < 0 || partitionIndex >= count) {
        return;
    }
    std::vector<PartitionInfo> partitions;
    for (int32_t i = partitionIndex; i < count; i++) {
        std::string buf = content[i];
        PartitionInfo info;
        if (ParsePartitionInfo(buf, info)) {
            int64_t sizeBytes = (info.GetEndSector() - info.GetStartSector() + 1) * tableInfo.GetSectorSize();
            info.SetSizeBytes(sizeBytes);
            info.SetFsType(GetFsTypeByDiskIdAndPartNum(tableInfo.GetDiskId(), info.GetPartitionNum()));
            partitions.push_back(info);
            LOGI("partition info is partitionNum=%{public}d, startSector=%{public}lld, endSector=%{public}lld, "
                 "sizeBytes=%{public}lld, fsType=%{public}s", info.GetPartitionNum(),
                 static_cast<long long>(info.GetStartSector()), static_cast<long long>(info.GetEndSector()),
                 static_cast<long long>(info.GetSizeBytes()), info.GetFsType().c_str());
        }
    }
    tableInfo.SetPartitions(partitions);
}

std::string DiskManager::GetFsTypeByDiskIdAndPartNum(const std::string &diskId, int32_t partitionNum)
{
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        return "";
    }
    for (const auto &item: disk.GetVolumeIds()) {
        VolumeExternal external;
        if (GetVolumeById(item, external) != E_OK) {
            continue;
        }
        if (external.GetPartitionNum() != partitionNum) {
            continue;
        }
        return external.GetFsTypeString();
    }
    return "";
}

bool DiskManager::ParsePartitionInfo(const std::string &context, PartitionInfo &info)
{
    if (context.empty()) {
        return false;
    }
    std::stringstream ss(context);
    std::string item;
    ss >> item;
    if (item.empty()) {
        return false;
    }
    int32_t partitionNum;
    if (!ConvertStringToInt32(item, partitionNum)) {
        LOGE("convert partition num failed, %{public}s", item.c_str());
        return false;
    }
    info.SetPartitionNum(partitionNum);
    ss >> item;
    if (item.empty()) {
        return false;
    }
    int64_t startSector;
    if (!ConvertStringToInt(item, startSector)) {
        LOGE("convert start sector failed, %{public}s", item.c_str());
        return false;
    }
    info.SetStartSector(startSector);
    ss >> item;
    if (item.empty()) {
        return false;
    }
    int64_t endSector;
    if (!ConvertStringToInt(item, endSector)) {
        LOGE("convert end sector failed, %{public}s", item.c_str());
        return false;
    }
    info.SetEndSector(endSector);
    return true;
}

bool DiskManager::SetUsableSector(std::vector<std::string> &content, PartitionTableInfo &info)
{
    auto count = static_cast<int32_t>(content.size());
    std::string prefix = "First usable sector is";
    std::string target;
    for (int32_t i = 0; i < count; i++) {
        std::string buf = content[i];
        if (buf.find(prefix) == 0) {
            target = buf;
            break;
        }
    }
    if (target.empty()) {
        LOGE("not found usable sector");
        return false;
    }
    std::regex pattern(R"(last usable sector is (\d+))");
    std::smatch match;
    if (!std::regex_search(target, match, pattern)) {
        LOGE("usable sector not match, target=%{public}s", target.c_str());
        return false;
    }
    std::string result = match[1].str();
    int64_t lastUsableSector = 0;
    if (!ConvertStringToInt(result, lastUsableSector)) {
        LOGE("convert last usable sector failed, result=%{public}s", result.c_str());
        return false;
    }
    info.SetLastUsableSector(lastUsableSector);
    info.SetTotalSector(lastUsableSector);
    LOGI("parse lastUsableSector success, %{public}lld", static_cast<long long>(lastUsableSector));
    return true;
}

bool DiskManager::SetSectorSize(std::vector<std::string> &content, PartitionTableInfo &info)
{
    auto count = static_cast<int32_t>(content.size());
    std::string prefix = "Sector size ";
    std::string target;
    for (int32_t i = 0; i < count; i++) {
        std::string buf = content[i];
        if (buf.find(prefix) == 0) {
            target = buf;
            break;
        }
    }
    if (target.empty()) {
        LOGE("not found sector size");
        return false;
    }
    std::regex pattern(R"(Sector size\s*\(logical(?:/physical)?\):\s*(\d+))");
    std::smatch match;
    if (!std::regex_search(target, match, pattern)) {
        LOGE("sector size not match, target=%{public}s", target.c_str());
        return false;
    }
    std::string result = match[1].str();
    int32_t sectorSize = 0;
    if (!ConvertStringToInt32(result, sectorSize)) {
        LOGE("convert sector size failed, result=%{public}s", result.c_str());
        return false;
    }
    info.SetSectorSize(sectorSize);
    LOGI("parse sectorSize success, %{public}lld", static_cast<long long>(sectorSize));
    return true;
}

bool DiskManager::SetAlignSector(std::vector<std::string> &content, PartitionTableInfo &info)
{
    auto count = static_cast<int32_t>(content.size());
    std::string prefix = "Partitions will be aligned on";
    std::string target;
    for (int32_t i = 0; i < count; i++) {
        std::string buf = content[i];
        if (buf.find(prefix) == 0) {
            target = buf;
            break;
        }
    }
    if (target.empty()) {
        LOGE("not found align sector");
        return false;
    }
    std::regex pattern(R"(Partitions will be aligned on (\d+)-sector boundaries)");
    std::smatch match;
    if (!std::regex_search(target, match, pattern)) {
        LOGE("align sector not match, target=%{public}s", target.c_str());
        return false;
    }
    std::string result = match[1].str();
    int32_t alignSector = 0;
    if (!ConvertStringToInt32(result, alignSector)) {
        LOGE("convert align sector failed, result=%{public}s", result.c_str());
        return false;
    }
    info.SetAlignSector(alignSector);
    LOGI("parse alignSector success, %{public}lld", static_cast<long long>(alignSector));
    return true;
}

void DiskManager::SetTableType(std::vector<std::string> &content, PartitionTableInfo &info)
{
    auto count = static_cast<int32_t>(content.size());
    std::string prefix = "Found invalid GPT and valid MBR";
    bool isMBR = false;
    for (int32_t i = 0; i < count; i++) {
        std::string buf = content[i];
        if (buf.find(prefix) == 0) {
            LOGI("this disk table type is mbr");
            isMBR = true;
            break;
        }
    }
    info.SetTableType(isMBR ? "MBR" : "GPT");
}

int32_t DiskManager::CreatePartition(const std::string &diskId, const PartitionParams &params)
{
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        return E_NON_EXIST;
    }
    if (disk.GetDiskType() != DiskType::SD_FLAG && disk.GetDiskType() != DiskType::USB_FLAG) {
        LOGE("disk type not support, diskType=%{public}d.", disk.GetDiskType());
        return E_CREATE_PARTITION_NOT_SUPPORT;
    }
    auto codeIt = typeCodeMap_.find(params.GetTypeCode());
    if (codeIt == typeCodeMap_.end()) {
        LOGE("type code not support, typeCode=%{public}s.", params.GetTypeCode().c_str());
        return E_CREATE_PARTITION_NOT_SUPPORT;
    }
    if (IsDiskNotReady(diskId)) {
        return E_VOL_STATE;
    }
    {
        std::lock_guard<std::mutex> lock(partitionLock_);
        if (partitionTableMap_.find(diskId) == partitionTableMap_.end()) {
            LOGE("partition table info not exists, id=%{public}s", diskId.c_str());
            return E_NON_EXIST;
        }
        PartitionTableInfo info = partitionTableMap_[diskId];
        if (!IsParamsValid(params, info)) {
            return E_PARAMS_INVALID;
        }
        int32_t ret = StorageDaemonAdapter::GetInstance().CreatePartition("/dev/block/" + diskId,
            params.GetPartitionNum(), params.GetStartSector(), params.GetEndSector(), codeIt->second);
        if (ret != DiskManagerErrNo::E_OK) {
            LOGE("CreatePartition failed, diskId=%{public}s, err=%{public}d", diskId.c_str(), ret);
            return E_CREATE_PARTITION_ERROR;
        }
    }
    LOGI("CreatePartition daemon call success, waiting for uevent diskId=%{public}s", diskId.c_str());
    WaitForPartitionDone(diskId, WAIT_UEVENT_TIMEOUT);
    LOGI("CreatePartition complete diskId=%{public}s", diskId.c_str());
    return DiskManagerErrNo::E_OK;
}

bool DiskManager::IsDiskNotReady(const std::string &diskId)
{
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        return true;
    }
    std::vector<std::string> volIds = disk.GetVolumeIds();
    if (volIds.empty()) {
        return false;
    }
    for (const auto &item: volIds) {
        VolumeExternal external;
        if (GetVolumeById(item, external) != E_OK) {
            continue;
        }
        if (external.GetState() != UNMOUNTED) {
            LOGE("volume status is not unmounted, id=%{public}s", item.c_str());
            return true;
        }
    }
    return false;
}

bool DiskManager::IsParamsValid(const PartitionParams &params, const PartitionTableInfo &info)
{
    int64_t startSector = params.GetStartSector();
    if (startSector > info.GetLastUsableSector() || startSector < info.GetAlignSector()) {
        LOGE("start sector out range");
        return false;
    }
    if (((startSector * info.GetSectorSize()) % info.GetAlignSector()) != 0) {
        LOGE("start sector not align");
        return false;
    }
    int64_t endSector = params.GetEndSector();
    if (endSector > info.GetLastUsableSector() || endSector < info.GetAlignSector()) {
        LOGE("end sector out range");
        return false;
    }
    std::string typeCode = params.GetTypeCode();
    int64_t sectorInterval = (endSector - startSector + 1) * info.GetSectorSize();
    if (typeCode == "vfat" && sectorInterval < VFAT_TYPECODE_MIN_SIZE) {
        LOGE("vfat sector interval invalid");
        return false;
    }
    if (typeCode == "exfat" && sectorInterval < EXFAT_TYPECODE_MIN_SIZE) {
        LOGE("exfat sector interval invalid");
        return false;
    }
    if (typeCode == "ntfs" && sectorInterval < NTFS_TYPECODE_MIN_SIZE) {
        LOGE("ntfs sector interval invalid");
        return false;
    }
    if (typeCode == "ext4" && sectorInterval < EXT4_TYPECODE_MIN_SIZE) {
        LOGE("ext4 sector interval invalid");
        return false;
    }
    if ((typeCode == "f2fs" || typeCode == "hmfs") && sectorInterval < F2FS_TYPECODE_MIN_SIZE) {
        LOGE("f2fs or hmfs sector interval invalid");
        return false;
    }
    return true;
}

int32_t DiskManager::DeletePartition(const std::string &diskId, int32_t partitionNum)
{
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        return E_NON_EXIST;
    }
    if (disk.GetDiskType() != DiskType::SD_FLAG && disk.GetDiskType() != DiskType::USB_FLAG) {
        LOGE("disk type not support, diskType=%{public}d.", disk.GetDiskType());
        return E_DELETE_PARTITION_NOT_SUPPORT;
    }
    if (IsDiskNotReady(diskId)) {
        return E_VOL_STATE;
    }
    {
        std::lock_guard<std::mutex> lock(partitionLock_);
        if (partitionTableMap_.find(diskId) == partitionTableMap_.end()) {
            LOGE("partition table info not exists, id=%{public}s", diskId.c_str());
            return E_NON_EXIST;
        }
        PartitionTableInfo info = partitionTableMap_[diskId];
        bool partNumValid = false;
        std::vector<PartitionInfo> infos = info.GetPartitions();
        for (const auto &item: infos) {
            if (item.GetPartitionNum() == partitionNum) {
                partNumValid = true;
                break;
            }
        }
        if (!partNumValid) {
            LOGE("partition num not exists, partitionNum=%{public}d.", partitionNum);
            return E_NON_EXIST;
        }
        int32_t ret = StorageDaemonAdapter::GetInstance().DeletePartition(disk.GetSysPath(), diskId, partitionNum);
        if (ret != DiskManagerErrNo::E_OK) {
            LOGE("DeletePartition failed, diskId=%{public}s, err=%{public}d", diskId.c_str(), ret);
            return E_DELETE_PARTITION_ERROR;
        }
        DestroyVolumeByDiskIdAndPartNum(diskId, partitionNum);
    }
    LOGI("DeletePartition daemon call success, waiting for uevent diskId=%{public}s", diskId.c_str());
    WaitForPartitionDone(diskId, WAIT_UEVENT_TIMEOUT);
    LOGI("DeletePartition success");
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManager::FormatPartition(const std::string &diskId, int32_t partitionNum, const FormatParams &params)
{
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        return E_NON_EXIST;
    }
    if (disk.GetDiskType() != DiskType::SD_FLAG && disk.GetDiskType() != DiskType::USB_FLAG) {
        LOGE("disk type not support, diskType=%{public}d.", disk.GetDiskType());
        return E_FORMAT_PARTITION_NOT_SUPPORT;
    }
    if (IsVolumeMounted(diskId, partitionNum)) {
        return E_VOL_STATE;
    }
    VolumeExternal fmtVol = FindVolumeForPartition(disk, partitionNum);
    std::string devPath;
    {
        std::lock_guard<std::mutex> lock(partitionLock_);
        auto tblIt = partitionTableMap_.find(diskId);
        if (tblIt == partitionTableMap_.end()) {
            LOGE("partition table info not exists, id=%{public}s", diskId.c_str());
            return E_NON_EXIST;
        }
        for (const auto &item : tblIt->second.GetPartitions()) {
            if (item.GetPartitionNum() == partitionNum) {
                devPath = disk.GetSysPath() + std::to_string(partitionNum);
                break;
            }
        }
        if (devPath.empty()) {
            LOGE("partition num not exists, partitionNum=%{public}d.", partitionNum);
            return E_NON_EXIST;
        }
    }
    int32_t ret = StorageDaemonAdapter::GetInstance().FormatPartition(devPath, params.GetFsType(),
                                                                      params.GetVolumeName(), params.GetQuickFormat());
    if (ret != DiskManagerErrNo::E_OK) {
        LOGE("FormatPartition failed, diskId=%{public}s, err=%{public}d", diskId.c_str(), ret);
        if (!fmtVol.GetId().empty()) {
            CommonEventPublisher::PublishVolumeResult(FORMAT_FINISH_FAIL, fmtVol);
        }
        return E_FORMAT_PARTITION_ERROR;
    }
    LOGI("FormatPartition success");
    return DiskManagerErrNo::E_OK;
}

bool DiskManager::IsVolumeMounted(const std::string &diskId, int32_t partitionNum)
{
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        return true;
    }
    std::vector<std::string> volIds = disk.GetVolumeIds();
    if (volIds.empty()) {
        return true;
    }
    for (const auto &item: volIds) {
        VolumeExternal external;
        if (GetVolumeById(item, external) != E_OK) {
            continue;
        }
        if (external.GetPartitionNum() == partitionNum && external.GetState() != UNMOUNTED) {
            LOGE("volume status is not unmounted, id=%{public}s", item.c_str());
            return true;
        }
    }
    return false;
}

void DiskManager::SetVolumeStateLocked(const std::string &volumeId, VolumeState state)
{
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volumeId);
    if (it != volumeMap_.end()) {
        it->second.SetState(state);
    }
}

void DiskManager::PublishFormatFailEvent(const std::string &volumeId)
{
    SetVolumeStateLocked(volumeId, FORMAT_FINISH_FAIL);
    std::shared_lock<std::shared_mutex> volReadLock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it != volumeMap_.end()) {
        CommonEventPublisher::PublishVolumeResult(FORMAT_FINISH_FAIL, it->second);
    }
}

int32_t DiskManager::UpdateVolumeAfterFormat(const std::string &volumeId, const std::string &fsType,
    const std::string &diskId, const std::string &oldFsUuid, const std::string &blockVolId)
{
    std::string uuid;
    std::string type;
    std::string label;
    StorageDaemonAdapter::GetInstance().ReadMetadata("/dev/block/" + blockVolId, uuid, type, label);
    UpdateVoldataMappingAfterFormat(diskId, oldFsUuid, uuid, fsType);
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    auto it = volumeMap_.find(volumeId);
    if (it == volumeMap_.end()) {
        return E_NON_EXIST;
    }
    it->second.SetFsUuid(uuid);
    it->second.SetDescription(label);
    it->second.SetFsType(it->second.GetFsTypeByStr(fsType));
    it->second.SetState(UNMOUNTED);
    return E_OK;
}

VolumeExternal DiskManager::FindVolumeForPartition(const Disk &disk, int32_t partitionNum)
{
    for (const auto &vid : disk.GetVolumeIds()) {
        VolumeExternal ext;
        if (GetVolumeById(vid, ext) == E_OK && ext.GetPartitionNum() == partitionNum) {
            return ext;
        }
    }
    return VolumeExternal();
}

int32_t DiskManager::RepairAndCheckVolume(VolumeExternal &volExternal, const std::string &volumeId)
{
    const std::string devPath = "/dev/block/" + volExternal.GetId();
    const std::string fsType = volExternal.GetFsTypeString();
    volExternal.SetState(CHECKING);
    SetVolumeStateLocked(volumeId, CHECKING);
    CommonEventPublisher::PublishVolumeResult(CHECKING, volExternal);

    int32_t fixErr = StorageDaemonAdapter::GetInstance().Repair(devPath, fsType);
    if (fixErr != ERR_OK) {
        LOGE("RepairAndCheck: Repair failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), fixErr);
        volExternal.SetState(REPAIR_FINISH_FAIL);
        SetVolumeStateLocked(volumeId, REPAIR_FINISH_FAIL);
        CommonEventPublisher::PublishVolumeResult(REPAIR_FINISH_FAIL, volExternal);
        return fixErr;
    }

    if (ComputeVolumeMountPolicy(volExternal.GetDiskId(), fsType).useVoldataPath) {
        int32_t checkErr = StorageDaemonAdapter::GetInstance().Check(devPath, fsType, true);
        if (checkErr != ERR_OK) {
            LOGE("RepairAndCheck: Check failed volumeId=%{public}s err=%{public}d", volumeId.c_str(), checkErr);
            volExternal.SetState(REPAIR_FINISH_FAIL);
            SetVolumeStateLocked(volumeId, REPAIR_FINISH_FAIL);
            CommonEventPublisher::PublishVolumeResult(REPAIR_FINISH_FAIL, volExternal);
            return checkErr;
        }
    }
    CommonEventPublisher::PublishVolumeResult(REPAIR_FINISH_SUCCESS, volExternal);
    volExternal.SetState(UNMOUNTED);
    SetVolumeStateLocked(volumeId, UNMOUNTED);
    return E_OK;
}

int32_t DiskManager::NotifyVolumeEjecting(const std::string &volumeId, VolumeExternal &volExternal)
{
    const int32_t previousState = volExternal.GetState();
    volExternal.SetState(EJECTING);
    {
        std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
        const auto it = volumeMap_.find(volumeId);
        if (it != volumeMap_.end()) {
            it->second.SetState(EJECTING);
        }
    }
    CommonEventPublisher::PublishVolumeChange(EJECTING, volExternal);
    return previousState;
}

void DiskManager::RestoreVolumeState(const std::string &volumeId, VolumeExternal &volExternal, int32_t state)
{
    volExternal.SetState(state);
    std::unique_lock<std::shared_mutex> volWriteLock(volumeMapMutex_);
    const auto it = volumeMap_.find(volumeId);
    if (it != volumeMap_.end()) {
        it->second.SetState(state);
    }
}

void DiskManager::SaveVolumeFreeSize(VolumeExternal &volExternal)
{
    int64_t freeSize = 0;
    int32_t ret = GetFreeSizeOfVolume(volExternal.GetUuid(), freeSize);
    if (ret == E_OK) {
        if (freeSize < 0) {
            LOGW("Unmount: invalid freeSize=%{public}lld for volumeId=%{public}s, skip saving",
                static_cast<long long>(freeSize), volExternal.GetId().c_str());
        } else {
            volExternal.SetFreeSize(freeSize);
            LOGI("Unmount: saving freeSize=%{public}lld for volumeId=%{public}s",
                static_cast<long long>(freeSize), volExternal.GetId().c_str());
        }
    } else {
        LOGW("Unmount: failed to get freeSize for volumeId=%{public}s, ret=%{public}d",
            volExternal.GetId().c_str(), ret);
    }
}

bool DiskManager::DestroyVolumeByDiskIdAndPartNum(const std::string &diskId, int32_t partNum)
{
    LOGI("DestroyVolume enter diskId=%{public}s, partNum=%{public}d.", diskId.c_str(), partNum);
    Disk disk;
    if (GetDiskById(diskId, disk) != E_OK) {
        return false;
    }
    std::vector<std::string> volumeIds = disk.GetVolumeIds();
    VolumeExternal vol;
    bool found;
    for (const auto &item: volumeIds) {
        if (GetVolumeById(item, vol) != E_OK) {
            continue;
        }
        if (vol.GetPartitionNum() == partNum) {
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }
    std::string devPath = "/dev/block/" + vol.GetId();
    int32_t ret = StorageDaemonAdapter::GetInstance().DestroyBlockDeviceNode(devPath);
    if (ret != E_OK) {
        LOGI("Destroy volume failed devPath:%{public}s, ret:%{public}d", devPath.c_str(), ret);
        return false;
    }
    CommonEventPublisher::PublishVolumeChange((vol.GetState() == UNMOUNTED) ? REMOVED : BAD_REMOVAL, vol);
    (void)DiskManager::GetInstance().OnVolumeDestroyed(vol.GetId());
    LOGI("DestroyVolume end.");
    return true;
}
} // namespace DiskManager
} // namespace OHOS
