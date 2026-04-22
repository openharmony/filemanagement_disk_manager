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

#include "uevent_bootstrap.h"
#include "usb_fuse_adapter.h"
#include "disk_manager.h"
#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {

using namespace OHOS::DiskManager;

REGISTER_SYSTEM_ABILITY_BY_ID(DiskManagerProvider, DISK_MANAGER_SA_ID, false);

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
    return DiskDataManager::GetInstance().Mount(volumeId);
}

int32_t DiskManagerProvider::Unmount(const std::string &volumeId)
{
    LOGI("Unmount volumeId=%{public}s", volumeId.c_str());
    return DiskDataManager::GetInstance().Unmount(volumeId);
}

int32_t DiskManagerProvider::Format(const std::string &volumeId, const std::string &fsType)
{
    LOGI("Format volumeId=%{public}s fsType=%{public}s", volumeId.c_str(), fsType.c_str());
    return DiskDataManager::GetInstance().Format(volumeId, fsType);
}

int32_t DiskManagerProvider::TryToFix(const std::string &volumeId)
{
    LOGI("TryToFix volumeId=%{public}s", volumeId.c_str());
    return DiskDataManager::GetInstance().TryToFix(volumeId);
}

int32_t DiskManagerProvider::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    LOGI("SetVolumeDescription fsUuid=%{public}s", fsUuid.c_str());
    return DiskDataManager::GetInstance().SetVolumeDescription(fsUuid, description);
}

int32_t DiskManagerProvider::GetAllVolumes(std::vector<VolumeExternal> &vecOfVol)
{
    int32_t err = DiskDataManager::GetInstance().GetAllVolumes(vecOfVol);
    LOGI("GetAllVolumes count=%{public}zu err=%{public}d", vecOfVol.size(), err);
    return err;
}

int32_t DiskManagerProvider::GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &vc)
{
    const int32_t err = DiskDataManager::GetInstance().GetVolumeByUuid(fsUuid, vc);
    LOGI("GetVolumeByUuid fsUuid=%{public}s err=%{public}d", fsUuid.c_str(), err);
    return err;
}

int32_t DiskManagerProvider::GetVolumeById(const std::string &volumeId, VolumeExternal &vc)
{
    const int32_t err = DiskDataManager::GetInstance().GetVolumeById(volumeId, vc);
    LOGI("GetVolumeById volumeId=%{public}s err=%{public}d", volumeId.c_str(), err);
    return err;
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
    return DiskDataManager::GetInstance().Partition(diskId, type);
}

int32_t DiskManagerProvider::OnBlockDiskUevent(const std::string &rawUeventMsg)
{
    LOGI("OnBlockDiskUevent len=%{public}zu", rawUeventMsg.size());
    return UeventBootstrap::OnBlockDiskUevent(rawUeventMsg);
}

int32_t DiskManagerProvider::GetAllDisks(std::vector<Disk> &vecOfDisk)
{
    const int32_t err = DiskDataManager::GetInstance().GetAllDisks(vecOfDisk);
    LOGI("GetAllDisks count=%{public}zu err=%{public}d", vecOfDisk.size(), err);
    return err;
}

int32_t DiskManagerProvider::GetDiskById(const std::string &diskId, Disk &disk)
{
    const int32_t err = DiskDataManager::GetInstance().GetDiskById(diskId, disk);
    LOGI("GetDiskById diskId=%{public}s err=%{public}d", diskId.c_str(), err);
    return err;
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
    static constexpr const char *UNDEFINED_FS_TYPE = "undefined";
    std::string fsTypeStr = UNDEFINED_FS_TYPE;
    auto it = FS_TYPE_MAP.find(type);
    if (it != FS_TYPE_MAP.end()) {
        fsTypeStr = it->second;
    }
    isUsbFuse = UsbFuseAdapter::GetInstance().IsUsbFuseByType(fsTypeStr);
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

} // namespace DiskManager
} // namespace OHOS