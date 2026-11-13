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
#include <parcel.h>
#include <memory>
#include <string>

#include "local_disk_info.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class LocalDiskInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: DefaultConstructor_TestCase_001
 * @tc.desc: Test default constructor initializes fields correctly.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    LocalDiskInfo info;
    EXPECT_EQ(info.GetSysPath(), "");
    EXPECT_EQ(info.GetModel(), "");
    EXPECT_EQ(info.GetDescription(), "");
    EXPECT_EQ(info.GetSize(), 0);
    EXPECT_EQ(info.GetMediaType(), 0);
    EXPECT_EQ(info.GetState(), "");
}

/**
 * @tc.name: SettersGetters_TestCase_001
 * @tc.desc: Test all setters and getters round-trip.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, SettersGetters_TestCase_001, TestSize.Level0)
{
    LocalDiskInfo info;
    info.SetSysPath("/sys/block/nvme0n1");
    info.SetModel("Samsung SSD 980");
    info.SetDescription("Samsung SSD 980");
    info.SetSize(500107862016);
    info.SetMediaType(1);
    info.SetState("live");
    EXPECT_EQ(info.GetSysPath(), "/sys/block/nvme0n1");
    EXPECT_EQ(info.GetModel(), "Samsung SSD 980");
    EXPECT_EQ(info.GetDescription(), "Samsung SSD 980");
    EXPECT_EQ(info.GetSize(), 500107862016);
    EXPECT_EQ(info.GetMediaType(), 1);
    EXPECT_EQ(info.GetState(), "live");
}

/**
 * @tc.name: Marshalling_TestCase_001
 * @tc.desc: Test Marshalling and Unmarshalling round-trip with all fields.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, Marshalling_TestCase_001, TestSize.Level0)
{
    LocalDiskInfo info;
    info.SetSysPath("/sys/block/sda");
    info.SetModel("ST1000DM003");
    info.SetDescription("ST1000DM003");
    info.SetSize(1000204886016);
    info.SetMediaType(2);
    info.SetState("live");

    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));

    auto result = std::unique_ptr<LocalDiskInfo>(LocalDiskInfo::Unmarshalling(parcel));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetSysPath(), "/sys/block/sda");
    EXPECT_EQ(result->GetModel(), "ST1000DM003");
    EXPECT_EQ(result->GetDescription(), "ST1000DM003");
    EXPECT_EQ(result->GetSize(), 1000204886016);
    EXPECT_EQ(result->GetMediaType(), 2);
    EXPECT_EQ(result->GetState(), "live");
}

/**
 * @tc.name: Unmarshalling_StringTooLong_001
 * @tc.desc: Test Unmarshalling with sysPath exceeding PARCEL_STRING_MAX_LEN.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, Unmarshalling_StringTooLong_001, TestSize.Level0)
{
    Parcel parcel;
    std::string longStr(5000, 'A');
    parcel.WriteString(longStr);
    parcel.WriteString("model");
    parcel.WriteString("desc");
    parcel.WriteInt64(100);
    parcel.WriteInt32(1);
    parcel.WriteString("state");
    auto result = LocalDiskInfo::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: Unmarshalling_StringTooLong_002
 * @tc.desc: Test Unmarshalling with model exceeding PARCEL_STRING_MAX_LEN.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, Unmarshalling_StringTooLong_002, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("sysPath");
    std::string longStr(5000, 'B');
    parcel.WriteString(longStr);
    parcel.WriteString("desc");
    parcel.WriteInt64(100);
    parcel.WriteInt32(1);
    parcel.WriteString("state");
    auto result = LocalDiskInfo::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: Unmarshalling_StringTooLong_003
 * @tc.desc: Test Unmarshalling with description exceeding PARCEL_STRING_MAX_LEN.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, Unmarshalling_StringTooLong_003, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("sysPath");
    parcel.WriteString("model");
    std::string longStr(5000, 'C');
    parcel.WriteString(longStr);
    parcel.WriteInt64(100);
    parcel.WriteInt32(1);
    parcel.WriteString("state");
    auto result = LocalDiskInfo::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: Unmarshalling_StringTooLong_004
 * @tc.desc: Test Unmarshalling with state exceeding PARCEL_STRING_MAX_LEN.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, Unmarshalling_StringTooLong_004, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("sysPath");
    parcel.WriteString("model");
    parcel.WriteString("desc");
    parcel.WriteInt64(100);
    parcel.WriteInt32(1);
    std::string longStr(5000, 'D');
    parcel.WriteString(longStr);
    auto result = LocalDiskInfo::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: EmptyFields_TestCase_001
 * @tc.desc: Test Marshalling/Unmarshalling with empty string fields.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(LocalDiskInfoTest, EmptyFields_TestCase_001, TestSize.Level0)
{
    LocalDiskInfo info;
    info.SetSysPath("");
    info.SetModel("");
    info.SetDescription("");
    info.SetSize(0);
    info.SetMediaType(0);
    info.SetState("");

    Parcel parcel;
    EXPECT_TRUE(info.Marshalling(parcel));

    auto result = std::unique_ptr<LocalDiskInfo>(LocalDiskInfo::Unmarshalling(parcel));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetSysPath(), "");
    EXPECT_EQ(result->GetModel(), "");
    EXPECT_EQ(result->GetDescription(), "");
    EXPECT_EQ(result->GetSize(), 0);
    EXPECT_EQ(result->GetMediaType(), 0);
    EXPECT_EQ(result->GetState(), "");
}

} // namespace DiskManager
} // namespace OHOS
