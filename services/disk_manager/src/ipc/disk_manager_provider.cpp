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

#include "ipc/disk_manager_provider.h"

#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {

using namespace OHOS::DiskManager;

REGISTER_SYSTEM_ABILITY_BY_ID(DiskManagerProvider, DISK_MANAGER_SA_ID, true);

DiskManagerProvider::DiskManagerProvider(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate) {}

void DiskManagerProvider::OnStart()
{
    LOGI("OnStart begin");
    const bool published = SystemAbility::Publish(this);
    if (!published) {
        LOGE("Publish failed");
    }
    LOGI("OnStart end");
}

void DiskManagerProvider::OnStop()
{
    LOGI("OnStop");
}

int32_t DiskManagerProvider::Mount(const std::string &volumeId)
{
    LOGI("Mount volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::Unmount(const std::string &volumeId)
{
    LOGI("Unmount volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::Format(const std::string &volumeId, const std::string &fsType)
{
    LOGI("Format volumeId=%{public}s fsType=%{public}s", volumeId.c_str(), fsType.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::TryToFix(const std::string &volumeId)
{
    LOGI("TryToFix volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    (void)description;
    LOGI("SetVolumeDescription fsUuid=%{public}s", fsUuid.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetAllVolumes(std::vector<VolumeExternal> &vecOfVol)
{
    LOGI("GetAllVolumes vecSize=%{public}zu", vecOfVol.size());
    vecOfVol.clear();
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &vc)
{
    (void)vc;
    LOGI("GetVolumeByUuid fsUuid=%{public}s", fsUuid.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetVolumeById(const std::string &volumeId, VolumeExternal &vc)
{
    (void)vc;
    LOGI("GetVolumeById volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize)
{
    LOGI("GetFreeSizeOfVolume volumeUuid=%{public}s", volumeUuid.c_str());
    freeSize = 0;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize)
{
    LOGI("GetTotalSizeOfVolume volumeUuid=%{public}s", volumeUuid.c_str());
    totalSize = 0;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::Partition(const std::string &diskId, int32_t type)
{
    LOGI("Partition diskId=%{public}s type=%{public}d", diskId.c_str(), type);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetAllDisks(std::vector<Disk> &vecOfDisk)
{
    LOGI("GetAllDisks vecSize=%{public}zu", vecOfDisk.size());
    vecOfDisk.clear();
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetDiskById(const std::string &diskId, Disk &disk)
{
    (void)disk;
    LOGI("GetDiskById diskId=%{public}s", diskId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::QueryUsbIsInUse(const std::string &diskPath, bool &isInUse)
{
    LOGI("QueryUsbIsInUse pathLen=%{public}zu", diskPath.size());
    isInUse = false;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::IsUsbFuseByType(int32_t type, bool &isUsbFuse)
{
    LOGI("IsUsbFuseByType type=%{public}d", type);
    isUsbFuse = false;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyDiskCreated(const Disk &disk)
{
    LOGI("NotifyDiskCreated diskId=%{public}s sizeBytes=%{public}lld flag=%{public}d", disk.GetDiskId().c_str(),
         static_cast<long long>(disk.GetSizeBytes()), disk.GetFlag());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyDiskDestroyed(const std::string &diskId)
{
    LOGI("NotifyDiskDestroyed diskId=%{public}s", diskId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyVolumeCreated(const VolumeCore &vc)
{
    LOGI("NotifyVolumeCreated id=%{public}s type=%{public}d diskId=%{public}s state=%{public}d", vc.GetId().c_str(),
         vc.GetType(), vc.GetDiskId().c_str(), vc.GetState());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyVolumeMounted(const VolumeInfoStr &volumeInfoStr)
{
    LOGI("NotifyVolumeMounted volumeId=%{public}s fsTypeStr=%{public}s isDamaged=%{public}d",
         volumeInfoStr.volumeId.c_str(), volumeInfoStr.fsTypeStr.c_str(), static_cast<int>(volumeInfoStr.isDamaged));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyVolumeStateChanged(const std::string &volumeId, uint32_t state)
{
    LOGI("NotifyVolumeStateChanged volumeId=%{public}s state=%{public}u", volumeId.c_str(), state);
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyVolumeDamaged(const VolumeInfoStr &volumeInfoStr)
{
    LOGI("NotifyVolumeDamaged volumeId=%{public}s fsTypeStr=%{public}s isDamaged=%{public}d",
         volumeInfoStr.volumeId.c_str(), volumeInfoStr.fsTypeStr.c_str(), static_cast<int>(volumeInfoStr.isDamaged));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyMtpMounted(const std::string &id,
                                              const std::string &path,
                                              const std::string &desc,
                                              const std::string &uuid,
                                              const std::string &fsType)
{
    (void)path;
    (void)desc;
    (void)uuid;
    LOGI("NotifyMtpMounted id=%{public}s fsType=%{public}s", id.c_str(), fsType.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyMtpUnmounted(const std::string &id, bool isBadRemove)
{
    LOGI("NotifyMtpUnmounted id=%{public}s isBadRemove=%{public}d", id.c_str(), static_cast<int>(isBadRemove));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::NotifyEncryptVolumeStateChanged(const VolumeInfoStr &volumeInfoStr)
{
    LOGI("NotifyEncryptVolumeStateChanged volumeId=%{public}s fsTypeStr=%{public}s isDamaged=%{public}d",
         volumeInfoStr.volumeId.c_str(), volumeInfoStr.fsTypeStr.c_str(), static_cast<int>(volumeInfoStr.isDamaged));
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::Encrypt(const std::string &volumeId, const std::string &pazzword)
{
    (void)pazzword;
    LOGI("Encrypt volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetCryptProgressById(const std::string &volumeId, int32_t &progress)
{
    LOGI("GetCryptProgressById volumeId=%{public}s", volumeId.c_str());
    progress = 0;
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::GetCryptUuidById(const std::string &volumeId, std::string &uuid)
{
    LOGI("GetCryptUuidById volumeId=%{public}s", volumeId.c_str());
    uuid.clear();
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::BindRecoverKeyToPasswd(const std::string &volumeId,
                                                    const std::string &pazzword,
                                                    const std::string &recoverKey)
{
    (void)pazzword;
    (void)recoverKey;
    LOGI("BindRecoverKeyToPasswd volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::UpdateCryptPasswd(const std::string &volumeId,
                                               const std::string &pazzword,
                                               const std::string &newPazzword)
{
    (void)pazzword;
    (void)newPazzword;
    LOGI("UpdateCryptPasswd volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::ResetCryptPasswd(const std::string &volumeId,
                                              const std::string &recoverKey,
                                              const std::string &newPazzword)
{
    (void)recoverKey;
    (void)newPazzword;
    LOGI("ResetCryptPasswd volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::VerifyCryptPasswd(const std::string &volumeId, const std::string &pazzword)
{
    (void)pazzword;
    LOGI("VerifyCryptPasswd volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::Unlock(const std::string &volumeId, const std::string &pazzword)
{
    (void)pazzword;
    LOGI("Unlock volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t DiskManagerProvider::Decrypt(const std::string &volumeId, const std::string &pazzword)
{
    (void)pazzword;
    LOGI("Decrypt volumeId=%{public}s", volumeId.c_str());
    return DiskManagerErrNo::E_OK;
}

} // namespace DiskManager
} // namespace OHOS