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

#include <cinttypes>
#include <iservice_registry.h>
#include <system_ability_definition.h>

#include "disk_manager_hilog.h"
#include "errors.h"

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

int32_t StorageDaemonAdapter::QueryUsbIsInUse(const std::string &diskPath, bool &isInUse)
{
    LOGI("QueryUsbIsInUse enter, diskPath=%{public}s", diskPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("QueryUsbIsInUse exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->QueryUsbIsInUse(diskPath, isInUse);
    LOGI("QueryUsbIsInUse exit ret=%{public}d isInUse=%{public}d", ret, static_cast<int32_t>(isInUse));
    return ret;
}

int32_t StorageDaemonAdapter::CreateBlockDeviceNode(const std::string &devPath,
                                                    uint32_t mode,
                                                    int32_t major,
                                                    int32_t minor)
{
    LOGI("CreateBlockDeviceNode enter, devPath=%{public}s, mode=%{public}u, major=%{public}d, minor=%{public}d",
         devPath.c_str(), mode, major, minor);
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("CreateBlockDeviceNode exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->CreateBlockDeviceNode(devPath, mode, major, minor);
    LOGI("CreateBlockDeviceNode exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::DestroyBlockDeviceNode(const std::string &devPath)
{
    LOGI("DestroyBlockDeviceNode enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("DestroyBlockDeviceNode exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->DestroyBlockDeviceNode(devPath);
    LOGI("DestroyBlockDeviceNode exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::ReadPartitionTable(const std::string &devPath, std::string &output, int32_t &maxVolume)
{
    LOGI("ReadPartitionTable enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("ReadPartitionTable exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->ReadPartitionTable(devPath, output, maxVolume);
    LOGI("ReadPartitionTable exit ret=%{public}d outputLen=%{public}zu maxVolume=%{public}d", ret, output.size(),
         maxVolume);
    return ret;
}

int32_t StorageDaemonAdapter::Eject(const std::string &devPath)
{
    LOGI("Eject enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("Eject exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->Eject(devPath);
    LOGI("Eject exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::GetCDStatus(const std::string &devPath, int32_t &status)
{
    LOGI("GetCDStatus enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("GetCDStatus exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->GetCDStatus(devPath, status);
    LOGI("GetCDStatus exit ret=%{public}d status=%{public}d", ret, status);
    return ret;
}

int32_t StorageDaemonAdapter::Mount(const std::string &devPath,
                                    const std::string &mountPath,
                                    const std::string &fsType,
                                    uint32_t mountFlag)
{
    LOGI("Mount enter, devPath=%{public}s, mountPath=%{public}s, fsType=%{public}s, mountFlag=%{public}u",
         devPath.c_str(), mountPath.c_str(), fsType.c_str(), mountFlag);
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("Mount exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->Mount(devPath, mountPath, fsType, mountFlag);
    LOGI("Mount exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::Unmount(const std::string &mountPath, const std::string &fsType, bool force)
{
    LOGI("Unmount enter, mountPath=%{public}s, force=%{public}d", mountPath.c_str(), static_cast<int32_t>(force));
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("Unmount exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->Unmount(mountPath, fsType, force);
    LOGI("Unmount exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::FormatVolume(const std::string &devPath, const std::string &fsType)
{
    LOGI("FormatVolume enter, devPath=%{public}s, fsType=%{public}s", devPath.c_str(), fsType.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("FormatVolume exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->FormatVolume(devPath, fsType);
    LOGI("FormatVolume exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::Check(const std::string &devPath, const std::string &fsType, bool autoFix)
{
    LOGI("Check enter, devPath=%{public}s, fsType=%{public}s, autoFix=%{public}d", devPath.c_str(), fsType.c_str(),
         static_cast<int32_t>(autoFix));
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("Check exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->Check(devPath, fsType, autoFix);
    LOGI("Check exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::Repair(const std::string &devPath, const std::string &fsType)
{
    LOGI("Repair enter, devPath=%{public}s, fsType=%{public}s", devPath.c_str(), fsType.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("Repair exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->Repair(devPath, fsType);
    LOGI("Repair exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::SetLabel(const std::string &devPath, const std::string &fsType, const std::string &label)
{
    LOGI("SetLabel enter, devPath=%{public}s, fsType=%{public}s, label=%{public}s", devPath.c_str(), fsType.c_str(),
         label.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("SetLabel exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->SetLabel(devPath, fsType, label);
    LOGI("SetLabel exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::ReadMetadata(const std::string &devPath,
                                           std::string &uuid,
                                           std::string &type,
                                           std::string &label)
{
    LOGI("ReadMetadata enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("ReadMetadata exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->ReadMetadata(devPath, uuid, type, label);
    LOGI("ReadMetadata exit ret=%{public}d uuidLen=%{public}zu type=%{public}s labelLen=%{public}zu", ret, uuid.size(),
         type.c_str(), label.size());
    return ret;
}

int32_t StorageDaemonAdapter::GetCapacity(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize)
{
    LOGI("GetCapacity enter, mountPath=%{public}s", mountPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("GetCapacity exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->GetCapacity(mountPath, totalSize, freeSize);
    LOGI("GetCapacity exit ret=%{public}d totalSize=%{public}" PRId64 " freeSize=%{public}" PRId64, ret, totalSize,
         freeSize);
    return ret;
}

int32_t StorageDaemonAdapter::MountFuseDevice(const std::string &mountPath, int32_t &fuseFd)
{
    LOGI("MountFuseDevice enter, mountPath=%{public}s", mountPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("MountFuseDevice exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->MountFuseDevice(mountPath, fuseFd);
    LOGI("MountFuseDevice exit ret=%{public}d fuseFd=%{public}d", ret, fuseFd);
    return ret;
}

int32_t StorageDaemonAdapter::Partition(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags)
{
    LOGI("Partition enter, diskPath=%{public}s, partitionType=%{public}d, partitionFlags=%{public}u", diskPath.c_str(),
         partitionType, partitionFlags);
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("Partition exit err=%{public}d (proxy not ready)", err);
        return err;
    }
    const int32_t ret = storageDaemon_->Partition(diskPath, partitionType, partitionFlags);
    LOGI("Partition exit ret=%{public}d", ret);
    return ret;
}

int32_t StorageDaemonAdapter::GetBlockInfoByType(const std::string &type, std::string &blockInfos)
{
    LOGI("GetBlockInfoByType enter, type=%{public}s", type.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        LOGE("GetBlockInfoByType exit err=%{public}d (proxy not ready)", err);
        blockInfos.clear();
        return err;
    }
    const ErrCode ret = storageDaemon_->GetBlockInfoByType(type, blockInfos);
    LOGI("GetBlockInfoByType exit ret=%{public}d bytes=%{public}zu",
         static_cast<int32_t>(ret), blockInfos.size());
    return static_cast<int32_t>(ret);
}
} // namespace DiskManager
} // namespace OHOS
