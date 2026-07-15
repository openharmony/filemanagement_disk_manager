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

#include "disk_manager_utils.h"

using namespace testing::ext;
using namespace OHOS::DiskManager;

namespace {
constexpr size_t UUID_MAX_LEN = 128;
class DiskManagerUtilsTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};
}

HWTEST_F(DiskManagerUtilsTest, GetAnonyString_ShortLength_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAnonyString_ShortLength_001 Start";
    EXPECT_EQ(GetAnonyString(""), "******");
    EXPECT_EQ(GetAnonyString("a"), "******");
    EXPECT_EQ(GetAnonyString("ab"), "******");
    GTEST_LOG_(INFO) << "GetAnonyString_ShortLength_001 End";
}

HWTEST_F(DiskManagerUtilsTest, GetAnonyString_MidLength_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAnonyString_MidLength_001 Start";
    EXPECT_EQ(GetAnonyString("abc"), "a******c");
    EXPECT_EQ(GetAnonyString("abcdefghij"), "a******j");
    EXPECT_EQ(GetAnonyString("abcdefghijklmnopqrst"), "a******t");
    GTEST_LOG_(INFO) << "GetAnonyString_MidLength_001 End";
}

HWTEST_F(DiskManagerUtilsTest, GetAnonyString_LongLength_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAnonyString_LongLength_001 Start";
    std::string longStr = "abcdefghijklmnopqrstuvwxy";
    EXPECT_EQ(GetAnonyString(longStr), "abcd******vwxy");
    std::string longerStr = "abcdefghijklmnopqrstuvwxyABCDEFGHIJKLMNOPQRSTUVWXY";
    EXPECT_EQ(GetAnonyString(longerStr), "abcd******VWXY");
    GTEST_LOG_(INFO) << "GetAnonyString_LongLength_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_NoTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_NoTraversal_001 Start";
    EXPECT_FALSE(IsFilePathInvalid("/mnt/data/usb"));
    EXPECT_FALSE(IsFilePathInvalid("/dev/block/sda1"));
    EXPECT_FALSE(IsFilePathInvalid("vol-8-1"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_NoTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_Empty_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_Empty_001 Start";
    EXPECT_TRUE(IsFilePathInvalid(""));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_Empty_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_LeadingTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_LeadingTraversal_001 Start";
    EXPECT_TRUE(IsFilePathInvalid("../etc/passwd"));
    EXPECT_TRUE(IsFilePathInvalid("../../secret"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_LeadingTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_InteriorTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_InteriorTraversal_001 Start";
    EXPECT_TRUE(IsFilePathInvalid("/mnt/data/../etc/passwd"));
    EXPECT_TRUE(IsFilePathInvalid("/home/user/../root"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_InteriorTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_EmbeddedNonTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_EmbeddedNonTraversal_001 Start";
    EXPECT_FALSE(IsFilePathInvalid("abc../def"));
    EXPECT_FALSE(IsFilePathInvalid("some..file"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_EmbeddedNonTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_TailTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_TailTraversal_001 Start";
    EXPECT_TRUE(IsFilePathInvalid("/mnt/data/.."));
    EXPECT_TRUE(IsFilePathInvalid("/home/user/.."));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_TailTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_TailNotTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_TailNotTraversal_001 Start";
    EXPECT_FALSE(IsFilePathInvalid("/mnt/data/..."));
    EXPECT_FALSE(IsFilePathInvalid("/mnt/data/a/..b"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_TailNotTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_BackslashTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_BackslashTraversal_001 Start";
    EXPECT_TRUE(IsFilePathInvalid("..\\etc\\passwd"));
    EXPECT_TRUE(IsFilePathInvalid("/mnt/data\\..\\etc"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_BackslashTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_UrlEncodedTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_UrlEncodedTraversal_001 Start";
    EXPECT_TRUE(IsFilePathInvalid("..%2fetc%2fpasswd"));
    EXPECT_TRUE(IsFilePathInvalid("..%2Fetc%2Fpasswd"));
    EXPECT_TRUE(IsFilePathInvalid("/mnt/data%2f.."));
    EXPECT_TRUE(IsFilePathInvalid("..%5cetc%5cpasswd"));
    EXPECT_TRUE(IsFilePathInvalid("..%5Cetc%5Cpasswd"));
    EXPECT_TRUE(IsFilePathInvalid("%2e%2e%2fetc"));
    EXPECT_TRUE(IsFilePathInvalid("%2E%2E%2Fetc"));
    EXPECT_TRUE(IsFilePathInvalid("%2e%2e%5cetc"));
    EXPECT_TRUE(IsFilePathInvalid("%2E%2E%5Cetc"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_UrlEncodedTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_DoubleUrlEncodedTraversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_DoubleUrlEncodedTraversal_001 Start";
    EXPECT_TRUE(IsFilePathInvalid("%252e%252e%252fetc"));
    EXPECT_TRUE(IsFilePathInvalid("%252E%252E%252Fetc"));
    EXPECT_TRUE(IsFilePathInvalid("%252e%252e%255cetc"));
    EXPECT_TRUE(IsFilePathInvalid("%252E%252E%255Cetc"));
    EXPECT_TRUE(IsFilePathInvalid("..%252fetc"));
    EXPECT_TRUE(IsFilePathInvalid("%252f.."));
    EXPECT_TRUE(IsFilePathInvalid("..%255cetc"));
    EXPECT_TRUE(IsFilePathInvalid("%255c.."));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_DoubleUrlEncodedTraversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsFilePathInvalid_NullByte_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsFilePathInvalid_NullByte_001 Start";
    EXPECT_TRUE(IsFilePathInvalid("/mnt/data%00"));
    EXPECT_TRUE(IsFilePathInvalid("/etc/passwd%00.jpg"));
    EXPECT_TRUE(IsFilePathInvalid("%00"));
    GTEST_LOG_(INFO) << "IsFilePathInvalid_NullByte_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsPureDigitsInRange_Valid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_Valid_001 Start";
    EXPECT_TRUE(IsPureDigitsInRange("1", 1, 4));
    EXPECT_TRUE(IsPureDigitsInRange("12", 1, 4));
    EXPECT_TRUE(IsPureDigitsInRange("1234", 1, 4));
    EXPECT_TRUE(IsPureDigitsInRange("99", 2, 2));
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_Valid_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsPureDigitsInRange_TooShort_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_TooShort_001 Start";
    EXPECT_FALSE(IsPureDigitsInRange("", 1, 4));
    EXPECT_FALSE(IsPureDigitsInRange("1", 2, 4));
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_TooShort_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsPureDigitsInRange_TooLong_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_TooLong_001 Start";
    EXPECT_FALSE(IsPureDigitsInRange("12345", 1, 4));
    EXPECT_FALSE(IsPureDigitsInRange("1234567890", 1, 4));
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_TooLong_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsPureDigitsInRange_NonDigit_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_NonDigit_001 Start";
    EXPECT_FALSE(IsPureDigitsInRange("ab", 1, 4));
    EXPECT_FALSE(IsPureDigitsInRange("1a", 1, 4));
    EXPECT_FALSE(IsPureDigitsInRange("-1", 1, 4));
    EXPECT_FALSE(IsPureDigitsInRange("12!", 1, 4));
    GTEST_LOG_(INFO) << "IsPureDigitsInRange_NonDigit_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsMountPathValid_Valid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsMountPathValid_Valid_001 Start";
    EXPECT_TRUE(IsMountPathValid("/mnt/data"));
    EXPECT_TRUE(IsMountPathValid("/mnt/data/usb"));
    EXPECT_TRUE(IsMountPathValid("/mnt/data/usb/1"));
    GTEST_LOG_(INFO) << "IsMountPathValid_Valid_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsMountPathValid_Invalid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsMountPathValid_Invalid_001 Start";
    EXPECT_FALSE(IsMountPathValid("/data"));
    EXPECT_FALSE(IsMountPathValid("/mnt"));
    EXPECT_FALSE(IsMountPathValid("/mnt/media"));
    EXPECT_FALSE(IsMountPathValid(""));
    EXPECT_FALSE(IsMountPathValid("mnt/data"));
    GTEST_LOG_(INFO) << "IsMountPathValid_Invalid_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_Valid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_Valid_001 Start";
    EXPECT_TRUE(IsVolumeIdValid("vol-8-1"));
    EXPECT_TRUE(IsVolumeIdValid("vol-123-456"));
    EXPECT_TRUE(IsVolumeIdValid("vol-1-9999"));
    EXPECT_TRUE(IsVolumeIdValid("vol-1-1"));
    EXPECT_TRUE(IsVolumeIdValid("vol-9999-9999"));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_Valid_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_Empty_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_Empty_001 Start";
    EXPECT_FALSE(IsVolumeIdValid(""));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_Empty_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_Traversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_Traversal_001 Start";
    EXPECT_FALSE(IsVolumeIdValid("vol-../-1"));
    EXPECT_FALSE(IsVolumeIdValid("vol-/.."));
    EXPECT_FALSE(IsVolumeIdValid("../vol-1-1"));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_Traversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_WrongPrefix_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_WrongPrefix_001 Start";
    EXPECT_FALSE(IsVolumeIdValid("disk-8-1"));
    EXPECT_FALSE(IsVolumeIdValid("volume-8-1"));
    EXPECT_FALSE(IsVolumeIdValid("vol"));
    EXPECT_FALSE(IsVolumeIdValid("vol-"));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_WrongPrefix_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_MissingSecondDash_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_MissingSecondDash_001 Start";
    EXPECT_FALSE(IsVolumeIdValid("vol-8"));
    EXPECT_FALSE(IsVolumeIdValid("vol-1234"));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_MissingSecondDash_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_ExtraDash_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_ExtraDash_001 Start";
    EXPECT_FALSE(IsVolumeIdValid("vol-8-1-2"));
    EXPECT_FALSE(IsVolumeIdValid("vol-1-2-3"));
    EXPECT_FALSE(IsVolumeIdValid("vol-8-1-"));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_ExtraDash_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_SegmentNotDigits_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_SegmentNotDigits_001 Start";
    EXPECT_FALSE(IsVolumeIdValid("vol-ab-1"));
    EXPECT_FALSE(IsVolumeIdValid("vol-8-ab"));
    EXPECT_FALSE(IsVolumeIdValid("vol-1a-2b"));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_SegmentNotDigits_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsVolumeIdValid_SegmentOutOfRange_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsVolumeIdValid_SegmentOutOfRange_001 Start";
    EXPECT_FALSE(IsVolumeIdValid("vol-12345-1"));
    EXPECT_FALSE(IsVolumeIdValid("vol-1-12345"));
    GTEST_LOG_(INFO) << "IsVolumeIdValid_SegmentOutOfRange_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_Valid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_Valid_001 Start";
    EXPECT_TRUE(IsDiskIdValid("disk-8-1"));
    EXPECT_TRUE(IsDiskIdValid("disk-123-456"));
    EXPECT_TRUE(IsDiskIdValid("disk-1-9999"));
    EXPECT_TRUE(IsDiskIdValid("disk-1-1"));
    EXPECT_TRUE(IsDiskIdValid("disk-9999-9999"));
    GTEST_LOG_(INFO) << "IsDiskIdValid_Valid_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_Empty_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_Empty_001 Start";
    EXPECT_FALSE(IsDiskIdValid(""));
    GTEST_LOG_(INFO) << "IsDiskIdValid_Empty_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_Traversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_Traversal_001 Start";
    EXPECT_FALSE(IsDiskIdValid("disk-../-1"));
    EXPECT_FALSE(IsDiskIdValid("disk-/.."));
    EXPECT_FALSE(IsDiskIdValid("../disk-1-1"));
    GTEST_LOG_(INFO) << "IsDiskIdValid_Traversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_WrongPrefix_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_WrongPrefix_001 Start";
    EXPECT_FALSE(IsDiskIdValid("vol-8-1"));
    EXPECT_FALSE(IsDiskIdValid("diskette-8-1"));
    EXPECT_FALSE(IsDiskIdValid("disk"));
    EXPECT_FALSE(IsDiskIdValid("disk-"));
    GTEST_LOG_(INFO) << "IsDiskIdValid_WrongPrefix_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_MissingSecondDash_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_MissingSecondDash_001 Start";
    EXPECT_FALSE(IsDiskIdValid("disk-8"));
    EXPECT_FALSE(IsDiskIdValid("disk-1234"));
    GTEST_LOG_(INFO) << "IsDiskIdValid_MissingSecondDash_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_ExtraDash_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_ExtraDash_001 Start";
    EXPECT_FALSE(IsDiskIdValid("disk-8-1-2"));
    EXPECT_FALSE(IsDiskIdValid("disk-1-2-3"));
    EXPECT_FALSE(IsDiskIdValid("disk-8-1-"));
    GTEST_LOG_(INFO) << "IsDiskIdValid_ExtraDash_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_SegmentNotDigits_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_SegmentNotDigits_001 Start";
    EXPECT_FALSE(IsDiskIdValid("disk-ab-1"));
    EXPECT_FALSE(IsDiskIdValid("disk-8-ab"));
    EXPECT_FALSE(IsDiskIdValid("disk-1a-2b"));
    GTEST_LOG_(INFO) << "IsDiskIdValid_SegmentNotDigits_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsDiskIdValid_SegmentOutOfRange_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsDiskIdValid_SegmentOutOfRange_001 Start";
    EXPECT_FALSE(IsDiskIdValid("disk-12345-1"));
    EXPECT_FALSE(IsDiskIdValid("disk-1-12345"));
    GTEST_LOG_(INFO) << "IsDiskIdValid_SegmentOutOfRange_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsUuidValid_Valid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUuidValid_Valid_001 Start";
    EXPECT_TRUE(IsUuidValid("abc123"));
    EXPECT_TRUE(IsUuidValid("ABC-123-def"));
    EXPECT_TRUE(IsUuidValid("12345678-9012-3456-7890-abcdef012345"));
    EXPECT_TRUE(IsUuidValid("a"));
    EXPECT_TRUE(IsUuidValid("0-9-A-Z-a-z"));
    GTEST_LOG_(INFO) << "IsUuidValid_Valid_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsUuidValid_Empty_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUuidValid_Empty_001 Start";
    EXPECT_FALSE(IsUuidValid(""));
    GTEST_LOG_(INFO) << "IsUuidValid_Empty_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsUuidValid_Traversal_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUuidValid_Traversal_001 Start";
    EXPECT_FALSE(IsUuidValid("../abc"));
    EXPECT_FALSE(IsUuidValid("/mnt/.."));
    EXPECT_FALSE(IsUuidValid("abc/.."));
    GTEST_LOG_(INFO) << "IsUuidValid_Traversal_001 End";
}

HWTEST_F(DiskManagerUtilsTest, IsUuidValid_TooLong_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUuidValid_TooLong_001 Start";
    std::string longUuid(UUID_MAX_LEN + 1, 'a');
    EXPECT_FALSE(IsUuidValid(longUuid));
    std::string maxUuid(UUID_MAX_LEN, 'a');
    EXPECT_TRUE(IsUuidValid(maxUuid));
    GTEST_LOG_(INFO) << "IsUuidValid_TooLong_001 End";
}
