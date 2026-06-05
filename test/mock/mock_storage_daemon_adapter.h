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

#ifndef OHOS_DISK_MANAGER_MOCK_STORAGE_DAEMON_ADAPTER_H
#define OHOS_DISK_MANAGER_MOCK_STORAGE_DAEMON_ADAPTER_H

#include <cstdint>
#include <string>

#include <gmock/gmock.h>

#ifndef ERR_INVALID_DATA
constexpr int32_t ERR_INVALID_DATA = 22;
#endif

namespace OHOS {
namespace DiskManager {

class MockStorageDaemonAdapter {
public:
    static MockStorageDaemonAdapter &GetInstance();

    MOCK_METHOD(int32_t, Connect, ());
    MOCK_METHOD(int32_t, QueryUsbIsInUse, (const std::string &diskPath, bool &isInUse));
    MOCK_METHOD(int32_t, CreateBlockDeviceNode,
        (const std::string &devPath, uint32_t mode, int32_t major, int32_t minor));
    MOCK_METHOD(int32_t, DestroyBlockDeviceNode, (const std::string &devPath));
    MOCK_METHOD(int32_t, ReadPartitionTable,
        (const std::string &devPath, std::string &output, int32_t &maxVolume));
    MOCK_METHOD(int32_t, Eject, (const std::string &volId));
    MOCK_METHOD(int32_t, QueryCDStatus, (const std::string &devPath, int32_t &status));
    MOCK_METHOD(int32_t, Mount,
        (const std::string &devPath, const std::string &mountPath,
         const std::string &fsType, uint64_t mountFlag, const std::string &mountData));
    MOCK_METHOD(int32_t, Unmount, (const std::string &mountPath, const std::string &fsType, bool force));
    MOCK_METHOD(int32_t, FormatVolume, (const std::string &devPath, const std::string &fsType));
    MOCK_METHOD(int32_t, Check, (const std::string &devPath, const std::string &fsType, bool autoFix));
    MOCK_METHOD(int32_t, Repair, (const std::string &devPath, const std::string &fsType));
    MOCK_METHOD(int32_t, SetLabel,
        (const std::string &devPath, const std::string &fsType, const std::string &label));
    MOCK_METHOD(int32_t, ReadMetadata,
        (const std::string &devPath, std::string &uuid, std::string &type, std::string &label));
    MOCK_METHOD(int32_t, GetCapacity,
        (const std::string &mountPath, int64_t &totalSize, int64_t &freeSize));
    MOCK_METHOD(int32_t, MountFuseDevice, (const std::string &mountPath, int32_t &fuseFd));
    MOCK_METHOD(int32_t, Partition, (const std::string &diskPath, const std::string &partitionType));
    MOCK_METHOD(int32_t, GetBlockInfoByType,
        (const std::string &type, std::string &blockInfos, const std::string &diskId));
    MOCK_METHOD(int32_t, GetPartitionTableInfo,
        (const std::string &devPath, std::string &execRet));
    MOCK_METHOD(int32_t, CreatePartition,
        (const std::string &devPath, int32_t partitionNum,
         int64_t startSector, int64_t endSector, const std::string &typeCode));
    MOCK_METHOD(int32_t, DeletePartition, (const std::string &devPath, int32_t partitionNum));
    MOCK_METHOD(int32_t, FormatPartition,
        (const std::string &devPath, const std::string &fsType,
         const std::string &volumeName, bool quickFormat));
    MOCK_METHOD(int32_t, EnsureProxyReady, ());
    MOCK_METHOD(int32_t, ResetSdProxy, ());

    static MockStorageDaemonAdapter mockInstance_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_STORAGE_DAEMON_ADAPTER_H