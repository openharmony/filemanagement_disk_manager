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

#ifndef OHOS_DISK_MANAGER_DISK_DISK_MANAGER_MOCK_H
#define OHOS_DISK_MANAGER_DISK_DISK_MANAGER_MOCK_H

#include "disk.h"
#include "volume_external.h"
#include "partition_types.h"
#include "disk/storage_spec_models.h"

#include <cstdint>
#include <string>
#include <vector>

#include <gmock/gmock.h>

namespace OHOS {
namespace DiskManager {

class DiskManager {
public:
    static DiskManager &GetInstance();

    MOCK_METHOD(int32_t, Mount, (const std::string &volumeId));
    MOCK_METHOD(int32_t, Unmount, (const std::string &volumeId));
    MOCK_METHOD(int32_t, ForceUnmount, (const std::string &volumeId));
    MOCK_METHOD(int32_t, Format, (const std::string &volumeId, const std::string &fsType));
    MOCK_METHOD(int32_t, TryToFix, (const std::string &volumeId));
    MOCK_METHOD(int32_t, SetVolumeDescription, (const std::string &fsUuid, const std::string &description));
    MOCK_METHOD(int32_t, Partition, (const std::string &diskId, int32_t type));
    MOCK_METHOD(int32_t, PurgeVolumesForDisk, (const std::string &diskId));
    MOCK_METHOD(bool, IsPartitioning, (const std::string &diskId));
    MOCK_METHOD(int32_t, OnDiskCreated, (const Disk &disk));
    MOCK_METHOD(bool, HasDisk, (const std::string &diskId));
    MOCK_METHOD(int32_t, OnDiskDestroyed, (const std::string &diskId));
    MOCK_METHOD(int32_t, OnVolumeCreated, (const VolumeExternal &volExternal));
    MOCK_METHOD(int32_t, OnVolumeDestroyed, (const std::string &volumeId));
    MOCK_METHOD(int32_t, ReplacePartitionsForDisk,
        (const std::string &diskId, const std::vector<PartitionRecord> &partitions));
    MOCK_METHOD(int32_t, GetAllDisks, (std::vector<Disk> &out));
    MOCK_METHOD(int32_t, GetDiskById, (const std::string &diskId, Disk &out));
    MOCK_METHOD(int32_t, UpdateDisk, (const Disk &disk));
    MOCK_METHOD(int32_t, GetAllVolumes, (std::vector<VolumeExternal> &out));
    MOCK_METHOD(int32_t, GetVolumeById, (const std::string &volumeId, VolumeExternal &out));
    MOCK_METHOD(int32_t, UpdateVolumeMetadata,
        (const std::string &volumeId, const std::string &fsUuid,
         const std::string &fsTypeStr, const std::string &description));
    MOCK_METHOD(int32_t, Erase, (const std::string &volumeId));
    MOCK_METHOD(int32_t, Eject, (const std::string &diskId));
    MOCK_METHOD(void, NotifyPartitionDone, (const std::string &diskId));
    MOCK_METHOD(std::string, GetDiscType, (const std::string &extraInfo));
    MOCK_METHOD(std::string, GetDriverType, (const std::string &extraInfo));
    MOCK_METHOD(bool, DestroyVolumeByDiskIdAndPartNum, (const std::string &diskId, int32_t partNum));

    static DiskManager *mockInstance_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_DISK_MANAGER_MOCK_H