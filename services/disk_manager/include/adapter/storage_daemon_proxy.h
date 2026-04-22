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

#ifndef OHOS_STORAGEDAEMON_STORAGE_DAEMON_PROXY_H
#define OHOS_STORAGEDAEMON_STORAGE_DAEMON_PROXY_H

#include "iremote_proxy.h"
#include "istorage_daemon.h"

namespace OHOS {
namespace StorageDaemon {

class StorageDaemonProxy : public IRemoteProxy<IStorageDaemon> {
public:
    explicit StorageDaemonProxy(const sptr<IRemoteObject> &impl);
    ~StorageDaemonProxy() = default;

    ErrCode QueryUsbIsInUse(const std::string &diskPath, bool &isInUse) override;
    ErrCode MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd) override;

    ErrCode CreateBlockDeviceNode(const std::string &devPath,
                                  uint32_t mode,
                                  int32_t major,
                                  int32_t minor) override;
    ErrCode DestroyBlockDeviceNode(const std::string &devPath) override;
    ErrCode ReadPartitionTable(const std::string &devPath, std::string &output, int32_t &maxVolume) override;
    ErrCode ReadVolumeMetaData(const std::string &devPath,
                               std::string &fsUuid,
                               std::string &fsType,
                               std::string &fsLabel) override;
    ErrCode Eject(const std::string &devPath) override;
    ErrCode GetCDStatus(const std::string &devPath, int32_t &status) override;
    ErrCode Mount(const std::string &devPath,
                  const std::string &mountPath,
                  const std::string &fsType,
                  const std::string &mountData) override;
    ErrCode Unmount(const std::string &mountPath, const std::string &fsType, bool force) override;
    ErrCode FormatVolume(const std::string &devPath, const std::string &fsType) override;
    ErrCode Check(const std::string &devPath, const std::string &fsType, bool autoFix) override;
    ErrCode Repair(const std::string &devPath, const std::string &fsType) override;
    ErrCode SetLabel(const std::string &devPath, const std::string &fsType, const std::string &label) override;
    ErrCode ReadMetadata(const std::string &devPath,
                         std::string &uuid,
                         std::string &type,
                         std::string &label) override;
    ErrCode GetCapacity(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize) override;
    ErrCode OpenFuseDevice(int32_t &fuseFd) override;
    ErrCode MountFuseDevice(int32_t fuseFd,
                            const std::string &mountPath,
                            const std::string &fsUuid,
                            const std::string &options) override;
    ErrCode Partition(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags) override;
    ErrCode EnsureMountPath(const std::string &mountPath) override;
    ErrCode RemoveMountPath(const std::string &mountPath) override;

private:
    static inline BrokerDelegator<StorageDaemonProxy> delegator_;
};

} // namespace StorageDaemon
} // namespace OHOS

#endif // OHOS_STORAGEDAEMON_STORAGE_DAEMON_PROXY_H
