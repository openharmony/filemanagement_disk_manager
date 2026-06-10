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

#include "partition_table_parser.h"

#include <gtest/gtest.h>

using namespace OHOS::DiskManager;
using namespace testing;
using namespace testing::ext;

class PartitionTableParserTest : public Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_001_0x06, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x06"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_002_0x07, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x07"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_003_0x0b, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x0b"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_004_0x0c, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x0c"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_005_0x0e, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x0e"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_006_0x1b, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x1b"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_007_0x83, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x83"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_008_Unsupported, TestSize.Level0)
{
    EXPECT_FALSE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x05"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_009_Unsupported02, TestSize.Level0)
{
    EXPECT_FALSE(PartitionTableParser::IsMbrTypeSupportedForVolume("0x82"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_010_Without0xPrefix, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("06"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_011_UpperCase0X, TestSize.Level0)
{
    EXPECT_TRUE(PartitionTableParser::IsMbrTypeSupportedForVolume("0X07"));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_012_EmptyString, TestSize.Level0)
{
    EXPECT_FALSE(PartitionTableParser::IsMbrTypeSupportedForVolume(""));
}

HWTEST_F(PartitionTableParserTest, IsMbrTypeSupported_TestCase_013_RandomHex, TestSize.Level0)
{
    EXPECT_FALSE(PartitionTableParser::IsMbrTypeSupportedForVolume("0xff"));
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_001_GptOnePartition, TestSize.Level0)
{
    std::string raw = "DISK gpt\nPART 1";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "gpt");
    EXPECT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0].diskId, "disk-8-0");
    EXPECT_EQ(parts[0].partitionNumber, 1u);
    EXPECT_EQ(parts[0].partitionType, "gpt");
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_002_MbrSupportedType, TestSize.Level0)
{
    std::string raw = "DISK mbr\nPART 1 0x07";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "mbr");
    EXPECT_EQ(parts.size(), 1u);
    EXPECT_EQ(parts[0].partitionType, "mbr");
    EXPECT_EQ(parts[0].fsTypeRaw, "0x07");
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_003_MbrUnsupportedType, TestSize.Level0)
{
    std::string raw = "DISK mbr\nPART 1 0x05";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "mbr");
    EXPECT_EQ(parts.size(), 0u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_004_PartNumZero, TestSize.Level0)
{
    std::string raw = "DISK gpt\nPART 0";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(parts.size(), 0u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_005_EmptyDump, TestSize.Level0)
{
    std::string raw = "";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_FALSE(ret);
    EXPECT_EQ(tableType, "");
    EXPECT_EQ(parts.size(), 0u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_006_OnlyWhitespace, TestSize.Level0)
{
    std::string raw = "  \n  \t  \n";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_FALSE(ret);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_007_NoDiskLine, TestSize.Level0)
{
    std::string raw = "PART 1";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_FALSE(ret);
    EXPECT_EQ(parts.size(), 0u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_008_UnknownTableType, TestSize.Level0)
{
    std::string raw = "DISK unknown\nPART 1";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "unknown");
    EXPECT_EQ(parts.size(), 0u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_009_MbrInsufficientTokens, TestSize.Level0)
{
    std::string raw = "DISK mbr\nPART 1";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "mbr");
    EXPECT_EQ(parts.size(), 0u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_010_GptMultiplePartitions, TestSize.Level0)
{
    std::string raw = "DISK gpt\nPART 1\nPART 2\nPART 3";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "gpt");
    EXPECT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0].partitionNumber, 1u);
    EXPECT_EQ(parts[1].partitionNumber, 2u);
    EXPECT_EQ(parts[2].partitionNumber, 3u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_011_GptUpperCase, TestSize.Level0)
{
    std::string raw = "DISK GPT\nPART 1";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "gpt");
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_012_Mbr0x83Supported, TestSize.Level0)
{
    std::string raw = "DISK mbr\nPART 1 0x83";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(parts.size(), 1u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_013_LineNotStartWithDiskOrPart, TestSize.Level0)
{
    std::string raw = "DISK gpt\nOTHER 1\nPART 1";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(parts.size(), 1u);
}

HWTEST_F(PartitionTableParserTest, ParseSgdiskDump_TestCase_014_DiskLineOnly, TestSize.Level0)
{
    std::string raw = "DISK gpt";
    std::string tableType;
    std::vector<PartitionRecord> parts;
    bool ret = PartitionTableParser::ParseSgdiskDump(raw, "disk-8-0", tableType, parts);
    EXPECT_TRUE(ret);
    EXPECT_EQ(tableType, "gpt");
    EXPECT_EQ(parts.size(), 0u);
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_001_BothSideSpaces, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim("  hello  "), "hello");
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_002_LeftSpacesOnly, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim("   hello"), "hello");
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_003_RightSpacesOnly, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim("hello   "), "hello");
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_004_Tabs, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim("\thello\t"), "hello");
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_005_MixedWhitespace, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim(" \t \rhello\r \t "), "hello");
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_006_AllWhitespace, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim(" \t\r "), "");
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_007_EmptyString, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim(""), "");
}

HWTEST_F(PartitionTableParserTest, Trim_TestCase_008_NoWhitespace, TestSize.Level0)
{
    EXPECT_EQ(PartitionTableParser::Trim("hello"), "hello");
}