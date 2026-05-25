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

#ifndef OHOS_DISK_MANAGER_PARTITION_TYPES_H
#define OHOS_DISK_MANAGER_PARTITION_TYPES_H

#include "parcel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {

/**
 * Partition information.
 * Corresponds to volumeManager.PartitionInfo in new_api.
 */
class PartitionInfo : public Parcelable {
public:
    PartitionInfo() = default;
    PartitionInfo(int32_t partitionNum,
                  const std::string &diskId,
                  int64_t startSector,
                  int64_t endSector,
                  int64_t sizeBytes,
                  const std::string &fsType);

    int32_t GetPartitionNum() const;
    void SetPartitionNum(int32_t partitionNum);
    std::string GetDiskId() const;
    void SetDiskId(const std::string &diskId);
    int64_t GetStartSector() const;
    void SetStartSector(int64_t startSector);
    int64_t GetEndSector() const;
    void SetEndSector(int64_t endSector);
    int64_t GetSizeBytes() const;
    void SetSizeBytes(int64_t sizeBytes);
    std::string GetFsType() const;
    void SetFsType(const std::string &fsType);

    bool Marshalling(Parcel &parcel) const override;
    static PartitionInfo *Unmarshalling(Parcel &parcel);

private:
    int32_t partitionNum_ {0};      // partitionNum
    std::string diskId_;           // diskId
    int64_t startSector_ {0};      // startSector
    int64_t endSector_ {0};        // endSector
    int64_t sizeBytes_ {0};        // sizeBytes
    std::string fsType_;           // fsType
};

/**
 * Partition table information.
 * Corresponds to volumeManager.PartitionTableInfo in new_api.
 */
class PartitionTableInfo : public Parcelable {
public:
    PartitionTableInfo() = default;

    std::string GetDiskId() const;
    void SetDiskId(const std::string &diskId);
    std::string GetTableType() const;
    void SetTableType(const std::string &tableType);
    int32_t GetPartitionCount() const;
    void SetPartitionCount(int32_t partitionCount);
    int64_t GetTotalSector() const;
    void SetTotalSector(int64_t totalSector);
    int32_t GetSectorSize() const;
    void SetSectorSize(int32_t sectorSize);
    int32_t GetAlignSector() const;
    void SetAlignSector(int32_t alignSector);
    const std::vector<PartitionInfo> &GetPartitions() const;
    void SetPartitions(const std::vector<PartitionInfo> &partitions);
    void SetPartitions(std::vector<PartitionInfo> &&partitions);
    int64_t GetLastUsableSector() const;
    void SetLastUsableSector(int64_t lastUsableSector);

    bool Marshalling(Parcel &parcel) const override;
    static PartitionTableInfo *Unmarshalling(Parcel &parcel);

private:
    static constexpr uint32_t PARTITION_COUNT_MAX = 256U;

    std::string diskId_;                      // diskId
    std::string tableType_;                   // tableType (gpt/mbr)
    int32_t partitionCount_ {0};              // partitionCount
    int64_t totalSector_ {0};                 // totalSector
    int32_t sectorSize_ {512};                // sectorSize
    int32_t alignSector_ {2048};              // alignSector
    int32_t lastUsableSector_ {0};             // lastUsableSector
    std::vector<PartitionInfo> partitions_;   // partitions array
};

/**
 * Partition creation parameters.
 * Corresponds to volumeManager.PartitionParams in new_api.
 */
struct PartitionParams {
    int32_t partitionNum {0};       // partitionNum
    int64_t startSector {0};      // startSector
    int64_t endSector {0};        // endSector
    std::string typeCode;         // typeCode (filesystem type)
};

/**
 * Format parameters for partition formatting.
 * Corresponds to volumeManager.FormatParams in new_api.
 */
struct FormatParams {
    std::string fsType;           // fsType (ext4/vfat/exfat)
    bool quickFormat {true};      // quickFormat, default true
    std::string volumeName;       // volumeName after formatting
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_PARTITION_TYPES_H
