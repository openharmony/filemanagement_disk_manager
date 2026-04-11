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

#include "storage_daemon_adapter.h"
#include "disk_manager_errno.h"

#include <iservice_registry.h>
#include <system_ability_definition.h>

#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {

StorageDaemonAdapter &StorageDaemonAdapter::GetInstance()
{
    static StorageDaemonAdapter instance;
    return instance;
}

StorageDaemonAdapter::StorageDaemonAdapter()
{
    storageDaemon_ = nullptr;
}

StorageDaemonAdapter::~StorageDaemonAdapter() {}

int32_t StorageDaemonAdapter::Connect()
{
    LOGD("Connect start");
    std::lock_guard<std::mutex> lock(mutex_);
    if (storageDaemon_ == nullptr) {
        auto sam = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (sam == nullptr) {
            LOGE("Connect samgr nullptr");
            return E_SA_IS_NULLPTR;
        }
        auto object = sam->GetSystemAbility(STORAGE_MANAGER_DAEMON_ID);
        if (object == nullptr) {
            LOGE("Connect remote object nullptr");
            return E_REMOTE_IS_NULLPTR;
        }
        storageDaemon_ = iface_cast<StorageDaemon::IStorageDaemon>(object);
        if (storageDaemon_ == nullptr) {
            LOGE("Connect iface_cast failed");
            return E_SERVICE_IS_NULLPTR;
        }
        deathRecipient_ = new (std::nothrow) SdDeathRecipient();
        if (deathRecipient_ == nullptr) {
            LOGE("Connect death recipient OOM");
            return E_DEATH_RECIPIENT_IS_NULLPTR;
        }
        storageDaemon_->AsObject()->AddDeathRecipient(deathRecipient_);
    }
    LOGD("Connect end");
    return E_OK;
}

int32_t StorageDaemonAdapter::EnsureProxyReady()
{
    int32_t err = Connect();
    if (err != E_OK) {
        return err;
    }
    if (storageDaemon_ == nullptr) {
        return E_SERVICE_IS_NULLPTR;
    }
    return E_OK;
}

int32_t StorageDaemonAdapter::ResetSdProxy()
{
    LOGI("ResetSdProxy");
    std::lock_guard<std::mutex> lock(mutex_);
    if (storageDaemon_ != nullptr && storageDaemon_->AsObject() != nullptr) {
        storageDaemon_->AsObject()->RemoveDeathRecipient(deathRecipient_);
    }
    storageDaemon_ = nullptr;
    return E_OK;
}

void SdDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    (void)remote;
    LOGE("storage_daemon OnRemoteDied");
    StorageDaemonAdapter::GetInstance().ResetSdProxy();
}

int32_t StorageDaemonAdapter::Mount(std::string volumeId, int32_t flag)
{
    LOGI("StorageDaemonAdapter::Mount");
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Mount(volumeId, static_cast<uint32_t>(flag));
}

int32_t StorageDaemonAdapter::Unmount(std::string volumeId)
{
    LOGI("StorageDaemonAdapter::Unmount volumeId = %{public}s", volumeId.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->UMount(volumeId);
}

int32_t StorageDaemonAdapter::TryToFix(std::string volumeId, int32_t flag)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->TryToFix(volumeId, static_cast<uint32_t>(flag));
}

int32_t StorageDaemonAdapter::Check(std::string volumeId)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Check(volumeId);
}

int32_t StorageDaemonAdapter::Partition(std::string diskId, int32_t type)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Partition(diskId, type);
}

int32_t StorageDaemonAdapter::Format(std::string volumeId, std::string type)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Format(volumeId, type);
}

int32_t StorageDaemonAdapter::SetVolumeDescription(std::string volumeId, std::string description)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->SetVolumeDescription(volumeId, description);
}

int32_t StorageDaemonAdapter::QueryUsbIsInUse(const std::string &diskPath, bool &isInUse)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->QueryUsbIsInUse(diskPath, isInUse);
}

int32_t StorageDaemonAdapter::MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->MountUsbFuse(volumeId, fsUuid, fuseFd);
}

} // namespace DiskManager
} // namespace OHOS
