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

#include "external_volume_info.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class ExternalVolumeInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ExternalVolumeInfoTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    ExternalVolumeInfo info;
    EXPECT_EQ(info.GetVolumeId(), "");
    EXPECT_EQ(info.GetUuid(), "");
    EXPECT_EQ(info.GetDiskId(), "");
    EXPECT_EQ(info.GetDescription(), "");
    EXPECT_EQ(info.GetState(), 0);
    EXPECT_EQ(info.GetTotalSize(), 0);
    EXPECT_EQ(info.GetFreeSize(), 0);
    EXPECT_EQ(info.GetPath(), "");
    EXPECT_EQ(info.GetFsType(), "");
}

HWTEST_F(ExternalVolumeInfoTest, SettersGetters_TestCase_001, TestSize.Level0)
{
    ExternalVolumeInfo info;
    info.SetVolumeId("vol-8-1");
    info.SetUuid("7FC7-17EC");
    info.SetDiskId("disk-8-0");
    info.SetDescription("USB Drive");
    info.SetState(2);
    info.SetTotalSize(2147483648LL);
    info.SetFreeSize(1073741824LL);
    info.SetPath("/mnt/external/vol-8-1");
    info.SetFsType("vfat");
    EXPECT_EQ(info.GetVolumeId(), "vol-8-1");
    EXPECT_EQ(info.GetUuid(), "7FC7-17EC");
    EXPECT_EQ(info.GetDiskId(), "disk-8-0");
    EXPECT_EQ(info.GetDescription(), "USB Drive");
    EXPECT_EQ(info.GetState(), 2);
    EXPECT_EQ(info.GetTotalSize(), 2147483648LL);
    EXPECT_EQ(info.GetFreeSize(), 1073741824LL);
    EXPECT_EQ(info.GetPath(), "/mnt/external/vol-8-1");
    EXPECT_EQ(info.GetFsType(), "vfat");
}

HWTEST_F(ExternalVolumeInfoTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    ExternalVolumeInfo info;
    info.SetVolumeId("vol-8-1");
    info.SetUuid("7FC7-17EC");
    info.SetDiskId("disk-8-0");
    info.SetDescription("USB Drive");
    info.SetState(2);
    info.SetTotalSize(2147483648LL);
    info.SetFreeSize(1073741824LL);
    info.SetPath("/mnt/external/vol-8-1");
    info.SetFsType("vfat");
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
}

HWTEST_F(ExternalVolumeInfoTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    ExternalVolumeInfo info;
    info.SetVolumeId("vol-8-1");
    info.SetUuid("7FC7-17EC");
    info.SetDiskId("disk-8-0");
    info.SetDescription("USB Drive");
    info.SetState(2);
    info.SetTotalSize(2147483648LL);
    info.SetFreeSize(1073741824LL);
    info.SetPath("/mnt/external/vol-8-1");
    info.SetFsType("vfat");
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
    ExternalVolumeInfo *result = ExternalVolumeInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetVolumeId(), "vol-8-1");
    EXPECT_EQ(result->GetUuid(), "7FC7-17EC");
    EXPECT_EQ(result->GetDiskId(), "disk-8-0");
    EXPECT_EQ(result->GetDescription(), "USB Drive");
    EXPECT_EQ(result->GetState(), 2);
    EXPECT_EQ(result->GetTotalSize(), 2147483648LL);
    EXPECT_EQ(result->GetFreeSize(), 1073741824LL);
    EXPECT_EQ(result->GetPath(), "/mnt/external/vol-8-1");
    EXPECT_EQ(result->GetFsType(), "vfat");
    delete result;
}

HWTEST_F(ExternalVolumeInfoTest, Marshalling_EmptyFields_TestCase_001, TestSize.Level0)
{
    ExternalVolumeInfo info;
    info.SetVolumeId("vol-1");
    info.SetDiskId("disk-1");
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
    ExternalVolumeInfo *result = ExternalVolumeInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetVolumeId(), "vol-1");
    EXPECT_EQ(result->GetUuid(), "");
    EXPECT_EQ(result->GetDiskId(), "disk-1");
    EXPECT_EQ(result->GetDescription(), "");
    EXPECT_EQ(result->GetState(), 0);
    EXPECT_EQ(result->GetTotalSize(), 0);
    EXPECT_EQ(result->GetFreeSize(), 0);
    EXPECT_EQ(result->GetPath(), "");
    EXPECT_EQ(result->GetFsType(), "");
    delete result;
}

HWTEST_F(ExternalVolumeInfoTest, Unmarshalling_EmptyParcel_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    ExternalVolumeInfo *result = ExternalVolumeInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetVolumeId(), "");
    EXPECT_EQ(result->GetUuid(), "");
    EXPECT_EQ(result->GetDiskId(), "");
    EXPECT_EQ(result->GetDescription(), "");
    EXPECT_EQ(result->GetState(), 0);
    EXPECT_EQ(result->GetTotalSize(), 0);
    EXPECT_EQ(result->GetFreeSize(), 0);
    EXPECT_EQ(result->GetPath(), "");
    EXPECT_EQ(result->GetFsType(), "");
    delete result;
}

HWTEST_F(ExternalVolumeInfoTest, Marshalling_RoundTrip_NegativeSize_TestCase_001, TestSize.Level0)
{
    ExternalVolumeInfo info;
    info.SetVolumeId("vol-sd");
    info.SetState(0);
    info.SetTotalSize(-1);
    info.SetFreeSize(-1);
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
    ExternalVolumeInfo *result = ExternalVolumeInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetVolumeId(), "vol-sd");
    EXPECT_EQ(result->GetTotalSize(), -1);
    EXPECT_EQ(result->GetFreeSize(), -1);
    delete result;
}
} // namespace DiskManager
} // namespace OHOS
