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

#include "disk/uevent_env_parser.h"

namespace OHOS {
namespace DiskManager {

using namespace testing::ext;

namespace {

constexpr unsigned int TEST_MAJOR = 8;
constexpr unsigned int TEST_MINOR = 0;
constexpr unsigned int TEST_MAJOR_MMC = 179;
constexpr unsigned int TEST_MINOR_MMC = 0;

} // namespace

class UeventEnvParserTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GTEST_LOG_(INFO) << "UeventEnvParserTest SetUpTestCase";
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "UeventEnvParserTest TearDownTestCase";
    }

    void SetUp() override
    {
        GTEST_LOG_(INFO) << "UeventEnvParserTest SetUp";
    }

    void TearDown() override
    {
        GTEST_LOG_(INFO) << "UeventEnvParserTest TearDown";
    }
};

/**
 * @tc.name: ToLowerTest001
 * @tc.desc: 测试 ToLower 方法，全大写字符串转换为小写。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ToLowerTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ToLowerTest001 Start";

    std::string input = "ACTION";
    UeventEnvParser::ToLower(input);
    EXPECT_EQ(input, "action");

    GTEST_LOG_(INFO) << "ToLowerTest001 End";
}

/**
 * @tc.name: ToLowerTest002
 * @tc.desc: 测试 ToLower 方法，大小写混合字符串转换为小写。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ToLowerTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ToLowerTest002 Start";

    std::string input = "SubSystem";
    UeventEnvParser::ToLower(input);
    EXPECT_EQ(input, "subsystem");

    GTEST_LOG_(INFO) << "ToLowerTest002 End";
}

/**
 * @tc.name: ToLowerTest003
 * @tc.desc: 测试 ToLower 方法，全小写字符串保持不变。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ToLowerTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ToLowerTest003 Start";

    std::string input = "devtype";
    UeventEnvParser::ToLower(input);
    EXPECT_EQ(input, "devtype");

    GTEST_LOG_(INFO) << "ToLowerTest003 End";
}

/**
 * @tc.name: ToLowerTest004
 * @tc.desc: 测试 ToLower 方法，空字符串保持不变。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ToLowerTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ToLowerTest004 Start";

    std::string input = "";
    UeventEnvParser::ToLower(input);
    EXPECT_EQ(input, "");

    GTEST_LOG_(INFO) << "ToLowerTest004 End";
}

/**
 * @tc.name: ToLowerTest005
 * @tc.desc: 测试 ToLower 方法，包含数字的字符串。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ToLowerTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ToLowerTest005 Start";

    std::string input = "ACTION123";
    UeventEnvParser::ToLower(input);
    EXPECT_EQ(input, "action123");

    GTEST_LOG_(INFO) << "ToLowerTest005 End";
}

/**
 * @tc.name: ToLowerTest006
 * @tc.desc: 测试 ToLower 方法，包含特殊字符的字符串。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ToLowerTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ToLowerTest006 Start";

    std::string input = "ACTION_TEST";
    UeventEnvParser::ToLower(input);
    EXPECT_EQ(input, "action_test");

    GTEST_LOG_(INFO) << "ToLowerTest006 End";
}

/**
 * @tc.name: IsBlockDiskEventTest001
 * @tc.desc: 测试 IsBlockDiskEvent，subsystem=block 且 devType=disk 时返回 true。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, IsBlockDiskEventTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsBlockDiskEventTest001 Start";

    UeventEnv env;
    env.subsystem = "block";
    env.devType = "disk";
    EXPECT_TRUE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "IsBlockDiskEventTest001 End";
}

/**
 * @tc.name: IsBlockDiskEventTest002
 * @tc.desc: 测试 IsBlockDiskEvent，subsystem 非 block 时返回 false。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, IsBlockDiskEventTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsBlockDiskEventTest002 Start";

    UeventEnv env;
    env.subsystem = "usb";
    env.devType = "disk";
    EXPECT_FALSE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "IsBlockDiskEventTest002 End";
}

/**
 * @tc.name: IsBlockDiskEventTest003
 * @tc.desc: 测试 IsBlockDiskEvent，devType 非 disk 时返回 false。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, IsBlockDiskEventTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsBlockDiskEventTest003 Start";

    UeventEnv env;
    env.subsystem = "block";
    env.devType = "partition";
    EXPECT_FALSE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "IsBlockDiskEventTest003 End";
}

/**
 * @tc.name: IsBlockDiskEventTest004
 * @tc.desc: 测试 IsBlockDiskEvent，subsystem 和 devType 都不符合时返回 false。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, IsBlockDiskEventTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsBlockDiskEventTest004 Start";

    UeventEnv env;
    env.subsystem = "net";
    env.devType = "interface";
    EXPECT_FALSE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "IsBlockDiskEventTest004 End";
}

/**
 * @tc.name: IsBlockDiskEventTest005
 * @tc.desc: 测试 IsBlockDiskEvent，空字符串字段时返回 false。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, IsBlockDiskEventTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsBlockDiskEventTest005 Start";

    UeventEnv env;
    env.subsystem = "";
    env.devType = "";
    EXPECT_FALSE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "IsBlockDiskEventTest005 End";
}

/**
 * @tc.name: IsBlockDiskEventTest006
 * @tc.desc: 测试 IsBlockDiskEvent，大小写不匹配（block vs Block）时返回 false。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, IsBlockDiskEventTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "IsBlockDiskEventTest006 Start";

    UeventEnv env;
    env.subsystem = "Block";
    env.devType = "Disk";
    EXPECT_FALSE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "IsBlockDiskEventTest006 End";
}

/**
 * @tc.name: ParseTest001
 * @tc.desc: 测试 Parse 方法，标准完整的 uevent 消息解析成功。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest001 Start";

    std::string raw =
        "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nMAJOR=8\nMINOR=0\n"
        "DEVPATH=/devices/sda\nDEVNAME=sda";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "add");
    EXPECT_EQ(env.subsystem, "block");
    EXPECT_EQ(env.devType, "disk");
    EXPECT_EQ(env.major, TEST_MAJOR);
    EXPECT_EQ(env.minor, TEST_MINOR);
    EXPECT_EQ(env.devPath, "/devices/sda");
    EXPECT_EQ(env.sysPath, "/sys/devices/sda");
    EXPECT_EQ(env.devName, "sda");

    GTEST_LOG_(INFO) << "ParseTest001 End";
}

/**
 * @tc.name: ParseTest002
 * @tc.desc: 测试 Parse 方法，缺少 action 字段时解析失败。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest002 Start";

    std::string raw = "SUBSYSTEM=block\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_FALSE(ret);
    EXPECT_TRUE(env.action.empty());

    GTEST_LOG_(INFO) << "ParseTest002 End";
}

/**
 * @tc.name: ParseTest003
 * @tc.desc: 测试 Parse 方法，缺少 subsystem 字段时解析失败。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest003 Start";

    std::string raw = "ACTION=add\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_FALSE(ret);
    EXPECT_TRUE(env.subsystem.empty());

    GTEST_LOG_(INFO) << "ParseTest003 End";
}

/**
 * @tc.name: ParseTest004
 * @tc.desc: 测试 Parse 方法，缺少 devtype 字段时解析失败。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest004 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_FALSE(ret);
    EXPECT_TRUE(env.devType.empty());

    GTEST_LOG_(INFO) << "ParseTest004 End";
}

/**
 * @tc.name: ParseTest005
 * @tc.desc: 测试 Parse 方法，空消息时解析失败。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest005 Start";

    std::string raw = "";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_FALSE(ret);

    GTEST_LOG_(INFO) << "ParseTest005 End";
}

/**
 * @tc.name: ParseTest006
 * @tc.desc: 测试 Parse 方法，包含 disk_eject_request 字段解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest006 Start";

    std::string raw = "ACTION=change\nSUBSYSTEM=block\nDEVTYPE=disk\nDISK_EJECT_REQUEST=1";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "change");
    EXPECT_TRUE(env.ejectRequest);

    GTEST_LOG_(INFO) << "ParseTest006 End";
}

/**
 * @tc.name: ParseTest007
 * @tc.desc: 测试 Parse 方法，disk_eject_request 非 1 时为 false。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest007 Start";

    std::string raw = "ACTION=change\nSUBSYSTEM=block\nDEVTYPE=disk\nDISK_EJECT_REQUEST=0";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_FALSE(env.ejectRequest);

    GTEST_LOG_(INFO) << "ParseTest007 End";
}

/**
 * @tc.name: ParseTest008
 * @tc.desc: 测试 Parse 方法，无 disk_eject_request 字段时默认为 false。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest008 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_FALSE(env.ejectRequest);

    GTEST_LOG_(INFO) << "ParseTest008 End";
}

/**
 * @tc.name: ParseTest009
 * @tc.desc: 测试 Parse 方法，大写 ACTION 字段转换为小写。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest009, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest009 Start";

    std::string raw = "ACTION=ADD\nSUBSYSTEM=BLOCK\nDEVTYPE=DISK";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "add");
    EXPECT_EQ(env.subsystem, "block");
    EXPECT_EQ(env.devType, "disk");

    GTEST_LOG_(INFO) << "ParseTest009 End";
}

/**
 * @tc.name: ParseTest010
 * @tc.desc: 测试 Parse 方法，大小写混合字段正确转换。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest010, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest010 Start";

    std::string raw = "Action=Remove\nSubSystem=Block\nDevType=Disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "remove");
    EXPECT_EQ(env.subsystem, "block");
    EXPECT_EQ(env.devType, "disk");

    GTEST_LOG_(INFO) << "ParseTest010 End";
}

/**
 * @tc.name: ParseTest011
 * @tc.desc: 测试 Parse 方法，包含无效行（无等号）时跳过该行。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest011, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest011 Start";

    std::string raw = "ACTION=add\nINVALID_LINE\nSUBSYSTEM=block\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "add");
    EXPECT_EQ(env.subsystem, "block");
    EXPECT_EQ(env.devType, "disk");

    GTEST_LOG_(INFO) << "ParseTest011 End";
}

/**
 * @tc.name: ParseTest012
 * @tc.desc: 测试 Parse 方法，MMC 设备解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest012, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest012 Start";

    std::string raw =
        "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nMAJOR=179\nMINOR=0\n"
        "DEVPATH=/devices/mmcblk0\nDEVNAME=mmcblk0";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.major, TEST_MAJOR_MMC);
    EXPECT_EQ(env.minor, TEST_MINOR_MMC);
    EXPECT_EQ(env.devName, "mmcblk0");
    EXPECT_TRUE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "ParseTest012 End";
}

/**
 * @tc.name: ParseTest013
 * @tc.desc: 测试 Parse 方法，action=remove 解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest013, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest013 Start";

    std::string raw = "ACTION=remove\nSUBSYSTEM=block\nDEVTYPE=disk\nDEVPATH=/devices/sda";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "remove");

    GTEST_LOG_(INFO) << "ParseTest013 End";
}

/**
 * @tc.name: ParseTest014
 * @tc.desc: 测试 Parse 方法，action=change 解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest014, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest014 Start";

    std::string raw = "ACTION=change\nSUBSYSTEM=block\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "change");

    GTEST_LOG_(INFO) << "ParseTest014 End";
}

/**
 * @tc.name: ParseTest015
 * @tc.desc: 测试 Parse 方法，devtype=partition 不是 disk 事件。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest015, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest015 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=partition\nMAJOR=8\nMINOR=1";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.devType, "partition");
    EXPECT_FALSE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "ParseTest015 End";
}

/**
 * @tc.name: ParseTest016
 * @tc.desc: 测试 Parse 方法，MAJOR/MINOR 数值解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest016, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest016 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nMAJOR=254\nMINOR=128";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.major, 254);
    EXPECT_EQ(env.minor, 128);

    GTEST_LOG_(INFO) << "ParseTest016 End";
}

/**
 * @tc.name: ParseTest017
 * @tc.desc: 测试 Parse 方法，DEVPATH 生成正确的 sysPath。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest017, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest017 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nDEVPATH=/devices/platform/soc/usb";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.devPath, "/devices/platform/soc/usb");
    EXPECT_EQ(env.sysPath, "/sys/devices/platform/soc/usb");

    GTEST_LOG_(INFO) << "ParseTest017 End";
}

/**
 * @tc.name: ParseTest018
 * @tc.desc: 测试 Parse 方法，无 DEVPATH 时 sysPath 为空。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest018, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest018 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_TRUE(env.devPath.empty());
    EXPECT_TRUE(env.sysPath.empty());

    GTEST_LOG_(INFO) << "ParseTest018 End";
}

/**
 * @tc.name: ParseTest019
 * @tc.desc: 测试 Parse 方法，无 DEVNAME 时 devName 为空。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest019, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest019 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_TRUE(env.devName.empty());

    GTEST_LOG_(INFO) << "ParseTest019 End";
}

/**
 * @tc.name: ParseTest020
 * @tc.desc: 测试 Parse 方法，无 MAJOR/MINOR 时默认为 0。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest020, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest020 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.major, 0);
    EXPECT_EQ(env.minor, 0);

    GTEST_LOG_(INFO) << "ParseTest020 End";
}

/**
 * @tc.name: ParseTest021
 * @tc.desc: 测试 Parse 方法，包含多个未知字段时忽略并正常解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest021, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest021 Start";

    std::string raw =
        "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nUNKNOWN_FIELD=value\n"
        "ANOTHER_UNKNOWN=test";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "add");
    EXPECT_EQ(env.subsystem, "block");
    EXPECT_EQ(env.devType, "disk");

    GTEST_LOG_(INFO) << "ParseTest021 End";
}

/**
 * @tc.name: ParseTest022
 * @tc.desc: 测试 Parse 方法，值中包含等号时正确解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest022, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest022 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nDEVPATH=/path=a=b";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.devPath, "/path=a=b");
    EXPECT_EQ(env.sysPath, "/sys/path=a=b");

    GTEST_LOG_(INFO) << "ParseTest022 End";
}

/**
 * @tc.name: ParseTest023
 * @tc.desc: 测试 Parse 方法，连续两个换行符。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest023, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest023 Start";

    std::string raw = "ACTION=add\n\nSUBSYSTEM=block\n\nDEVTYPE=disk";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "add");
    EXPECT_EQ(env.subsystem, "block");
    EXPECT_EQ(env.devType, "disk");

    GTEST_LOG_(INFO) << "ParseTest023 End";
}

/**
 * @tc.name: ParseTest024
 * @tc.desc: 测试 Parse 方法，行末换行符。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest024, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest024 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\n";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.action, "add");
    EXPECT_EQ(env.subsystem, "block");
    EXPECT_EQ(env.devType, "disk");

    GTEST_LOG_(INFO) << "ParseTest024 End";
}

/**
 * @tc.name: ParseTest025
 * @tc.desc: 测试 Parse 方法，MAJOR 为非数字时解析为 0。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest025, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest025 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nMAJOR=invalid";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.major, 0);

    GTEST_LOG_(INFO) << "ParseTest025 End";
}

/**
 * @tc.name: ParseTest026
 * @tc.desc: 测试 Parse 方法，MAJOR 为空值时解析为 0。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseTest026, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseTest026 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nMAJOR=";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.major, 0);

    GTEST_LOG_(INFO) << "ParseTest026 End";
}

/**
 * @tc.name: UeventEnvDefaultTest001
 * @tc.desc: 测试 UeventEnv 默认值。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, UeventEnvDefaultTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UeventEnvDefaultTest001 Start";

    UeventEnv env;
    EXPECT_EQ(env.major, 0);
    EXPECT_EQ(env.minor, 0);
    EXPECT_FALSE(env.ejectRequest);
    EXPECT_TRUE(env.action.empty());
    EXPECT_TRUE(env.subsystem.empty());
    EXPECT_TRUE(env.devType.empty());
    EXPECT_TRUE(env.devPath.empty());
    EXPECT_TRUE(env.sysPath.empty());
    EXPECT_TRUE(env.devName.empty());

    GTEST_LOG_(INFO) << "UeventEnvDefaultTest001 End";
}

/**
 * @tc.name: ParseAndCheckBlockDiskTest001
 * @tc.desc: 测试解析后调用 IsBlockDiskEvent 返回 true。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseAndCheckBlockDiskTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseAndCheckBlockDiskTest001 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nMAJOR=8\nMINOR=0";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_TRUE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "ParseAndCheckBlockDiskTest001 End";
}

/**
 * @tc.name: ParseAndCheckBlockDiskTest002
 * @tc.desc: 测试解析后调用 IsBlockDiskEvent 返回 false（partition）。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseAndCheckBlockDiskTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseAndCheckBlockDiskTest002 Start";

    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=partition";
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_FALSE(env.IsBlockDiskEvent());

    GTEST_LOG_(INFO) << "ParseAndCheckBlockDiskTest002 End";
}

/**
 * @tc.name: ParseResetTest001
 * @tc.desc: 测试 Parse 方法，多次调用会重置输出结构体。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseResetTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseResetTest001 Start";

    UeventEnv env;

    std::string raw1 = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nMAJOR=8\nMINOR=0";
    bool ret1 = UeventEnvParser::Parse(raw1, env);
    EXPECT_TRUE(ret1);
    EXPECT_EQ(env.major, TEST_MAJOR);

    std::string raw2 = "ACTION=remove\nSUBSYSTEM=block\nDEVTYPE=disk";
    bool ret2 = UeventEnvParser::Parse(raw2, env);
    EXPECT_TRUE(ret2);
    EXPECT_EQ(env.action, "remove");
    EXPECT_EQ(env.major, 0);

    GTEST_LOG_(INFO) << "ParseResetTest001 End";
}

/**
 * @tc.name: ParseLongDevpathTest001
 * @tc.desc: 测试 Parse 方法，长 DEVPATH 解析。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(UeventEnvParserTest, ParseLongDevpathTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ParseLongDevpathTest001 Start";

    std::string longPath =
        "/devices/platform/soc/fe300000.dwc3/xhci-hcd.0.auto/usb1/1-1/1-1:1.0/host0/target0:0:0/0:0:0:0/block/sda";
    std::string raw = "ACTION=add\nSUBSYSTEM=block\nDEVTYPE=disk\nDEVPATH=" + longPath;
    UeventEnv env;
    bool ret = UeventEnvParser::Parse(raw, env);

    EXPECT_TRUE(ret);
    EXPECT_EQ(env.devPath, longPath);
    EXPECT_EQ(env.sysPath, "/sys" + longPath);

    GTEST_LOG_(INFO) << "ParseLongDevpathTest001 End";
}

} // namespace DiskManager
} // namespace OHOS