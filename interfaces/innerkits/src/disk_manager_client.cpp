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

#include "disk_manager_client.h"

#include <iservice_registry.h>
#include <new>
#include <system_ability_definition.h>

#include "idisk_manager.h"

#include "disk_manager_hilog.h"
#include "disk_manager_napi_errno.h"

namespace OHOS {
namespace DiskManager {

namespace {
class DmDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    DmDeathRecipient() = default;
    ~DmDeathRecipient() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        (void)remote;
        DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
        client.ResetProxy();
    }
};
} // namespace

DiskManagerClient::DiskManagerClient() {}

DiskManagerClient::~DiskManagerClient()
{
    ResetProxy();
}

int32_t DiskManagerClient::Connect(sptr<IDiskManager> &proxy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (diskManager_ == nullptr) {
        SystemAbilityManagerClient &samClient = SystemAbilityManagerClient::GetInstance();
        sptr<ISystemAbilityManager> sam = samClient.GetSystemAbilityManager();
        if (sam == nullptr) {
            LOGE("DiskManagerClient::Connect samgr == nullptr");
            return E_SA_IS_NULLPTR;
        }
        ISystemAbilityManager &mgr = *sam;
        sptr<IRemoteObject> object = mgr.GetSystemAbility(DISK_MANAGER_SA_ID);
        if (object == nullptr) {
            LOGE("DiskManagerClient::Connect object == nullptr");
            return E_REMOTE_IS_NULLPTR;
        }
        diskManager_ = iface_cast<IDiskManager>(object);
        if (diskManager_ == nullptr) {
            LOGE("DiskManagerClient::Connect iface_cast IDiskManager failed");
            return E_SERVICE_IS_NULLPTR;
        }
        deathRecipient_ = new (std::nothrow) DmDeathRecipient();
        if (deathRecipient_ == nullptr) {
            LOGE("DiskManagerClient::Connect death recipient null");
            diskManager_ = nullptr;
            return E_SERVICE_IS_NULLPTR;
        }
        sptr<IRemoteObject> ao = diskManager_->AsObject();
        if (ao != nullptr) {
            ao->AddDeathRecipient(deathRecipient_);
        }
    }
    proxy = diskManager_;
    if (proxy == nullptr) {
        return E_SERVICE_IS_NULLPTR;
    }
    return E_OK;
}

int32_t DiskManagerClient::ResetProxy()
{
    LOGI("ResetProxy");
    std::lock_guard<std::mutex> lock(mutex_);
    if (diskManager_ != nullptr && deathRecipient_ != nullptr) {
        sptr<IRemoteObject> ao = diskManager_->AsObject();
        if (ao != nullptr) {
            ao->RemoveDeathRecipient(deathRecipient_);
        }
    }
    diskManager_ = nullptr;
    deathRecipient_ = nullptr;
    return E_OK;
}

int32_t DiskManagerClient::Mount(const std::string &volumeId)
{
    LOGI("Mount volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.Mount(volumeId);
}

int32_t DiskManagerClient::Unmount(const std::string &volumeId)
{
    LOGI("Unmount volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.Unmount(volumeId);
}

int32_t DiskManagerClient::Format(const std::string &volumeId, const std::string &fsType)
{
    LOGI("Format volumeId=%{public}s fsType=%{public}s", volumeId.c_str(), fsType.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.Format(volumeId, fsType);
}

int32_t DiskManagerClient::SetVolumeDescription(const std::string &fsUuid, const std::string &description)
{
    LOGI("SetVolumeDescription fsUuid=%{public}s", fsUuid.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.SetVolumeDescription(fsUuid, description);
}

int32_t DiskManagerClient::GetAllVolumes(std::vector<VolumeExternal> &vecOfVol)
{
    LOGI("GetAllVolumes enter");
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetAllVolumes(vecOfVol);
}

int32_t DiskManagerClient::GetVolumeByUuid(const std::string &uuid, VolumeExternal &vc)
{
    LOGI("GetVolumeByUuid uuid=%{public}s", uuid.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetVolumeByUuid(uuid, vc);
}

int32_t DiskManagerClient::GetVolumeById(const std::string &volumeId, VolumeExternal &vc)
{
    LOGI("GetVolumeById volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetVolumeById(volumeId, vc);
}

int32_t DiskManagerClient::GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize)
{
    LOGI("GetFreeSizeOfVolume volumeUuid=%{public}s", volumeUuid.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetFreeSizeOfVolume(volumeUuid, freeSize);
}

int32_t DiskManagerClient::GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize)
{
    LOGI("GetTotalSizeOfVolume volumeUuid=%{public}s", volumeUuid.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetTotalSizeOfVolume(volumeUuid, totalSize);
}

int32_t DiskManagerClient::Partition(const std::string &diskId, int32_t type)
{
    LOGI("Partition diskId=%{public}s type=%{public}d", diskId.c_str(), type);
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.Partition(diskId, type);
}

int32_t DiskManagerClient::OnBlockDiskUevent(const std::string &rawUeventMsg)
{
    LOGI("OnBlockDiskUevent len=%{public}zu", rawUeventMsg.size());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.OnBlockDiskUevent(rawUeventMsg);
}

} // namespace DiskManager
} // namespace OHOS
