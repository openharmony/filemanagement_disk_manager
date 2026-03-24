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
     (void)volumeId;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::Unmount(const std::string &volumeId)
 {
     (void)volumeId;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::Format(const std::string &volumeId, const std::string &fsType)
 {
     (void)volumeId;
     (void)fsType;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::TryToFix(const std::string &volumeId)
 {
     (void)volumeId;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
 {
     (void)fsUuid;
     (void)description;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetAllVolumes(std::vector<VolumeExternal> &vecOfVol)
 {
     vecOfVol.clear();
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &vc)
 {
     (void)fsUuid;
     (void)vc;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetVolumeById(const std::string &volumeId, VolumeExternal &vc)
 {
     (void)volumeId;
     (void)vc;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize)
 {
     (void)volumeUuid;
     freeSize = 0;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize)
 {
     (void)volumeUuid;
     totalSize = 0;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::Partition(const std::string &diskId, int32_t type)
 {
     (void)diskId;
     (void)type;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetAllDisks(std::vector<Disk> &vecOfDisk)
 {
     vecOfDisk.clear();
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetDiskById(const std::string &diskId, Disk &disk)
 {
     (void)diskId;
     (void)disk;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::QueryUsbIsInUse(const std::string &diskPath, bool &isInUse)
 {
     (void)diskPath;
     isInUse = false;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyDiskCreated(const Disk &disk)
 {
     (void)disk;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyDiskDestroyed(const std::string &diskId)
 {
     (void)diskId;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyVolumeCreated(const VolumeCore &vc)
 {
     (void)vc;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyVolumeMounted(const VolumeInfoStr &volumeInfoStr)
 {
     (void)volumeInfoStr;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyVolumeStateChanged(const std::string &volumeId, uint32_t state)
 {
     (void)volumeId;
     (void)state;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyVolumeDamaged(const VolumeInfoStr &volumeInfoStr)
 {
     (void)volumeInfoStr;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyMtpMounted(const std::string &id,
     const std::string &path,
     const std::string &desc,
     const std::string &uuid,
     const std::string &fsType)
 {
     (void)id;
     (void)path;
     (void)desc;
     (void)uuid;
     (void)fsType;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyMtpUnmounted(const std::string &id, bool isBadRemove)
 {
     (void)id;
     (void)isBadRemove;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::NotifyEncryptVolumeStateChanged(const VolumeInfoStr &volumeInfoStr)
 {
     (void)volumeInfoStr;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::Encrypt(const std::string &volumeId, const std::string &pazzword)
 {
     (void)volumeId;
     (void)pazzword;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetCryptProgressById(const std::string &volumeId, int32_t &progress)
 {
     (void)volumeId;
     progress = 0;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::GetCryptUuidById(const std::string &volumeId, std::string &uuid)
 {
     (void)volumeId;
     uuid.clear();
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::BindRecoverKeyToPasswd(
     const std::string &volumeId, const std::string &pazzword, const std::string &recoverKey)
 {
     (void)volumeId;
     (void)pazzword;
     (void)recoverKey;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::UpdateCryptPasswd(
     const std::string &volumeId, const std::string &pazzword, const std::string &newPazzword)
 {
     (void)volumeId;
     (void)pazzword;
     (void)newPazzword;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::ResetCryptPasswd(
     const std::string &volumeId, const std::string &recoverKey, const std::string &newPazzword)
 {
     (void)volumeId;
     (void)recoverKey;
     (void)newPazzword;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::VerifyCryptPasswd(const std::string &volumeId, const std::string &pazzword)
 {
     (void)volumeId;
     (void)pazzword;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::Unlock(const std::string &volumeId, const std::string &pazzword)
 {
     (void)volumeId;
     (void)pazzword;
     return DiskManagerErrNo::E_OK;
 }
 
 int32_t DiskManagerProvider::Decrypt(const std::string &volumeId, const std::string &pazzword)
 {
     (void)volumeId;
     (void)pazzword;
     return DiskManagerErrNo::E_OK;
 }
 
 } // namespace DiskManager
 } // namespace OHOS