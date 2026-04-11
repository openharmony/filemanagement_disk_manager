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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_STORAGE_DAEMON_ADAPTER_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_STORAGE_DAEMON_ADAPTER_H

#include <mutex>
#include <string>

#include "iremote_object.h"
#include "istorage_daemon.h"
#include "nocopyable.h"
#include "refbase.h"

namespace OHOS {
namespace DiskManager {

class SdDeathRecipient;

/**
 * 连接 storage_daemon 并转发卷管理相关 IStorageDaemon 调用。
 * 使用方式：StorageDaemonAdapter::GetInstance().Mount(...);
 */
class StorageDaemonAdapter : public NoCopyable {
    friend class SdDeathRecipient;

public:
    static StorageDaemonAdapter &GetInstance();

    int32_t Connect();

    int32_t Mount(std::string volumeId, int32_t flag);
    int32_t Unmount(std::string volumeId);
    int32_t TryToFix(std::string volumeId, int32_t flag);
    int32_t Check(std::string volumeId);
    int32_t Partition(std::string diskId, int32_t type);
    int32_t Format(std::string volumeId, std::string type);
    int32_t SetVolumeDescription(std::string volumeId, std::string description);
    int32_t QueryUsbIsInUse(const std::string &diskPath, bool &isInUse);
    int32_t GetOddCapacity(const std::string &volumeId, int64_t &totalSize, int64_t &freeSize);
    int32_t MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd);

private:
    StorageDaemonAdapter();
    ~StorageDaemonAdapter();

    int32_t EnsureProxyReady();
    int32_t ResetSdProxy();

    sptr<StorageDaemon::IStorageDaemon> storageDaemon_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ = nullptr;
    std::mutex mutex_;
};

class SdDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    SdDeathRecipient() = default;
    ~SdDeathRecipient() override = default;
    void OnRemoteDied(const wptr<IRemoteObject> &object) override;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_STORAGE_DAEMON_ADAPTER_H
