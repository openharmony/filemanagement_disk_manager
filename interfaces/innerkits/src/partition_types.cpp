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

#include "partition_types.h"

namespace OHOS {
namespace DiskManager {

// ---------- PartitionInfo ----------

PartitionInfo::PartitionInfo(int32_t partitionNum,
                             const std::string &diskId,
                             int64_t startSector,
                             int64_t endSector,
                             int64_t sizeBytes,
                             const std::string &fsType)
    : partitionNum_(partitionNum),
      diskId_(diskId),
      startSector_(startSector),
      endSector_(endSector),
      sizeBytes_(sizeBytes),
      fsType_(fsType)
{
}

int32_t PartitionInfo::GetPartitionNum() const
{
    return partitionNum_;
}

void PartitionInfo::SetPartitionNum(int32_t partitionNum)
{
    partitionNum_ = partitionNum;
}

std::string PartitionInfo::GetDiskId() const
{
    return diskId_;
}

void PartitionInfo::SetDiskId(const std::string &diskId)
{
    diskId_ = diskId;
}

int64_t PartitionInfo::GetStartSector() const
{
    return startSector_;
}

void PartitionInfo::SetStartSector(int64_t startSector)
{
    startSector_ = startSector;
}

int64_t PartitionInfo::GetEndSector() const
{
    return endSector_;
}

void PartitionInfo::SetEndSector(int64_t endSector)
{
    endSector_ = endSector;
}

int64_t PartitionInfo::GetSizeBytes() const
{
    return sizeBytes_;
}

void PartitionInfo::SetSizeBytes(int64_t sizeBytes)
{
    sizeBytes_ = sizeBytes;
}

std::string PartitionInfo::GetFsType() const
{
    return fsType_;
}

void PartitionInfo::SetFsType(const std::string &fsType)
{
    fsType_ = fsType;
}

bool PartitionInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteInt32(partitionNum_)) {
        return false;
    }
    if (!parcel.WriteString(diskId_)) {
        return false;
    }
    if (!parcel.WriteInt64(startSector_)) {
        return false;
    }
    if (!parcel.WriteInt64(endSector_)) {
        return false;
    }
    if (!parcel.WriteInt64(sizeBytes_)) {
        return false;
    }
    if (!parcel.WriteString(fsType_)) {
        return false;
    }
    return true;
}

PartitionInfo *PartitionInfo::Unmarshalling(Parcel &parcel)
{
    PartitionInfo *obj = new (std::nothrow) PartitionInfo();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->partitionNum_ = parcel.ReadInt32();
    obj->diskId_ = parcel.ReadString();
    obj->startSector_ = parcel.ReadInt64();
    obj->endSector_ = parcel.ReadInt64();
    obj->sizeBytes_ = parcel.ReadInt64();
    obj->fsType_ = parcel.ReadString();
    return obj;
}

// ---------- PartitionTableInfo ----------

std::string PartitionTableInfo::GetDiskId() const
{
    return diskId_;
}

void PartitionTableInfo::SetDiskId(const std::string &diskId)
{
    diskId_ = diskId;
}

std::string PartitionTableInfo::GetTableType() const
{
    return tableType_;
}

void PartitionTableInfo::SetTableType(const std::string &tableType)
{
    tableType_ = tableType;
}

int32_t PartitionTableInfo::GetPartitionCount() const
{
    return partitionCount_;
}

void PartitionTableInfo::SetPartitionCount(int32_t partitionCount)
{
    partitionCount_ = partitionCount;
}

int64_t PartitionTableInfo::GetTotalSector() const
{
    return totalSector_;
}

void PartitionTableInfo::SetTotalSector(int64_t totalSector)
{
    totalSector_ = totalSector;
}

int32_t PartitionTableInfo::GetSectorSize() const
{
    return sectorSize_;
}

void PartitionTableInfo::SetSectorSize(int32_t sectorSize)
{
    sectorSize_ = sectorSize;
}

int32_t PartitionTableInfo::GetAlignSector() const
{
    return alignSector_;
}

void PartitionTableInfo::SetAlignSector(int32_t alignSector)
{
    alignSector_ = alignSector;
}

const std::vector<PartitionInfo> &PartitionTableInfo::GetPartitions() const
{
    return partitions_;
}

void PartitionTableInfo::SetPartitions(const std::vector<PartitionInfo> &partitions)
{
    partitions_ = partitions;
}

void PartitionTableInfo::SetPartitions(std::vector<PartitionInfo> &&partitions)
{
    partitions_ = std::move(partitions);
}

int64_t PartitionTableInfo::GetLastUsableSector() const
{
    return lastUsableSector_;
}

void PartitionTableInfo::SetLastUsableSector(int64_t lastUsableSector)
{
    lastUsableSector_ = lastUsableSector;
}

bool PartitionTableInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(diskId_)) {
        return false;
    }
    if (!parcel.WriteString(tableType_)) {
        return false;
    }
    if (!parcel.WriteInt32(partitionCount_)) {
        return false;
    }
    if (!parcel.WriteInt64(totalSector_)) {
        return false;
    }
    if (!parcel.WriteInt32(sectorSize_)) {
        return false;
    }
    if (!parcel.WriteInt32(alignSector_)) {
        return false;
    }
    const uint32_t nPartitions = static_cast<uint32_t>(partitions_.size());
    if (nPartitions > PARTITION_COUNT_MAX) {
        return false;
    }
    if (!parcel.WriteUint32(nPartitions)) {
        return false;
    }
    for (const auto &part : partitions_) {
        if (!part.Marshalling(parcel)) {
            return false;
        }
    }
    return true;
}

PartitionTableInfo *PartitionTableInfo::Unmarshalling(Parcel &parcel)
{
    PartitionTableInfo *obj = new (std::nothrow) PartitionTableInfo();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->diskId_ = parcel.ReadString();
    obj->tableType_ = parcel.ReadString();
    obj->partitionCount_ = parcel.ReadInt32();
    obj->totalSector_ = parcel.ReadInt64();
    obj->sectorSize_ = parcel.ReadInt32();
    obj->alignSector_ = parcel.ReadInt32();
    const uint32_t nPartitions = parcel.ReadUint32();
    if (nPartitions > PARTITION_COUNT_MAX) {
        delete obj;
        return nullptr;
    }
    obj->partitions_.clear();
    obj->partitions_.reserve(nPartitions);
    for (uint32_t i = 0; i < nPartitions; ++i) {
        PartitionInfo *part = PartitionInfo::Unmarshalling(parcel);
        if (part == nullptr) {
            delete obj;
            return nullptr;
        }
        obj->partitions_.push_back(std::move(*part));
        delete part;
    }
    return obj;
}

} // namespace DiskManager
} // namespace OHOS
