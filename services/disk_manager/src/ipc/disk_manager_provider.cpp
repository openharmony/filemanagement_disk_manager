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

#include "disk_manager_provider.h"

#include <climits>

#include "block_info_table.h"
#include "disk_manager.h"
#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "disk_manager_utils.h"
#include "errors.h"
#include "ipc_caller_auth.h"
#include "ipc_skeleton.h"
#include "partition_types.h"
#include "storage_daemon_adapter.h"
#include "uevent_bootstrap.h"
#include "usb_fuse_adapter.h"
#include "voldata_uuid_store.h"

namespace OHOS {
namespace DiskManager {

using namespace OHOS::DiskManager;
constexpr pid_t STORAGEDAEMON_UID = 0;
constexpr pid_t STORAGE_MANAGER_UID = 1090;

REGISTER_SYSTEM_ABILITY_BY_ID(DiskManagerProvider, DISK_MANAGER_SA_ID, false);

DiskManagerProvider::DiskManagerProvider(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate) {}

void DiskManagerProvider::OnStart()
{
    LOGI("OnStart begin");
    const bool published = SystemAbility::Publish(this);
    if (!published) {
        LOGE("Publish failed");
    }
    UeventBootstrap::Init();
    const int32_t reloadErr = BlockInfoTable::GetInstance().ReloadFromDaemon();
    LOGI("OnStart BlockInfoTable::ReloadFromDaemon ret=%{public}d", reloadErr);
    const int32_t voldataMapErr = VoldataUuidStore::GetInstance().Init();
    LOGI("OnStart VoldataUuidStore::Init ret=%{public}d", voldataMapErr);
    LOGI("OnStart end");
}

void DiskManagerProvider::OnStop()
{
    LOGI("OnStop");
}

bool DiskManagerProvider::CheckStorageDaemonPermission()
{
    return IpcCallerAuth::VerifyNativeCallerMatches("storage_daemon",
                                                    STORAGEDAEMON_UID);
}

bool DiskManagerProvider::IsStorageManagerCaller() const
{
    return IpcCallerAuth::VerifyNativeCallerMatches("storage_manager", STORAGE_MANAGER_UID);
}

int32_t DiskManagerProvider::Mount(const std::string &volumeId)
{
    LOGI("Mount volumeId=%{public}s", volumeId.c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("Mount: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("Mount: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("Mount: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().Mount(volumeId);
}

int32_t DiskManagerProvider::Unmount(const std::string &volumeId)
{
    LOGI("Unmount volumeId=%{public}s", volumeId.c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("Unmount: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("Unmount: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("Unmount: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().Unmount(volumeId);
}

int32_t DiskManagerProvider::Format(const std::string &volumeId, const std::string &fsType)
{
    LOGI("Format volumeId=%{public}s fsType=%{public}s", volumeId.c_str(), fsType.c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("Format: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_FORMAT_MANAGER)) {
            LOGE("Format: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("Format: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().Format(volumeId, fsType);
}

int32_t DiskManagerProvider::TryToFix(const std::string &volumeId)
{
    LOGI("TryToFix volumeId=%{public}s", volumeId.c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("TryToFix: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("TryToFix: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("TryToFix: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().TryToFix(volumeId);
}

int32_t DiskManagerProvider::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    LOGI("SetVolumeDescription fsUuid=%{public}s", GetAnonyString(fsUuid).c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("SetVolumeDescription: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("SetVolumeDescription: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsUuidValid(fsUuid)) {
        LOGE("SetVolumeDescription: fsUuid is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().SetVolumeDescription(fsUuid, description);
}

int32_t DiskManagerProvider::GetAllVolumes(std::vector<VolumeExternal> &vecOfVol)
{
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("GetAllVolumes: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_STORAGE_MANAGER)) {
            LOGE("GetAllVolumes: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    int32_t err = DiskManager::GetInstance().GetAllVolumes(vecOfVol);
    LOGI("GetAllVolumes count=%{public}zu err=%{public}d", vecOfVol.size(), err);
    return err;
}

int32_t DiskManagerProvider::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &vc)
{
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("GetVolumeByUuid: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_STORAGE_MANAGER)) {
            LOGE("GetVolumeByUuid: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsUuidValid(fsUuid)) {
        LOGE("GetVolumeByUuid: fsUuid is invalid");
        return E_PARAMS_INVALID;
    }
    const int32_t err = DiskManager::GetInstance().GetVolumeByUuid(fsUuid, vc);
    LOGI("GetVolumeByUuid fsUuid=%{public}s err=%{public}d", GetAnonyString(fsUuid).c_str(), err);
    return err;
}

int32_t DiskManagerProvider::GetVolumeById(const std::string &volumeId, VolumeExternal &vc)
{
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("GetVolumeById: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_STORAGE_MANAGER)) {
            LOGE("GetVolumeById: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("GetVolumeById: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    const int32_t err = DiskManager::GetInstance().GetVolumeById(volumeId, vc);
    LOGI("GetVolumeById volumeId=%{public}s err=%{public}d", volumeId.c_str(), err);
    return err;
}

int32_t DiskManagerProvider::GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize)
{
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("GetFreeSizeOfVolume: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_STORAGE_MANAGER)) {
            LOGE("GetFreeSizeOfVolume: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsUuidValid(volumeUuid)) {
        LOGE("GetFreeSizeOfVolume: volumeUuid is invalid");
        return E_PARAMS_INVALID;
    }
    LOGI("GetFreeSizeOfVolume volumeUuid=%{public}s", GetAnonyString(volumeUuid).c_str());
    return DiskManager::GetInstance().GetFreeSizeOfVolume(volumeUuid, freeSize);
}

int32_t DiskManagerProvider::GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize)
{
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("GetTotalSizeOfVolume: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_STORAGE_MANAGER)) {
            LOGE("GetTotalSizeOfVolume: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsUuidValid(volumeUuid)) {
        LOGE("GetTotalSizeOfVolume: volumeUuid is invalid");
        return E_PARAMS_INVALID;
    }
    LOGI("GetTotalSizeOfVolume volumeUuid=%{public}s", GetAnonyString(volumeUuid).c_str());
    return DiskManager::GetInstance().GetTotalSizeOfVolume(volumeUuid, totalSize);
}

int32_t DiskManagerProvider::Partition(const std::string &diskId, int32_t type)
{
    LOGI("Partition diskId=%{public}s type=%{public}d", diskId.c_str(), type);
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("Partition: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_FORMAT_MANAGER)) {
            LOGE("Partition: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsDiskIdValid(diskId)) {
        LOGE("Partition: diskId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().Partition(diskId, type);
}

int32_t DiskManagerProvider::OnBlockDiskUevent(const std::string &rawUeventMsg)
{
    LOGI("OnBlockDiskUevent len=%{public}zu", rawUeventMsg.size());
    if (!CheckStorageDaemonPermission()) {
        return E_PERMISSION_DENIED;
    }
    if (rawUeventMsg.size() > UEVENT_MAX_LEN) {
        LOGE("OnBlockDiskUevent: rawUeventMsg msg too long, size=%{public}zu", rawUeventMsg.size());
        return E_PARAMS_INVALID;
    }
    if (IsFilePathInvalid(rawUeventMsg)) {
        LOGE("rawUeventMsg is invalid.");
        return E_PARAMS_INVALID;
    }
    return UeventBootstrap::OnBlockDiskUevent(rawUeventMsg);
}

int32_t DiskManagerProvider::GetAllDisks(std::vector<Disk> &vecOfDisk)
{
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("the caller is not sysapp");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_STORAGE_MANAGER)) {
            LOGE("GetAllDisks: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    const int32_t err = DiskManager::GetInstance().GetAllDisks(vecOfDisk);
    LOGI("GetAllDisks count=%{public}zu err=%{public}d", vecOfDisk.size(), err);
    return err;
}

int32_t DiskManagerProvider::GetDiskById(const std::string &diskId, Disk &disk)
{
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("the caller is not sysapp");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("GetDiskById: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    LOGI("GetDiskById diskId=%{public}s.", diskId.c_str());
    if (!IsDiskIdValid(diskId)) {
        LOGE("GetDiskById: diskId is invalid");
        return E_PARAMS_INVALID;
    }
    const int32_t err = DiskManager::GetInstance().GetDiskById(diskId, disk);
    LOGI("GetDiskById diskId=%{public}s err=%{public}d", diskId.c_str(), err);
    return err;
}

int32_t DiskManagerProvider::QueryUsbIsInUse(const std::string &diskPath, bool &isInUse)
{
    LOGI("QueryUsbIsInUse pathLen=%{public}zu", diskPath.size());
    if (!IpcCallerAuth::IsCallingSystemApp()) {
        LOGE("the caller is not sysapp");
        return E_SYS_APP_PERMISSION_DENIED;
    }
    if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
        return E_PERMISSION_DENIED;
    }
    if (!IsMountPathValid(diskPath)) {
        LOGE("mountPath is invalid.");
        return E_PARAMS_INVALID;
    }
    char realPath[PATH_MAX] = {0};
    if (realpath(diskPath.c_str(), realPath) == nullptr) {
        LOGE("DiskManagerProvider::QueryUsbIsInUse realpath failed, errno=%{public}d", errno);
        return E_PARAMS_INVALID;
    }
    isInUse = false;
    const int32_t err = StorageDaemonAdapter::GetInstance().QueryUsbIsInUse(diskPath, isInUse);
    LOGI("QueryUsbIsInUse done err=%{public}d isInUse=%{public}d", err, static_cast<int32_t>(isInUse));
    return err != DiskManagerErrNo::E_OK ? E_QUERY_VOLUME_IN_USE_ERROR : DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::IsUsbFuseByType(int32_t type, bool &isUsbFuse)
{
    LOGI("IsUsbFuseByType type=%{public}d", type);
    static constexpr const char *UNDEFINED_FS_TYPE = "undefined";
    std::string fsTypeStr = UNDEFINED_FS_TYPE;
    auto it = FS_TYPE_MAP.find(type);
    if (it != FS_TYPE_MAP.end()) {
        fsTypeStr = it->second;
    }
    isUsbFuse = UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType(fsTypeStr);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyMtpMounted(const std::string &id,
                                              const std::string &path,
                                              const std::string &desc,
                                              const std::string &uuid,
                                              const std::string &fsType)
{
    LOGI("NotifyMtpMounted id=%{public}s fsType=%{public}s", id.c_str(), fsType.c_str());
    if (!CheckStorageDaemonPermission()) {
        LOGE("DiskManagerProvider CheckStorageDaemonPermission error");
        return E_PERMISSION_DENIED;
    }
    if (!IsMountPathValid(path)) {
        LOGE("mountPath is invalid.");
        return E_PARAMS_INVALID;
    }
    char realPath[PATH_MAX] = {0};
    if (realpath(path.c_str(), realPath) == nullptr) {
        LOGE("DiskManagerProvider::NotifyMtpMounted realpath failed, errno=%{public}d", errno);
        return E_PARAMS_INVALID;
    }
    DiskManager::GetInstance().NotifyMtpMounted(id, path, desc, uuid, fsType);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyMtpUnmounted(const std::string &id, bool isBadRemove)
{
    LOGI("NotifyMtpUnmounted id=%{public}s isBadRemove=%{public}d", id.c_str(), static_cast<int>(isBadRemove));
    if (!CheckStorageDaemonPermission()) {
        LOGE("DiskManagerProvider CheckStorageDaemonPermission error");
        return E_PERMISSION_DENIED;
    }
    DiskManager::GetInstance().NotifyMtpUnmounted(id, isBadRemove);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::Erase(const std::string &volumeId)
{
    LOGI("Erase volumeId=%{public}s", volumeId.c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("Erase: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("Erase: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("Erase: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().Erase(volumeId);
}

int32_t DiskManagerProvider::Eject(const std::string &diskId)
{
    LOGI("Eject diskId=%{public}s", diskId.c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("Eject: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("Eject: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsDiskIdValid(diskId)) {
        LOGE("Eject: diskId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().Eject(diskId);
}

int32_t DiskManagerProvider::CreateIsoImage(const std::string &volumeId, const std::string &filePath)
{
    LOGI("CreateIsoImage volumeId=%{public}s pathLen=%{public}zu", volumeId.c_str(), filePath.size());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("CreateIsoImage: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("CreateIsoImage: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("CreateIsoImage: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    if (IsFilePathInvalid(filePath)) {
        LOGE("CreateIsoImage: filePath is invalid, path traversal detected");
        return E_PARAMS_INVALID;
    }
    char realPath[PATH_MAX] = {0};
    if (realpath(filePath.c_str(), realPath) == nullptr) {
        LOGE("DiskManagerProvider::CreateIsoImage realpath failed, errno=%{public}d", errno);
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().CreateIsoImage(volumeId, filePath);
}

int32_t DiskManagerProvider::Burn(const std::string &volumeId, const std::string &burnOptions)
{
    LOGI("Burn volumeId=%{public}s optsLen=%{public}zu", volumeId.c_str(), burnOptions.size());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("Burn: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("Burn: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("Burn: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    if (IsFilePathInvalid(burnOptions)) {
        LOGE("Burn: burnOptions is invalid, path traversal detected");
        return E_PARAMS_INVALID;
    }
    std::string callerBundle = IpcCallerAuth::GetCallingBundleOrNativeProcessName();
    int32_t callerUserId = IpcCallerAuth::GetCallingUserId();
    return DiskManager::GetInstance().Burn(volumeId, burnOptions, callerBundle, callerUserId);
}

int32_t DiskManagerProvider::GetVolumeOpProcess(const std::string &volumeId, int32_t &progressPct)
{
    LOGI("GetVolumeOpProcess volumeId=%{public}s", volumeId.c_str());
    if (!IsStorageManagerCaller()) {
        if (!IpcCallerAuth::IsCallingSystemApp()) {
            LOGE("GetVolumeOpProcess: caller is not system app");
            return E_SYS_APP_PERMISSION_DENIED;
        }
        if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
            LOGE("GetVolumeOpProcess: permission denied");
            return E_PERMISSION_DENIED;
        }
    }
    if (!IsVolumeIdValid(volumeId)) {
        LOGE("GetVolumeOpProcess: volumeId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().GetVolumeOpProcess(volumeId, progressPct);
}

int32_t DiskManagerProvider::GetPartitionTable(const std::string &diskId, PartitionTableInfo &out)
{
    if (!IpcCallerAuth::IsCallingSystemApp()) {
        LOGE("the caller is not sysapp");
        return E_SYS_APP_PERMISSION_DENIED;
    }
    if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
        return E_PERMISSION_DENIED;
    }
    LOGI("GetPartitionTable diskId=%{public}s.", diskId.c_str());
    if (!IsDiskIdValid(diskId)) {
        LOGE("GetPartitionTable: diskId is invalid");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().GetPartitionTable(diskId, out);
}

int32_t DiskManagerProvider::CreatePartition(const std::string &diskId, const PartitionParams &params)
{
    if (!IpcCallerAuth::IsCallingSystemApp()) {
        LOGE("the caller is not sysapp");
        return E_SYS_APP_PERMISSION_DENIED;
    }
    if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
        return E_PERMISSION_DENIED;
    }
    LOGI("CreatePartition diskId=%{public}s partitionNum=%{public}d.", diskId.c_str(), params.GetPartitionNum());
    if (!IsDiskIdValid(diskId)) {
        LOGE("CreatePartition: diskId is invalid");
        return E_PARAMS_INVALID;
    }
    if (params.GetPartitionNum() <= 0) {
        LOGE("CreatePartition: invalid partitionNum=%{public}d", params.GetPartitionNum());
        return E_PARAMS_INVALID;
    }
    if (params.GetStartSector() <= 0 || params.GetEndSector() <= 0 ||
        params.GetStartSector() >= params.GetEndSector()) {
        LOGE("CreatePartition: invalid sector range");
        return E_PARAMS_INVALID;
    }
    if (params.GetTypeCode().empty()) {
        LOGE("CreatePartition: typeCode is empty");
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().CreatePartition(diskId, params);
}

int32_t DiskManagerProvider::DeletePartition(const std::string &diskId, int32_t partitionNum)
{
    if (!IpcCallerAuth::IsCallingSystemApp()) {
        LOGE("the caller is not sysapp");
        return E_SYS_APP_PERMISSION_DENIED;
    }
    if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
        return E_PERMISSION_DENIED;
    }
    LOGI("DeletePartition diskId=%{public}s partitionNum=%{public}d", diskId.c_str(), partitionNum);
    if (!IsDiskIdValid(diskId)) {
        LOGE("DeletePartition: diskId is invalid");
        return E_PARAMS_INVALID;
    }
    if (partitionNum <= 0) {
        LOGE("DeletePartition: invalid partitionNum=%{public}d", partitionNum);
        return E_PARAMS_INVALID;
    }
    return DiskManager::GetInstance().DeletePartition(diskId, partitionNum);
}

int32_t DiskManagerProvider::FormatPartition(const std::string &diskId, int32_t partitionNum,
                                             const FormatParams &params)
{
    if (!IpcCallerAuth::IsCallingSystemApp()) {
        LOGE("the caller is not sysapp");
        return E_SYS_APP_PERMISSION_DENIED;
    }
    if (!IpcCallerAuth::VerifyCallerPermission(PERMISSION_MOUNT_MANAGER)) {
        return E_PERMISSION_DENIED;
    }
    LOGI("FormatPartition diskId=%{public}s partitionNum=%{public}d fsType=%{public}s", diskId.c_str(),
         partitionNum, params.GetFsType().c_str());
    if (!IsDiskIdValid(diskId)) {
        LOGE("FormatPartition: diskId is invalid");
        return E_PARAMS_INVALID;
    }
    if (partitionNum <= 0) {
        LOGE("FormatPartition: invalid partitionNum=%{public}d", partitionNum);
        return E_PARAMS_INVALID;
    }
    if (params.GetFsType().empty()) {
        LOGE("FormatPartition: fsType is empty");
        return E_PARAMS_INVALID;
    }
    if (!params.GetQuickFormat()) {
        LOGE("FormatPartition: quickFormat is invalid");
        return E_PARAMS_INVALID;
    }
    if (params.GetVolumeName().empty() || params.GetVolumeName().size() > VOLUME_NAME_MAX_LEN) {
        LOGE("FormatPartition: volumeName is empty or length exceeds %{public}zu limit",
             static_cast<size_t>(VOLUME_NAME_MAX_LEN));
        return E_PARAMS_INVALID;
    }
    int32_t ret = DiskManager::GetInstance().FormatPartition(diskId, partitionNum, params);
    LOGI("FormatPartition done ret=%{public}d", ret);
    return ret;
}
} // namespace DiskManager
} // namespace OHOS