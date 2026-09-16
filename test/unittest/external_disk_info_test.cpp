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

#include "disk.h"
#include "external_disk_info.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class ExternalDiskInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ExternalDiskInfoTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    EXPECT_EQ(info.GetDiskId(), "");
    EXPECT_EQ(info.GetDiskType(), 0);
    EXPECT_TRUE(info.GetVolumeIds().empty());
    EXPECT_EQ(info.GetVendorId(), 0);
    EXPECT_EQ(info.GetProductId(), 0);
}

HWTEST_F(ExternalDiskInfoTest, SettersGetters_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    info.SetDiskId("disk-8-0");
    info.SetDiskType(USB_FLAG);
    info.SetVendorId(1921);
    info.SetProductId(21889);
    EXPECT_EQ(info.GetDiskId(), "disk-8-0");
    EXPECT_EQ(info.GetDiskType(), USB_FLAG);
    EXPECT_EQ(info.GetVendorId(), 1921);
    EXPECT_EQ(info.GetProductId(), 21889);
}

HWTEST_F(ExternalDiskInfoTest, SetVolumeIds_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    std::vector<std::string> volIds = {"vol-8-1", "vol-8-2"};
    info.SetVolumeIds(volIds);
    EXPECT_EQ(info.GetVolumeIds().size(), 2U);
    EXPECT_EQ(info.GetVolumeIds()[0], "vol-8-1");
    EXPECT_EQ(info.GetVolumeIds()[1], "vol-8-2");
}

HWTEST_F(ExternalDiskInfoTest, SetVolumeIds_Move_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    std::vector<std::string> volIds = {"vol-1", "vol-2", "vol-3"};
    info.SetVolumeIds(std::move(volIds));
    EXPECT_EQ(info.GetVolumeIds().size(), 3U);
    EXPECT_EQ(info.GetVolumeIds()[2], "vol-3");
}

HWTEST_F(ExternalDiskInfoTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    info.SetDiskId("disk-8-0");
    info.SetDiskType(USB_FLAG);
    info.SetVolumeIds({"vol-8-1", "vol-8-2"});
    info.SetVendorId(1921);
    info.SetProductId(21889);
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
}

HWTEST_F(ExternalDiskInfoTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    info.SetDiskId("disk-8-0");
    info.SetDiskType(USB_FLAG);
    info.SetVolumeIds({"vol-8-1", "vol-8-2"});
    info.SetVendorId(1921);
    info.SetProductId(21889);
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
    ExternalDiskInfo *result = ExternalDiskInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiskId(), "disk-8-0");
    EXPECT_EQ(result->GetDiskType(), USB_FLAG);
    EXPECT_EQ(result->GetVolumeIds().size(), 2U);
    EXPECT_EQ(result->GetVolumeIds()[0], "vol-8-1");
    EXPECT_EQ(result->GetVolumeIds()[1], "vol-8-2");
    EXPECT_EQ(result->GetVendorId(), 1921);
    EXPECT_EQ(result->GetProductId(), 21889);
    delete result;
}

HWTEST_F(ExternalDiskInfoTest, Marshalling_EmptyVolumeIds_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    info.SetDiskId("disk-1");
    info.SetDiskType(SD_FLAG);
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
    ExternalDiskInfo *result = ExternalDiskInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiskId(), "disk-1");
    EXPECT_EQ(result->GetDiskType(), SD_FLAG);
    EXPECT_TRUE(result->GetVolumeIds().empty());
    delete result;
}

HWTEST_F(ExternalDiskInfoTest, Unmarshalling_EmptyParcel_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    ExternalDiskInfo *result = ExternalDiskInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiskId(), "");
    EXPECT_EQ(result->GetDiskType(), 0);
    EXPECT_TRUE(result->GetVolumeIds().empty());
    EXPECT_EQ(result->GetVendorId(), 0);
    EXPECT_EQ(result->GetProductId(), 0);
    delete result;
}

HWTEST_F(ExternalDiskInfoTest, Marshalling_RoundTrip_ZeroVidPid_TestCase_001, TestSize.Level0)
{
    ExternalDiskInfo info;
    info.SetDiskId("disk-sd");
    info.SetDiskType(SD_FLAG);
    info.SetVendorId(0);
    info.SetProductId(0);
    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));
    ExternalDiskInfo *result = ExternalDiskInfo::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetVendorId(), 0);
    EXPECT_EQ(result->GetProductId(), 0);
    delete result;
}
} // namespace DiskManager
} // namespace OHOS
