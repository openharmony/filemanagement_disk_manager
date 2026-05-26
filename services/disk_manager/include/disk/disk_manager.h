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

#include "storage_spec_models.h"

#include "disk.h"
#include "partition_types.h"
#include "volume_external.h"

#include <cstdint>
#include <map>
#include <shared_mutex>
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
    bool HasDisk(const std::string &diskId);

    int32_t OnDiskDestroyed(const std::string &diskId);

    int32_t OnVolumeCreated(const VolumeExternal &volExternal);
    int32_t OnVolumeDestroyed(const std::string &volumeId);

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

    int32_t EraseVolume(const std::string &volumeId);
    int32_t EjectVolume(const std::string &volumeId);
    int32_t CreateIsoImage(const std::string &volumeId, const std::string &filePath);
    int32_t BurnVolume(const std::string &volumeId, const std::string &burnOptions);
    int32_t GetVolumeOpProcess(const std::string &volumeId, int32_t &progressPct);
    int32_t VerifyBurnData(const std::string &volumeId, int32_t verifyType);

    void NotifyMtpMounted(const std::string &id,
                          const std::string &path,
                          const std::string &desc,
                          const std::string &uuid,
                          const std::string &fsType);
    void NotifyMtpUnmounted(const std::string &id, bool isBadRemove);
    int32_t GetPartitionTable(const std::string &diskId, PartitionTableInfo &out);
    int32_t CreatePartition(const std::string &diskId, const PartitionParams &params);
    int32_t DeletePartition(const std::string &diskId, int32_t partitionNum);
    int32_t FormatPartition(const std::string &diskId, int32_t partitionNum, const FormatParams &params);

private:
    DiskManager();
    ~DiskManager();
    DiskManager(const DiskManager &) = delete;
    DiskManager &operator=(const DiskManager &) = delete;

    struct VolumeMountPolicy {
        bool useVoldataPath = false;
        bool useFuseData = false;
    };

    struct MountDataPathParams {
        const VolumeExternal &volExternal;
        const std::string &fsUuid;
        const VolumeMountPolicy &policy;
        bool *voldataMappingCreated = nullptr;
    };

    bool IsSafeFsUuid(const std::string &fsUuid);
    /** 调用方已持 volumeMapMutex_（读锁）。 */
    int32_t LookupVolumeByUuidUnlocked(const std::string &fsUuid, VolumeExternal &out) const;
    std::string GetVolumePath(const std::string &volumeUuid);
    bool IsOddFsType(const std::string &fsType);
    int32_t GetOddCapacityAtMountPath(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize);
    bool IsPathMounted(std::string path);
    int32_t EnsureFsUuidReady(VolumeExternal &volExternal, std::string &outFsUuid);
    int32_t MountUsbFuseIfNeeded(const std::string &volumeId,
                                 const std::string &fsType,
                                 const std::string &fsUuid,
                                 bool useFuseData);
    /** 调用方已按顺序持 diskMapMutex_、volumeMapMutex_ 读锁。 */
    bool ShouldUseVoldataMountPathForDiskUnlocked(const std::string &diskId,
                                                  const std::string &fsNormLower) const;
    VolumeMountPolicy ComputeVolumeMountPolicy(const std::string &diskId, const std::string &fsType) const;

    int32_t MountVolumeFilesystem(VolumeExternal &volExternal, const std::string &fsType, const std::string &fsUuid);

    std::string BuildMountDataPath(const MountDataPathParams &params);

    int32_t UnmountVolumeMountPoints(const VolumeExternal &volExternal, bool force);
    int32_t GetFlagFromMajorInfo(const std::string &volumeId);

    /** 不持 map 锁；挂载完成后由 Mount 写回 volumeMap_。 */
    int32_t MountVolumeEntry(VolumeExternal &volExternal, const std::string &volumeId);
    bool SetTotalSector(std::vector<std::string> &content, PartitionTableInfo &info, const std::string &devPath);
    bool SetSectorSize(std::vector<std::string> &content, PartitionTableInfo &info);
    bool SetAlignSector(std::vector<std::string> &content, PartitionTableInfo &info);
    bool SetUsableSector(std::vector<std::string> &content, PartitionTableInfo &info);
    void SetPartitions(std::vector<std::string> &content, PartitionTableInfo &info);
    void SetTableType(std::vector<std::string> &content, PartitionTableInfo &info);
    bool ParsePartitionInfo(const std::string &context, PartitionInfo &info);
    std::string GetFsTypeByDiskIdAndPartNum(const std::string &diskId, int32_t partitionNum);
    void UpsertPartitionTable(PartitionTableInfo &info);
    int32_t GetPartInfo(const std::string &diskId, PartitionTableInfo &info);
    bool IsParamsValid(const PartitionParams &params, const PartitionTableInfo &info);
    bool IsDiskNotReady(const std::string &diskId);
    bool IsVolumeMounted(const std::string &diskId, int32_t partitionNum);

    /**
     * diskMapMutex_ 与 volumeMapMutex_ 相互独立。
     * 若同一流程需两把锁，必须按此顺序一次性加锁：先 disk，后 volume。
     */
    mutable std::shared_mutex diskMapMutex_;
    mutable std::shared_mutex volumeMapMutex_;
    mutable std::shared_mutex partitionTableMapMutex_;
    std::mutex partitionLock_;
    std::map<std::string, Disk> diskMap_;
    std::map<std::string, VolumeExternal> volumeMap_;
    std::map<std::string, PartitionTableInfo> partitionTableMap_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_DISK_MANAGER_H
