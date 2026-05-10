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

#include <chrono>
#include <condition_variable>
#include <iservice_registry.h>
#include <new>
#include <system_ability_definition.h>

#include "idisk_manager.h"
#include "system_ability_load_callback_stub.h"

#include "disk_manager_hilog.h"
#include "disk_manager_napi_errno.h"

namespace OHOS {
namespace DiskManager {

namespace {
constexpr int32_t SA_LOAD_WAIT_TIMEOUT_MS = 30000;
constexpr int32_t IPC_OK = 0;

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

class DmLoadCallback final : public SystemAbilityLoadCallbackStub {
public:
    void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject) override
    {
        (void)systemAbilityId;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_ = true;
            success_ = true;
            remoteObject_ = remoteObject;
        }
        cv_.notify_all();
    }

    void OnLoadSystemAbilityFail(int32_t systemAbilityId) override
    {
        (void)systemAbilityId;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            finished_ = true;
            success_ = false;
        }
        cv_.notify_all();
    }

    bool WaitResult(int32_t timeoutMs)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() { return finished_; });
    }

    bool IsSuccess() const
    {
        return success_;
    }

    sptr<IRemoteObject> GetRemoteObject() const
    {
        return remoteObject_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool finished_ = false;
    bool success_ = false;
    sptr<IRemoteObject> remoteObject_ = nullptr;
};

int32_t GetDiskManagerSaObject(ISystemAbilityManager &mgr, sptr<IRemoteObject> &object)
{
    object = mgr.GetSystemAbility(DISK_MANAGER_SA_ID);
    if (object != nullptr) {
        return E_OK;
    }
    sptr<DmLoadCallback> loadCallback = new (std::nothrow) DmLoadCallback();
    if (loadCallback == nullptr) {
        LOGE("GetDiskManagerSaObject load callback null");
        return E_SERVICE_IS_NULLPTR;
    }
    int32_t loadRet = mgr.LoadSystemAbility(DISK_MANAGER_SA_ID, loadCallback);
    if (loadRet != IPC_OK) {
        LOGE("GetDiskManagerSaObject LoadSystemAbility failed ret=%{public}d", loadRet);
        return E_REMOTE_IS_NULLPTR;
    }
    bool callbackNotified = loadCallback->WaitResult(SA_LOAD_WAIT_TIMEOUT_MS);
    if (!callbackNotified || !loadCallback->IsSuccess()) {
        LOGE("GetDiskManagerSaObject wait callback failed timeout=%{public}dms", SA_LOAD_WAIT_TIMEOUT_MS);
        return E_REMOTE_IS_NULLPTR;
    }
    object = loadCallback->GetRemoteObject();
    if (object == nullptr) {
        LOGE("GetDiskManagerSaObject object == nullptr");
        return E_REMOTE_IS_NULLPTR;
    }
    return E_OK;
}

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
        sptr<IRemoteObject> object = nullptr;
        int32_t saRet = GetDiskManagerSaObject(mgr, object);
        if (saRet != E_OK) {
            return saRet;
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
        sptr<IRemoteObject> remote = diskManager_->AsObject();
        if (remote != nullptr) {
            remote->AddDeathRecipient(deathRecipient_);
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

int32_t DiskManagerClient::GetAllDisks(std::vector<Disk> &vecOfDisk)
{
    LOGI("GetAllDisks enter");
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetAllDisks(vecOfDisk);
}

int32_t DiskManagerClient::GetDiskById(const std::string &diskId, Disk &disk)
{
    LOGI("GetDiskById diskId=%{public}s", diskId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetDiskById(diskId, disk);
}

int32_t DiskManagerClient::EraseVolume(const std::string &volumeId)
{
    LOGI("EraseVolume volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.EraseVolume(volumeId);
}

int32_t DiskManagerClient::EjectVolume(const std::string &volumeId)
{
    LOGI("EjectVolume volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.EjectVolume(volumeId);
}

int32_t DiskManagerClient::CreateIsoImage(const std::string &volumeId, const std::string &filePath)
{
    LOGI("CreateIsoImage volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.CreateIsoImage(volumeId, filePath);
}

int32_t DiskManagerClient::BurnVolume(const std::string &volumeId, const std::string &burnOptions)
{
    LOGI("BurnVolume volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.BurnVolume(volumeId, burnOptions);
}

int32_t DiskManagerClient::GetVolumeOpProcess(const std::string &volumeId, int32_t &progressPct)
{
    LOGI("GetVolumeOpProcess volumeId=%{public}s", volumeId.c_str());
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.GetVolumeOpProcess(volumeId, progressPct);
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

int32_t DiskManagerClient::VerifyBurnData(const std::string &volumeId, int32_t verifyType)
{
    LOGI("VerifyBurnData volumeId=%{public}s type=%{public}d", volumeId.c_str(), verifyType);
    sptr<IDiskManager> proxy;
    int32_t err = Connect(proxy);
    if (err != E_OK) {
        return err;
    }
    IDiskManager &dm = *proxy;
    return dm.VerifyBurnData(volumeId, verifyType);
}

} // namespace DiskManager
} // namespace OHOS
