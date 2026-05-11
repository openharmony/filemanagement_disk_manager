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
    /** uevent 引导等写入 define.md 对齐的磁盘快照（与 Notify 路径的 OnDiskCreated 可并存合并）。 */
    int32_t UpsertDiskSnapshot(const DiskStoreRecord &rec);
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

private:
    DiskManager();
    ~DiskManager();
    DiskManager(const DiskManager &) = delete;
    DiskManager &operator=(const DiskManager &) = delete;

    bool IsSafeFsUuid(const std::string &fsUuid);
    /** 已由调用方持 mapsRwMutex_（读：shared_lock / 写：unique_lock）；仅查 volumeMap_。 */
    int32_t LookupVolumeByUuidUnlocked(const std::string &fsUuid, VolumeExternal &out);
    std::string GetVolumePath(const std::string &volumeUuid);
    /** 在完成 statvfs 后按 fsType 判断是否走光盘容量语义。 */
    bool IsOddFsType(const std::string &fsType);
    int32_t GetOddCapacityAtMountPath(const std::string &mountPath, int64_t &totalSize, int64_t &freeSize);
    bool IsPathMounted(std::string path);
    int32_t EnsureFsUuidReady(VolumeExternal &volExternal, std::string &outFsUuid);
    int32_t MountUsbFuseIfNeeded(const std::string &volumeId,
                                 VolumeExternal &volExternal,
                                 const std::string &fsType,
                                 bool useEnterpriseFuseStack);
    /**
     * 在已持有 mapsRwMutex_（读或写）的前提下按 diskId 查 diskMap_；内部不再加锁。
     * （Mount/Unmount/Format/TryToFix 等已持写锁，同线程不可再套 shared_lock，否则会死锁。）
     */
    bool EffectiveUsbStackForVolumeDiskUnlocked(const std::string &diskId, const std::string &fsTypeRaw) const;
    bool ShouldUseVoldataMountPathForDiskUnlocked(const std::string &diskId, const std::string &fsNormLower) const;

    /** 在已持 mapsRwMutex_ 写锁且完成 FUSE 前置后，执行块设备挂载并更新卷状态。 */
    int32_t MountVolumeFilesystemLocked(VolumeExternal &volExternal, const std::string &fsType, const std::string &fsUuid);
    /** 调用前须已持有 mapsRwMutex_ 写锁。 */
    int32_t UnmountVolumeMountPoints(const VolumeExternal &volExternal, bool force);
    int32_t GetFlagFromMajorInfo(const std::string &volumeId);

    /** 执行挂载主体；调用前须已持有 mapsRwMutex_ 写锁且 volumeId 与 volExternal 一致。 */
    int32_t MountVolumeEntryUnlocked(VolumeExternal &volExternal, const std::string &volumeId);

    uint32_t AllocateVoldataMountIndexLocked();

    /**
     * 保护 diskMap_、volumeMap_（后续可扩展 partitionMap_）。
     * 读路径：shared_lock；写路径：unique_lock。
     */
    mutable std::shared_mutex mapsRwMutex_;
    std::map<std::string, VolumeExternal> volumeMap_;
    std::map<std::string, Disk> diskMap_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_DISK_MANAGER_H
