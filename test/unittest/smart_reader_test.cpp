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
#include <string>

#include "smart_reader.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class SmartReaderTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: GetSerialNumber_Nvme_NonExistent_001
 * @tc.desc: GetSerialNumber for non-existent NVMe device returns "Unknown".
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SmartReaderTest, GetSerialNumber_Nvme_NonExistent_001, TestSize.Level0)
{
    std::string serial = SmartReader::GetSerialNumber("nvme99n1", true);
    EXPECT_EQ(serial, "Unknown");
}

/**
 * @tc.name: GetSerialNumber_Hdd_NonExistent_001
 * @tc.desc: GetSerialNumber for non-existent HDD device returns "Unknown".
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SmartReaderTest, GetSerialNumber_Hdd_NonExistent_001, TestSize.Level0)
{
    std::string serial = SmartReader::GetSerialNumber("sdz9", false);
    EXPECT_EQ(serial, "Unknown");
}

/**
 * @tc.name: GetSerialNumber_Nvme_Valid_001
 * @tc.desc: GetSerialNumber for valid NVMe device returns non-empty string or "Unknown".
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SmartReaderTest, GetSerialNumber_Nvme_Valid_001, TestSize.Level0)
{
    std::string serial = SmartReader::GetSerialNumber("nvme0n1", true);
    EXPECT_TRUE(serial == "Unknown" || !serial.empty());
}

/**
 * @tc.name: GetSerialNumber_EmptyDevName_001
 * @tc.desc: GetSerialNumber with empty devName returns "Unknown".
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SmartReaderTest, GetSerialNumber_EmptyDevName_001, TestSize.Level0)
{
    std::string serial = SmartReader::GetSerialNumber("", true);
    EXPECT_EQ(serial, "Unknown");
}

} // namespace DiskManager
} // namespace OHOS
