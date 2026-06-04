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

#include <gtest/gtest.h>

#include "partition_types.h"
#include "message_parcel_mock.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class PartitionInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PartitionInfoTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi;
    EXPECT_EQ(pi.GetPartitionNum(), 0);
    EXPECT_EQ(pi.GetDiskId(), "");
    EXPECT_EQ(pi.GetStartSector(), 0);
    EXPECT_EQ(pi.GetEndSector(), 0);
    EXPECT_EQ(pi.GetSizeBytes(), 0);
    EXPECT_EQ(pi.GetFsType(), "");
}

HWTEST_F(PartitionInfoTest, ParameterizedConstructor_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi(1, "disk-1", 2048, 4096, 8192, "ext4");
    EXPECT_EQ(pi.GetPartitionNum(), 1);
    EXPECT_EQ(pi.GetDiskId(), "disk-1");
    EXPECT_EQ(pi.GetStartSector(), 2048);
    EXPECT_EQ(pi.GetEndSector(), 4096);
    EXPECT_EQ(pi.GetSizeBytes(), 8192);
    EXPECT_EQ(pi.GetFsType(), "ext4");
}

HWTEST_F(PartitionInfoTest, SetPartitionNum_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi;
    pi.SetPartitionNum(5);
    EXPECT_EQ(pi.GetPartitionNum(), 5);
}

HWTEST_F(PartitionInfoTest, SetDiskId_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi;
    pi.SetDiskId("disk-new");
    EXPECT_EQ(pi.GetDiskId(), "disk-new");
}

HWTEST_F(PartitionInfoTest, SetStartSector_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi;
    pi.SetStartSector(100);
    EXPECT_EQ(pi.GetStartSector(), 100);
}

HWTEST_F(PartitionInfoTest, SetEndSector_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi;
    pi.SetEndSector(200);
    EXPECT_EQ(pi.GetEndSector(), 200);
}

HWTEST_F(PartitionInfoTest, SetSizeBytes_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi;
    pi.SetSizeBytes(4096);
    EXPECT_EQ(pi.GetSizeBytes(), 4096);
}

HWTEST_F(PartitionInfoTest, SetFsType_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi;
    pi.SetFsType("vfat");
    EXPECT_EQ(pi.GetFsType(), "vfat");
}

HWTEST_F(PartitionInfoTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi(1, "disk-1", 2048, 4096, 8192, "ext4");
    Parcel parcel;
    EXPECT_TRUE(pi.Marshalling(parcel));
}

HWTEST_F(PartitionInfoTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    PartitionInfo pi(2, "disk-2", 100, 200, 300, "vfat");
    Parcel parcel;
    EXPECT_TRUE(pi.Marshalling(parcel));
    PartitionInfo *result = PartitionInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetPartitionNum(), 2);
    EXPECT_EQ(result->GetDiskId(), "disk-2");
    EXPECT_EQ(result->GetStartSector(), 100);
    EXPECT_EQ(result->GetEndSector(), 200);
    EXPECT_EQ(result->GetSizeBytes(), 300);
    EXPECT_EQ(result->GetFsType(), "vfat");
    delete result;
}

class PartitionTableInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PartitionTableInfoTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    EXPECT_EQ(pti.GetDiskId(), "");
    EXPECT_EQ(pti.GetTableType(), "");
    EXPECT_EQ(pti.GetPartitionCount(), 0);
    EXPECT_EQ(pti.GetTotalSector(), 0);
    EXPECT_EQ(pti.GetSectorSize(), 512);
    EXPECT_EQ(pti.GetAlignSector(), 2048);
    EXPECT_EQ(pti.GetPartitions().size(), 0u);
    EXPECT_EQ(pti.GetLastUsableSector(), 0);
}

HWTEST_F(PartitionTableInfoTest, SetDiskId_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetDiskId("disk-1");
    EXPECT_EQ(pti.GetDiskId(), "disk-1");
}

HWTEST_F(PartitionTableInfoTest, SetTableType_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetTableType("gpt");
    EXPECT_EQ(pti.GetTableType(), "gpt");
}

HWTEST_F(PartitionTableInfoTest, SetPartitionCount_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetPartitionCount(4);
    EXPECT_EQ(pti.GetPartitionCount(), 4);
}

HWTEST_F(PartitionTableInfoTest, SetTotalSector_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetTotalSector(1000000);
    EXPECT_EQ(pti.GetTotalSector(), 1000000);
}

HWTEST_F(PartitionTableInfoTest, SetSectorSize_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetSectorSize(4096);
    EXPECT_EQ(pti.GetSectorSize(), 4096);
}

HWTEST_F(PartitionTableInfoTest, SetAlignSector_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetAlignSector(8192);
    EXPECT_EQ(pti.GetAlignSector(), 8192);
}

HWTEST_F(PartitionTableInfoTest, SetPartitions_Copy_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    std::vector<PartitionInfo> parts = {
        PartitionInfo(1, "d", 0, 100, 200, "ext4"),
        PartitionInfo(2, "d", 100, 200, 300, "vfat"),
    };
    pti.SetPartitions(parts);
    EXPECT_EQ(pti.GetPartitions().size(), 2u);
    EXPECT_EQ(pti.GetPartitions()[0].GetPartitionNum(), 1);
}

HWTEST_F(PartitionTableInfoTest, SetPartitions_Move_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    std::vector<PartitionInfo> parts = {
        PartitionInfo(1, "d", 0, 100, 200, "ext4"),
    };
    pti.SetPartitions(std::move(parts));
    EXPECT_EQ(pti.GetPartitions().size(), 1u);
}

HWTEST_F(PartitionTableInfoTest, SetLastUsableSector_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetLastUsableSector(500000);
    EXPECT_EQ(pti.GetLastUsableSector(), 500000);
}

HWTEST_F(PartitionTableInfoTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetDiskId("disk-1");
    pti.SetTableType("gpt");
    pti.SetPartitionCount(2);
    pti.SetTotalSector(100000);
    pti.SetSectorSize(512);
    pti.SetAlignSector(2048);
    pti.SetPartitions({
        PartitionInfo(1, "disk-1", 2048, 50000, 96000, "ext4"),
    });
    Parcel parcel;
    EXPECT_TRUE(pti.Marshalling(parcel));
}

HWTEST_F(PartitionTableInfoTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetDiskId("disk-1");
    pti.SetTableType("gpt");
    pti.SetPartitionCount(1);
    pti.SetTotalSector(100000);
    pti.SetSectorSize(512);
    pti.SetAlignSector(2048);
    pti.SetPartitions({
        PartitionInfo(1, "disk-1", 2048, 50000, 96000, "ext4"),
    });
    Parcel parcel;
    EXPECT_TRUE(pti.Marshalling(parcel));
    PartitionTableInfo *result = PartitionTableInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiskId(), "disk-1");
    EXPECT_EQ(result->GetTableType(), "gpt");
    EXPECT_EQ(result->GetPartitionCount(), 1);
    EXPECT_EQ(result->GetTotalSector(), 100000);
    EXPECT_EQ(result->GetSectorSize(), 512);
    EXPECT_EQ(result->GetAlignSector(), 2048);
    EXPECT_EQ(result->GetPartitions().size(), 1u);
    EXPECT_EQ(result->GetPartitions()[0].GetPartitionNum(), 1);
    delete result;
}

HWTEST_F(PartitionTableInfoTest, Marshalling_OverMaxPartition_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo pti;
    pti.SetDiskId("d");
    pti.SetTableType("gpt");
    pti.SetPartitionCount(0);
    pti.SetTotalSector(0);
    pti.SetSectorSize(512);
    pti.SetAlignSector(2048);
    std::vector<PartitionInfo> parts(257, PartitionInfo(1, "d", 0, 0, 0, "ext4"));
    pti.SetPartitions(parts);
    Parcel parcel;
    EXPECT_FALSE(pti.Marshalling(parcel));
}

HWTEST_F(PartitionTableInfoTest, Unmarshalling_OverMaxPartition_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("d");
    parcel.WriteString("gpt");
    parcel.WriteInt32(0);
    parcel.WriteInt64(0);
    parcel.WriteInt32(512);
    parcel.WriteInt32(2048);
    parcel.WriteUint32(257u);
    PartitionTableInfo *result = PartitionTableInfo::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

class PartitionParamsTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PartitionParamsTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    PartitionParams pp;
    EXPECT_EQ(pp.GetPartitionNum(), 0);
    EXPECT_EQ(pp.GetStartSector(), 0);
    EXPECT_EQ(pp.GetEndSector(), 0);
    EXPECT_EQ(pp.GetTypeCode(), "");
}

HWTEST_F(PartitionParamsTest, ParameterizedConstructor_TestCase_001, TestSize.Level0)
{
    PartitionParams pp(1, 2048, 4096, "0FC");
    EXPECT_EQ(pp.GetPartitionNum(), 1);
    EXPECT_EQ(pp.GetStartSector(), 2048);
    EXPECT_EQ(pp.GetEndSector(), 4096);
    EXPECT_EQ(pp.GetTypeCode(), "0FC");
}

HWTEST_F(PartitionParamsTest, SetPartitionNum_TestCase_001, TestSize.Level0)
{
    PartitionParams pp;
    pp.SetPartitionNum(3);
    EXPECT_EQ(pp.GetPartitionNum(), 3);
}

HWTEST_F(PartitionParamsTest, SetStartSector_TestCase_001, TestSize.Level0)
{
    PartitionParams pp;
    pp.SetStartSector(100);
    EXPECT_EQ(pp.GetStartSector(), 100);
}

HWTEST_F(PartitionParamsTest, SetEndSector_TestCase_001, TestSize.Level0)
{
    PartitionParams pp;
    pp.SetEndSector(200);
    EXPECT_EQ(pp.GetEndSector(), 200);
}

HWTEST_F(PartitionParamsTest, SetTypeCode_TestCase_001, TestSize.Level0)
{
    PartitionParams pp;
    pp.SetTypeCode("EBD");
    EXPECT_EQ(pp.GetTypeCode(), "EBD");
}

HWTEST_F(PartitionParamsTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    PartitionParams pp(1, 2048, 4096, "0FC");
    Parcel parcel;
    EXPECT_TRUE(pp.Marshalling(parcel));
}

HWTEST_F(PartitionParamsTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    PartitionParams pp(1, 2048, 4096, "0FC");
    Parcel parcel;
    EXPECT_TRUE(pp.Marshalling(parcel));
    PartitionParams *result = PartitionParams::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetPartitionNum(), 1);
    EXPECT_EQ(result->GetStartSector(), 2048);
    EXPECT_EQ(result->GetEndSector(), 4096);
    EXPECT_EQ(result->GetTypeCode(), "0FC");
    delete result;
}

class FormatParamsTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(FormatParamsTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    FormatParams fp;
    EXPECT_EQ(fp.GetFsType(), "");
    EXPECT_TRUE(fp.GetQuickFormat());
    EXPECT_EQ(fp.GetVolumeName(), "");
}

HWTEST_F(FormatParamsTest, ParameterizedConstructor_TestCase_001, TestSize.Level0)
{
    FormatParams fp("ext4", false, "myvol");
    EXPECT_EQ(fp.GetFsType(), "ext4");
    EXPECT_FALSE(fp.GetQuickFormat());
    EXPECT_EQ(fp.GetVolumeName(), "myvol");
}

HWTEST_F(FormatParamsTest, SetFsType_TestCase_001, TestSize.Level0)
{
    FormatParams fp;
    fp.SetFsType("vfat");
    EXPECT_EQ(fp.GetFsType(), "vfat");
}

HWTEST_F(FormatParamsTest, SetQuickFormat_TestCase_001, TestSize.Level0)
{
    FormatParams fp;
    fp.SetQuickFormat(false);
    EXPECT_FALSE(fp.GetQuickFormat());
}

HWTEST_F(FormatParamsTest, SetVolumeName_TestCase_001, TestSize.Level0)
{
    FormatParams fp;
    fp.SetVolumeName("test-vol");
    EXPECT_EQ(fp.GetVolumeName(), "test-vol");
}

HWTEST_F(FormatParamsTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    FormatParams fp("ext4", true, "myvol");
    Parcel parcel;
    EXPECT_TRUE(fp.Marshalling(parcel));
}

HWTEST_F(FormatParamsTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    FormatParams fp("vfat", false, "usb-vol");
    Parcel parcel;
    EXPECT_TRUE(fp.Marshalling(parcel));
    FormatParams *result = FormatParams::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetFsType(), "vfat");
    EXPECT_FALSE(result->GetQuickFormat());
    EXPECT_EQ(result->GetVolumeName(), "usb-vol");
    delete result;
}

} // namespace DiskManager
} // namespace OHOS