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

bool PartitionTypesFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return false;
    }

    Parcel parcel;
    int32_t flag = *(reinterpret_cast<const int32_t *>(data));

    PartitionInfo partInfo(flag, "disk-1-1", flag, flag * 2, flag * 100, "ext4");
    partInfo.SetPartitionNum(flag);
    partInfo.SetDiskId("disk-1-1");
    partInfo.SetStartSector(static_cast<int64_t>(flag));
    partInfo.SetEndSector(static_cast<int64_t>(flag) * 2);
    partInfo.SetSizeBytes(static_cast<int64_t>(flag) * 100);
    partInfo.SetFsType("ntfs");
    partInfo.GetPartitionNum();
    partInfo.GetDiskId();
    partInfo.GetStartSector();
    partInfo.GetEndSector();
    partInfo.GetSizeBytes();
    partInfo.GetFsType();
    partInfo.Marshalling(parcel);
    auto unmarshallingPartInfo = std::unique_ptr<PartitionInfo>(PartitionInfo::Unmarshalling(parcel));

    Parcel parcel2;
    PartitionTableInfo tableInfo;
    tableInfo.SetDiskId("disk-1-1");
    tableInfo.SetTableType("gpt");
    tableInfo.SetPartitionCount(flag);
    tableInfo.SetTotalSector(static_cast<int64_t>(flag) * 1000);
    tableInfo.SetSectorSize(flag);
    tableInfo.SetAlignSector(flag);
    tableInfo.SetLastUsableSector(static_cast<int64_t>(flag) * 999);
    tableInfo.GetDiskId();
    tableInfo.GetTableType();
    tableInfo.GetPartitionCount();
    tableInfo.GetTotalSector();
    tableInfo.GetSectorSize();
    tableInfo.GetAlignSector();
    tableInfo.GetLastUsableSector();
    tableInfo.GetPartitions();
    tableInfo.Marshalling(parcel2);
    auto unmarshallingTableInfo = std::unique_ptr<PartitionTableInfo>(PartitionTableInfo::Unmarshalling(parcel2));

    Parcel parcel3;

    PartitionParams params(flag, flag, flag * 2, "0FC63DAF-8483-4772-8E79-3D69D8477DE4");
    params.SetPartitionNum(flag);
    params.SetStartSector(static_cast<int64_t>(flag));
    params.SetEndSector(static_cast<int64_t>(flag) * 2);
    params.SetTypeCode("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7");
    params.GetPartitionNum();
    params.GetStartSector();
    params.GetEndSector();
    params.GetTypeCode();
    params.Marshalling(parcel3);
    auto unmarshallingParams = std::unique_ptr<PartitionParams>(PartitionParams::Unmarshalling(parcel3));

    Parcel parcel4;

    FormatParams fmtParams("ext4", true, "testVolume");
    fmtParams.SetFsType("ntfs");
    fmtParams.SetQuickFormat(flag % 2 == 0);
    fmtParams.SetVolumeName("fuzzVolume");
    fmtParams.GetFsType();
    fmtParams.GetQuickFormat();
    fmtParams.GetVolumeName();
    fmtParams.Marshalling(parcel4);
    auto unmarshallingFmtParams = std::unique_ptr<FormatParams>(FormatParams::Unmarshalling(parcel4));

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::PartitionTypesFuzzTest(data, size);
    return 0;
}
