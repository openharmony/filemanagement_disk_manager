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

#include "block_info.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class BlockInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(BlockInfoTest, ToJson_TestCase_001, TestSize.Level0)
{
    BlockInfo info;
    info.sizeBytes = 1024;
    info.vendor = "VendorA";
    info.model = "Model-A";
    info.interfaceType = "USB";
    info.rpm = 5400;
    info.removable = true;
    info.serialNumber = "SN-001";
    info.diskId = "disk-1";
    info.devicePath = "/dev/sda";
    info.port = "1";
    info.devnum = "2";
    info.busnum = "3";
    info.devNode = "sda";
    info.scsiBusNum = "0";
    info.fwVersion = "1.0";
    info.ODD_INFO = nlohmann::json{{"key", "value"}};

    auto j = info.ToJson();
    EXPECT_EQ(j["sizeBytes"].get<uint64_t>(), 1024u);
    EXPECT_EQ(j["vendor"].get<std::string>(), "VendorA");
    EXPECT_EQ(j["model"].get<std::string>(), "Model-A");
    EXPECT_EQ(j["interfaceType"].get<std::string>(), "USB");
    EXPECT_EQ(j["rpm"].get<uint32_t>(), 5400u);
    EXPECT_EQ(j["removable"].get<bool>(), true);
    EXPECT_EQ(j["serialNumber"].get<std::string>(), "SN-001");
    EXPECT_EQ(j["diskId"].get<std::string>(), "disk-1");
    EXPECT_EQ(j["devicePath"].get<std::string>(), "/dev/sda");
    EXPECT_EQ(j["port"].get<std::string>(), "1");
    EXPECT_EQ(j["devnum"].get<std::string>(), "2");
    EXPECT_EQ(j["busnum"].get<std::string>(), "3");
    EXPECT_EQ(j["devNode"].get<std::string>(), "sda");
    EXPECT_EQ(j["scsiBusNum"].get<std::string>(), "0");
    EXPECT_EQ(j["fwVersion"].get<std::string>(), "1.0");
    EXPECT_EQ(j["ODD_INFO"]["key"].get<std::string>(), "value");
}

HWTEST_F(BlockInfoTest, ToJson_DefaultValues_TestCase_001, TestSize.Level0)
{
    BlockInfo info;
    auto j = info.ToJson();
    EXPECT_EQ(j["sizeBytes"].get<uint64_t>(), 0u);
    EXPECT_EQ(j["vendor"].get<std::string>(), "");
    EXPECT_EQ(j["removable"].get<bool>(), false);
}

HWTEST_F(BlockInfoTest, SerializeVector_TestCase_001, TestSize.Level0)
{
    std::vector<BlockInfo> infos;
    BlockInfo info1;
    info1.sizeBytes = 512;
    info1.vendor = "Vendor1";
    info1.devNode = "sda";
    infos.push_back(info1);

    BlockInfo info2;
    info2.sizeBytes = 1024;
    info2.vendor = "Vendor2";
    info2.devNode = "sdb";
    infos.push_back(info2);

    std::string result = BlockInfo::SerializeVector(infos);
    EXPECT_FALSE(result.empty());

    auto j = nlohmann::json::parse(result);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2u);
    EXPECT_EQ(j[0]["vendor"].get<std::string>(), "Vendor1");
    EXPECT_EQ(j[0]["sizeBytes"].get<uint64_t>(), 512u);
    EXPECT_EQ(j[1]["vendor"].get<std::string>(), "Vendor2");
    EXPECT_EQ(j[1]["sizeBytes"].get<uint64_t>(), 1024u);
}

HWTEST_F(BlockInfoTest, SerializeVector_Empty_TestCase_001, TestSize.Level0)
{
    std::vector<BlockInfo> infos;
    std::string result = BlockInfo::SerializeVector(infos);
    EXPECT_EQ(result, "[]");

    auto j = nlohmann::json::parse(result);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 0u);
}

HWTEST_F(BlockInfoTest, SerializeVector_SingleElement_TestCase_001, TestSize.Level0)
{
    std::vector<BlockInfo> infos;
    BlockInfo info;
    info.sizeBytes = 2048;
    info.vendor = "Samsung";
    info.diskId = "disk-0";
    infos.push_back(info);

    std::string result = BlockInfo::SerializeVector(infos);
    auto j = nlohmann::json::parse(result);
    EXPECT_EQ(j.size(), 1u);
    EXPECT_EQ(j[0]["vendor"].get<std::string>(), "Samsung");
    EXPECT_EQ(j[0]["diskId"].get<std::string>(), "disk-0");
}

} // namespace DiskManager
} // namespace OHOS
