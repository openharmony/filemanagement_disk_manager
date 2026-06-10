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

#include "disk_config.h"

#include <gtest/gtest.h>
#include <fnmatch.h>

using namespace OHOS::DiskManager;
using namespace testing;
using namespace testing::ext;

class DiskConfigTest : public Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(DiskConfigTest, Constructor_TestCase_001, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/sda*", "SD_CARD", 1);
    EXPECT_EQ(cfg.GetFlag(), 1);
}

HWTEST_F(DiskConfigTest, Constructor_TestCase_002, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/usb*", "USB", 2);
    EXPECT_EQ(cfg.GetFlag(), 2);
}

HWTEST_F(DiskConfigTest, Constructor_TestCase_003, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/mmc*", "MMC", 0);
    EXPECT_EQ(cfg.GetFlag(), 0);
}

HWTEST_F(DiskConfigTest, Constructor_TestCase_004, TestSize.Level0)
{
    DiskConfig cfg("", "", -1);
    EXPECT_EQ(cfg.GetFlag(), -1);
}

HWTEST_F(DiskConfigTest, Destructor_TestCase_001, TestSize.Level0)
{
    DiskConfig *cfg = new DiskConfig("/dev/block/sda*", "SD", 1);
    delete cfg;
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_001_ExactMatch, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/sda", "SD", 1);
    std::string sysPattern = "/dev/block/sda";
    EXPECT_TRUE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_002_WildcardStar, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/sda*", "SD", 1);
    std::string sysPattern = "/dev/block/sda1";
    EXPECT_TRUE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_003_WildcardStarNoMatch, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/sda*", "SD", 1);
    std::string sysPattern = "/dev/block/sdb1";
    EXPECT_FALSE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_004_WildcardQuestion, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/sda?", "SD", 1);
    std::string sysPattern = "/dev/block/sda1";
    EXPECT_TRUE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_005_WildcardQuestionNoMatch, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/sda?", "SD", 1);
    std::string sysPattern = "/dev/block/sda12";
    EXPECT_FALSE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_006_UsbPattern, TestSize.Level0)
{
    DiskConfig cfg("/devices/*/usb*", "USB", 2);
    std::string sysPattern = "/devices/pci/usb1";
    EXPECT_TRUE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_007_EmptyPattern, TestSize.Level0)
{
    DiskConfig cfg("", "EMPTY", 0);
    std::string sysPattern = "";
    EXPECT_TRUE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_008_EmptyPatternNonEmptyDev, TestSize.Level0)
{
    DiskConfig cfg("", "EMPTY", 0);
    std::string sysPattern = "/dev/block/sda";
    EXPECT_FALSE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_009_NonEmptyPatternEmptyDev, TestSize.Level0)
{
    DiskConfig cfg("/dev/block/sda*", "SD", 1);
    std::string sysPattern = "";
    EXPECT_FALSE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, IsMatch_TestCase_010_MmcPattern, TestSize.Level0)
{
    DiskConfig cfg("/devices/platform/*/mmc*", "MMC", 179);
    std::string sysPattern = "/devices/platform/soc/mmcblk0";
    EXPECT_TRUE(cfg.IsMatch(sysPattern));
}

HWTEST_F(DiskConfigTest, GetFlag_TestCase_001_PositiveFlag, TestSize.Level0)
{
    DiskConfig cfg("pattern", "label", 42);
    EXPECT_EQ(cfg.GetFlag(), 42);
}

HWTEST_F(DiskConfigTest, GetFlag_TestCase_002_ZeroFlag, TestSize.Level0)
{
    DiskConfig cfg("pattern", "label", 0);
    EXPECT_EQ(cfg.GetFlag(), 0);
}

HWTEST_F(DiskConfigTest, GetFlag_TestCase_003_NegativeFlag, TestSize.Level0)
{
    DiskConfig cfg("pattern", "label", -5);
    EXPECT_EQ(cfg.GetFlag(), -5);
}