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

#include "partitiontypes_fuzzer.h"

#include "partition_types.h"
#include "parcel.h"

#include <memory>

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr int32_t SECTOR_MULTIPLIER = 2;
constexpr int32_t SIZE_MULTIPLIER = 100;
constexpr int32_t TOTAL_SECTOR_MULTIPLIER = 1000;
constexpr int32_t LAST_USABLE_SECTOR_OFFSET = 999;
const std::string DEFAULT_FS_TYPE = "ext4";
} // namespace

void FuzzPartitionInfo(const int32_t flag, Parcel &parcel)
{
    PartitionInfo partInfo(flag, "disk-1-1", flag, flag * SECTOR_MULTIPLIER, flag * SIZE_MULTIPLIER, "ext4");
    partInfo.SetPartitionNum(flag);
    partInfo.SetDiskId("disk-1-1");
    partInfo.SetStartSector(static_cast<int64_t>(flag));
    partInfo.SetEndSector(static_cast<int64_t>(flag) * SECTOR_MULTIPLIER);
    partInfo.SetSizeBytes(static_cast<int64_t>(flag) * SIZE_MULTIPLIER);
    partInfo.SetFsType("ntfs");
    partInfo.GetPartitionNum();
    partInfo.GetDiskId();
    partInfo.GetStartSector();
    partInfo.GetEndSector();
    partInfo.GetSizeBytes();
    partInfo.GetFsType();
    partInfo.Marshalling(parcel);
    auto unmarshallingPartInfo = std::unique_ptr<PartitionInfo>(PartitionInfo::Unmarshalling(parcel));
}

void FuzzPartitionTableInfo(const int32_t flag, Parcel &parcel)
{
    PartitionTableInfo tableInfo;
    tableInfo.SetDiskId("disk-1-1");
    tableInfo.SetTableType("gpt");
    tableInfo.SetPartitionCount(flag);
    tableInfo.SetTotalSector(static_cast<int64_t>(flag) * TOTAL_SECTOR_MULTIPLIER);
    tableInfo.SetSectorSize(flag);
    tableInfo.SetAlignSector(flag);
    tableInfo.SetLastUsableSector(static_cast<int64_t>(flag) * LAST_USABLE_SECTOR_OFFSET);
    std::vector<PartitionInfo> parts;
    parts.emplace_back(flag, "disk-1-1", flag, flag * SECTOR_MULTIPLIER, flag * SIZE_MULTIPLIER, "ext4");
    tableInfo.SetPartitions(parts);
    std::vector<PartitionInfo> movedParts;
    movedParts.emplace_back(flag, "disk-1-2", flag, flag * SECTOR_MULTIPLIER, flag * SIZE_MULTIPLIER, "ntfs");
    tableInfo.SetPartitions(std::move(movedParts));
    tableInfo.GetDiskId();
    tableInfo.GetTableType();
    tableInfo.GetPartitionCount();
    tableInfo.GetTotalSector();
    tableInfo.GetSectorSize();
    tableInfo.GetAlignSector();
    tableInfo.GetLastUsableSector();
    tableInfo.GetPartitions();
    tableInfo.Marshalling(parcel);
    auto unmarshallingTableInfo = std::unique_ptr<PartitionTableInfo>(PartitionTableInfo::Unmarshalling(parcel));
}

void FuzzPartitionParams(const int32_t flag, Parcel &parcel)
{
    PartitionParams params(flag, flag, flag * SECTOR_MULTIPLIER, DEFAULT_FS_TYPE);
    params.SetPartitionNum(flag);
    params.SetStartSector(static_cast<int64_t>(flag));
    params.SetEndSector(static_cast<int64_t>(flag) * SECTOR_MULTIPLIER);
    params.SetTypeCode(DEFAULT_FS_TYPE);
    params.GetPartitionNum();
    params.GetStartSector();
    params.GetEndSector();
    params.GetTypeCode();
    params.Marshalling(parcel);
    auto unmarshallingParams = std::unique_ptr<PartitionParams>(PartitionParams::Unmarshalling(parcel));
}

void FuzzFormatParams(const int32_t flag, Parcel &parcel)
{
    FormatParams fmtParams("ext4", true, "testVolume");
    fmtParams.SetFsType("ntfs");
    fmtParams.SetQuickFormat(flag % SECTOR_MULTIPLIER == 0);
    fmtParams.SetVolumeName("fuzzVolume");
    fmtParams.GetFsType();
    fmtParams.GetQuickFormat();
    fmtParams.GetVolumeName();
    fmtParams.Marshalling(parcel);
    auto unmarshallingFmtParams = std::unique_ptr<FormatParams>(FormatParams::Unmarshalling(parcel));
}

bool PartitionTypesFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return false;
    }

    int32_t flag = *(reinterpret_cast<const int32_t *>(data));

    Parcel parcel;
    FuzzPartitionInfo(flag, parcel);

    Parcel parcel2;
    FuzzPartitionTableInfo(flag, parcel2);

    Parcel parcel3;
    FuzzPartitionParams(flag, parcel3);

    Parcel parcel4;
    FuzzFormatParams(flag, parcel4);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::PartitionTypesFuzzTest(data, size);
    return 0;
}
