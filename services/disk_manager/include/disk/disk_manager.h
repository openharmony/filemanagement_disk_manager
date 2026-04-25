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

#ifndef OHOS_DISK_MANAGER_DISK_DISK_MANAGER_H
#define OHOS_DISK_MANAGER_DISK_DISK_MANAGER_H

#include "disk/storage_spec_models.h"

#include "disk.h"
#include "volume_core.h"
#include "volume_external.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {

class DiskManager {
public:
    static DiskManager &GetInstance();

    int32_t Mount(const std::string &volumeId);
    int32_t Unmount(const std::string &volumeId);
    int32_t Format(const std::string &volumeId, const std::string &fsType);
    int32_t TryToFix(const std::string &volumeId);
    int32_t SetVolumeDescription(const std::string &fsUuid, const std::string &description);
    int32_t Partition(const std::string &diskId, int32_t type);

    int32_t OnDiskCreated(const Disk &disk);
    /** uevent 引导等写入 define.md 对齐的磁盘快照（与 Notify 路径的 OnDiskCreated 可并存合并）。 */
    int32_t UpsertDiskSnapshot(const DiskStoreRecord &rec);
    bool HasDisk(const std::string &diskId);

    int32_t OnDiskDestroyed(const std::string &diskId);

    int32_t OnVolumeCreated(const VolumeExternal &volExternal);
    int32_t OnVolumeMounted(const std::string &volumeId, const std::string &mountPath, int32_t state);
    int32_t OnVolumeStateChanged(const std::string &volumeId, uint32_t state);
    int32_t OnVolumeDestroyed(const std::string &volumeId);
    int32_t OnVolumeDamaged(const VolumeInfoStr &vis);

    int32_t ReplacePartitionsForDisk(const std::string &diskId, const std::vector<PartitionRecord> &partitions);

    int32_t GetAllDisks(std::vector<Disk> &out);
    int32_t GetDiskById(const std::string &diskId, Disk &out);
    int32_t GetAllVolumes(std::vector<VolumeExternal> &out);
    int32_t GetVolumeById(const std::string &volumeId, VolumeExternal &out);
    int32_t GetVolumeByUuid(const std::string &fsUuid, VolumeExternal &out);

    int32_t UpdateVolumeMetadata(const std::string &volumeId,
                                 const std::string &fsUuid,
                                 const std::string &fsTypeStr,
                                 const std::string &description);

    int32_t GetFreeSizeOfVolume(const std::string &volumeUuid, int64_t &freeSize);
    int32_t GetTotalSizeOfVolume(const std::string &volumeUuid, int64_t &totalSize);

private:
    DiskManager();
    ~DiskManager();
    DiskManager(const DiskManager &) = delete;
    DiskManager &operator=(const DiskManager &) = delete;
    std::map<std::string, VolumeExternal> volumeMap_;
    std::mutex volumeMapMutex_;
    std::map<std::string, Disk> diskMap_;
    std::mutex diskMapMutex_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_DISK_MANAGER_H
