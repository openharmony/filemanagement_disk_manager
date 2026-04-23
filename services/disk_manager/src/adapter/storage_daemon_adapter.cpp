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
        return err;
    }
    return storageDaemon_->QueryUsbIsInUse(diskPath, isInUse);
}

int32_t StorageDaemonAdapter::MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd)
{
    LOGI("MountUsbFuse enter, volumeId=%{public}s", volumeId.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->MountUsbFuse(volumeId, fsUuid, fuseFd);
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
        return err;
    }
    return storageDaemon_->CreateBlockDeviceNode(devPath, mode, major, minor);
}

int32_t StorageDaemonAdapter::DestroyBlockDeviceNode(const std::string &devPath)
{
    LOGI("DestroyBlockDeviceNode enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->DestroyBlockDeviceNode(devPath);
}

int32_t StorageDaemonAdapter::ReadPartitionTable(const std::string &devPath, std::string &output, int32_t &maxVolume)
{
    LOGI("ReadPartitionTable enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->ReadPartitionTable(devPath, output, maxVolume);
}

int32_t StorageDaemonAdapter::ReadVolumeMetaData(const std::string &devPath,
                                                 std::string &fsUuid,
                                                 std::string &fsType,
                                                 std::string &fsLabel)
{
    LOGI("ReadVolumeMetaData enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->ReadVolumeMetaData(devPath, fsUuid, fsType, fsLabel);
}

int32_t StorageDaemonAdapter::Eject(const std::string &devPath)
{
    LOGI("Eject enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Eject(devPath);
}

int32_t StorageDaemonAdapter::GetCDStatus(const std::string &devPath, int32_t &status)
{
    LOGI("GetCDStatus enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->GetCDStatus(devPath, status);
}

int32_t StorageDaemonAdapter::Mount(const std::string &devPath,
                                    const std::string &mountPath,
                                    const std::string &fsType,
                                    const std::string &mountData)
{
    LOGI("Mount enter, devPath=%{public}s, mountPath=%{public}s, fsType=%{public}s, mountData=%{public}s",
         devPath.c_str(), mountPath.c_str(), fsType.c_str(), mountData.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Mount(devPath, mountPath, fsType, mountData);
}

int32_t StorageDaemonAdapter::Unmount(const std::string &mountPath, const std::string &fsType, bool force)
{
    LOGI("Unmount enter, mountPath=%{public}s, force=%{public}d", mountPath.c_str(), static_cast<int32_t>(force));
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Unmount(mountPath, fsType, force);
}

int32_t StorageDaemonAdapter::FormatVolume(const std::string &devPath, const std::string &fsType)
{
    LOGI("FormatVolume enter, devPath=%{public}s, fsType=%{public}s", devPath.c_str(), fsType.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->FormatVolume(devPath, fsType);
}

int32_t StorageDaemonAdapter::Check(const std::string &devPath, const std::string &fsType, bool autoFix)
{
    LOGI("Check enter, devPath=%{public}s, fsType=%{public}s, autoFix=%{public}d",
         devPath.c_str(), fsType.c_str(), static_cast<int32_t>(autoFix));
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Check(devPath, fsType, autoFix);
}

int32_t StorageDaemonAdapter::Repair(const std::string &devPath, const std::string &fsType)
{
    LOGI("Repair enter, devPath=%{public}s, fsType=%{public}s", devPath.c_str(), fsType.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Repair(devPath, fsType);
}

int32_t StorageDaemonAdapter::SetLabel(const std::string &devPath, const std::string &fsType, const std::string &label)
{
    LOGI("SetLabel enter, devPath=%{public}s, fsType=%{public}s, label=%{public}s",
         devPath.c_str(), fsType.c_str(), label.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->SetLabel(devPath, fsType, label);
}

int32_t StorageDaemonAdapter::ReadMetadata(const std::string &devPath,
                                           std::string &uuid,
                                           std::string &type,
                                           std::string &label)
{
    LOGI("ReadMetadata enter, devPath=%{public}s", devPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->ReadMetadata(devPath, uuid, type, label);
}

int32_t StorageDaemonAdapter::GetCapacity(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize)
{
    LOGI("GetCapacity enter, mountPath=%{public}s", mountPath.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->GetCapacity(mountPath, totalSize, freeSize);
}

int32_t StorageDaemonAdapter::GetOddCapacity(const std::string &volumeId, int64_t &totalSize, int64_t &freeSize)
{
    LOGI("GetOddCapacity enter, volumeId=%{public}s", volumeId.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->GetOddCapacity(volumeId, totalSize, freeSize);
}

int32_t StorageDaemonAdapter::OpenFuseDevice(int32_t &fuseFd)
{
    LOGI("OpenFuseDevice enter");
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->OpenFuseDevice(fuseFd);
}

int32_t StorageDaemonAdapter::MountFuseDevice(int32_t fuseFd,
                                              const std::string &mountPath,
                                              const std::string &fsUuid,
                                              const std::string &options)
{
    LOGI("MountFuseDevice enter, fuseFd=%{public}d, mountPath=%{public}s, fsUuid=%{public}s, options=%{public}s",
         fuseFd, mountPath.c_str(), fsUuid.c_str(), options.c_str());
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->MountFuseDevice(fuseFd, mountPath, fsUuid, options);
}

int32_t StorageDaemonAdapter::Partition(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags)
{
    LOGI("Partition enter, diskPath=%{public}s, partitionType=%{public}d, partitionFlags=%{public}u",
         diskPath.c_str(), partitionType, partitionFlags);
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->Partition(diskPath, partitionType, partitionFlags);
}

int32_t StorageDaemonAdapter::RemoveMountPath(const std::string &mountPath)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->RemoveMountPath(mountPath);
}

int32_t StorageDaemonAdapter::EnsureMountPath(const std::string &mountPath)
{
    int32_t err = EnsureProxyReady();
    if (err != E_OK) {
        return err;
    }
    return storageDaemon_->EnsureMountPath(mountPath);
}
} // namespace DiskManager
} // namespace OHOS
