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
 * 连接 storage_daemon 并透传 IStorageDaemon 调用。
 */
class StorageDaemonAdapter : public NoCopyable {
    friend class SdDeathRecipient;

public:
    static StorageDaemonAdapter &GetInstance();

    int32_t Connect();
    int32_t QueryUsbIsInUse(const std::string &diskPath, bool &isInUse);
    int32_t CreateBlockDeviceNode(const std::string &devPath, uint32_t mode, int32_t major, int32_t minor);
    int32_t DestroyBlockDeviceNode(const std::string &devPath);
    int32_t ReadPartitionTable(const std::string &devPath, std::string &output, int32_t &maxVolume);
    int32_t Eject(const std::string &devPath);
    int32_t GetCDStatus(const std::string &devPath, int32_t &status);
    int32_t Mount(const std::string &devPath,
                  const std::string &mountPath,
                  const std::string &fsType,
                  const std::string &mountData);
    int32_t Unmount(const std::string &mountPath, const std::string &fsType, bool force);
    int32_t FormatVolume(const std::string &devPath, const std::string &fsType);
    int32_t Check(const std::string &devPath, const std::string &fsType, bool autoFix);
    int32_t Repair(const std::string &devPath, const std::string &fsType);
    int32_t SetLabel(const std::string &devPath, const std::string &fsType, const std::string &label);
    int32_t ReadMetadata(const std::string &devPath, std::string &uuid, std::string &type, std::string &label);
    int32_t GetCapacity(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize);
    int32_t MountFuseDevice(const std::string &mountPath, int32_t &fuseFd);
    int32_t Partition(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags);

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
