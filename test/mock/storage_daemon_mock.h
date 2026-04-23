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

#ifndef OHOS_DISK_MANAGER_STORAGE_DAEMON_MOCK_H
#define OHOS_DISK_MANAGER_STORAGE_DAEMON_MOCK_H

#include <gmock/gmock.h>
#include <string>

#include "adapter/istorage_daemon.h"
#include "errors.h"
#include "iremote_stub.h"

namespace OHOS {
namespace DiskManager {

class StorageDaemonMock final : public IRemoteStub<StorageDaemon::IStorageDaemon> {
public:
    MOCK_METHOD4(SendRequest, int(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option));
    MOCK_METHOD2(QueryUsbIsInUse, ErrCode(const std::string &diskPath, bool &isInUse));
    MOCK_METHOD3(MountUsbFuse, ErrCode(const std::string &volumeId, std::string &fsUuid, int &fuseFd));
    MOCK_METHOD4(CreateBlockDeviceNode,
                 ErrCode(const std::string &devPath, uint32_t mode, int32_t major, int32_t minor));
    MOCK_METHOD1(DestroyBlockDeviceNode, ErrCode(const std::string &devPath));
    MOCK_METHOD3(ReadPartitionTable, ErrCode(const std::string &devPath, std::string &output, int32_t &maxVolume));
    MOCK_METHOD4(ReadVolumeMetaData,
                 ErrCode(const std::string &devPath, std::string &fsUuid, std::string &fsType, std::string &fsLabel));
    MOCK_METHOD1(Eject, ErrCode(const std::string &devPath));
    MOCK_METHOD2(GetCDStatus, ErrCode(const std::string &devPath, int32_t &status));
    MOCK_METHOD4(Mount,
                 ErrCode(const std::string &devPath,
                         const std::string &mountPath,
                         const std::string &fsType,
                         const std::string &mountData));
    MOCK_METHOD3(Unmount, ErrCode(const std::string &mountPath, const std::string &fsType, bool force));
    MOCK_METHOD2(FormatVolume, ErrCode(const std::string &devPath, const std::string &fsType));
    MOCK_METHOD3(Check, ErrCode(const std::string &devPath, const std::string &fsType, bool autoFix));
    MOCK_METHOD2(Repair, ErrCode(const std::string &devPath, const std::string &fsType));
    MOCK_METHOD3(SetLabel, ErrCode(const std::string &devPath, const std::string &fsType, const std::string &label));
    MOCK_METHOD4(ReadMetadata,
                 ErrCode(const std::string &devPath, std::string &uuid, std::string &type, std::string &label));
    MOCK_METHOD3(GetCapacity, ErrCode(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize));
    MOCK_METHOD3(GetOddCapacity, ErrCode(const std::string &volumeId, int64_t &totalSize, int64_t &freeSize));
    MOCK_METHOD1(OpenFuseDevice, ErrCode(int32_t &fuseFd));
    MOCK_METHOD4(
        MountFuseDevice,
        ErrCode(int32_t fuseFd, const std::string &mountPath, const std::string &fsUuid, const std::string &options));
    MOCK_METHOD3(Partition, ErrCode(const std::string &diskPath, int32_t partitionType, uint32_t partitionFlags));
    MOCK_METHOD1(EnsureMountPath, ErrCode(const std::string &mountPath));
    MOCK_METHOD1(RemoveMountPath, ErrCode(const std::string &mountPath));
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_STORAGE_DAEMON_MOCK_H
