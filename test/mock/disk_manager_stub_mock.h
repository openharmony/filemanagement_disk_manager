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

#ifndef OHOS_DISK_MANAGER_DISK_MANAGER_STUB_MOCK_H
#define OHOS_DISK_MANAGER_DISK_MANAGER_STUB_MOCK_H

#include <gmock/gmock.h>

#include "disk_manager_stub.h"

namespace OHOS {
namespace DiskManager {

class DiskManagerStubMock : public DiskManagerStub {
public:
    MOCK_METHOD(int32_t, Mount, (const std::string &volumeId));
    MOCK_METHOD(int32_t, Unmount, (const std::string &volumeId));
    MOCK_METHOD(int32_t, Format, (const std::string &volumeId, const std::string &fsType));
    MOCK_METHOD(int32_t, TryToFix, (const std::string &volumeId));
    MOCK_METHOD(int32_t, SetVolumeDescription, (const std::string &fsUuid, const std::string &description));
    MOCK_METHOD(int32_t, GetAllVolumes, (std::vector<VolumeExternal> &vecOfVol));
    MOCK_METHOD(int32_t, GetVolumeByUuid, (const std::string &fsUuid, VolumeExternal &vc));
    MOCK_METHOD(int32_t, GetVolumeById, (const std::string &volumeId, VolumeExternal &vc));
    MOCK_METHOD(int32_t, GetFreeSizeOfVolume, (const std::string &volumeUuid, int64_t &freeSize));
    MOCK_METHOD(int32_t, GetTotalSizeOfVolume, (const std::string &volumeUuid, int64_t &totalSize));
    MOCK_METHOD(int32_t, Partition, (const std::string &diskId, int32_t type));
    MOCK_METHOD(int32_t, GetAllDisks, (std::vector<Disk> &vecOfDisk));
    MOCK_METHOD(int32_t, GetDiskById, (const std::string &diskId, Disk &disk));
    MOCK_METHOD(int32_t, QueryUsbIsInUse, (const std::string &diskPath, bool &isInUse));
    MOCK_METHOD(int32_t, OnBlockDiskUevent, (const std::string &rawUeventMsg));
    MOCK_METHOD(int32_t, NotifyMtpMounted,
        (const std::string &id, const std::string &path, const std::string &desc, const std::string &uuid,
         const std::string &fsType));
    MOCK_METHOD(int32_t, NotifyMtpUnmounted, (const std::string &id, bool isBadRemove));
    MOCK_METHOD(int32_t, Erase, (const std::string &volumeId));
    MOCK_METHOD(int32_t, Eject, (const std::string &diskId));
    MOCK_METHOD(int32_t, CreateIsoImage, (const std::string &volumeId, const std::string &filePath));
    MOCK_METHOD(int32_t, Burn, (const std::string &volumeId, const std::string &burnOptions));
    MOCK_METHOD(int32_t, GetVolumeOpProcess, (const std::string &volumeId, int32_t &progressPct));
    MOCK_METHOD(int32_t, GetPartitionTable, (const std::string &diskId, PartitionTableInfo &tableInfo));
    MOCK_METHOD(int32_t, CreatePartition, (const std::string &diskId, const PartitionParams &params));
    MOCK_METHOD(int32_t, DeletePartition, (const std::string &diskId, int32_t partitionNum));
    MOCK_METHOD(int32_t, FormatPartition,
        (const std::string &diskId, int32_t partitionNum, const FormatParams &params));
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_MANAGER_STUB_MOCK_H
