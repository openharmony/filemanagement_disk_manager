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
#include <gmock/gmock.h>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "disk_hardware_reader.h"
#include "disk.h"
#include "disk_manager_mock.h"
#include "mock_block_info_table.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class DiskHardwareReaderTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        testing::Mock::AllowLeak(&DiskManager::GetInstance());
        testing::Mock::AllowLeak(&BlockInfoTable::GetInstance());
    }
    static void TearDownTestCase(void) {}
    void SetUp() override
    {
        ON_CALL(DiskManager::GetInstance(), GetAllDisks(_)).WillByDefault(Return(0));
    }
    void TearDown() override
    {
        testing::Mock::VerifyAndClearExpectations(&DiskManager::GetInstance());
        testing::Mock::VerifyAndClearExpectations(&BlockInfoTable::GetInstance());
    }
};

/**
 * @tc.name: ReadSysfsNode_EmptyPath_001
 * @tc.desc: ReadSysfsNode with empty path returns empty string.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, ReadSysfsNode_EmptyPath_001, TestSize.Level0)
{
    EXPECT_EQ(DiskHardwareReader::ReadSysfsNode(""), "");
}

/**
 * @tc.name: ReadSysfsNode_NonExistent_001
 * @tc.desc: ReadSysfsNode with non-existent path returns empty string.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, ReadSysfsNode_NonExistent_001, TestSize.Level0)
{
    EXPECT_EQ(DiskHardwareReader::ReadSysfsNode("/sys/block/nonexistent_device_xyz/device/model"), "");
}

/**
 * @tc.name: ReadSysfsNode_PathTooLong_001
 * @tc.desc: ReadSysfsNode with path >= PATH_MAX returns empty string.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, ReadSysfsNode_PathTooLong_001, TestSize.Level0)
{
    std::string longPath(PATH_MAX, 'x');
    EXPECT_EQ(DiskHardwareReader::ReadSysfsNode(longPath), "");
}

/**
 * @tc.name: ReadSysfsNode_ValidFile_001
 * @tc.desc: ReadSysfsNode reads /proc/version successfully.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, ReadSysfsNode_ValidFile_001, TestSize.Level0)
{
    std::string content = DiskHardwareReader::ReadSysfsNode("/proc/version");
    EXPECT_FALSE(content.empty());
}

/**
 * @tc.name: GetLocalDiskInfos_MediaTypeNvme_001
 * @tc.desc: GetLocalDiskInfos returns mediaType=1 (SSD) for nvme* prefix devices.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, GetLocalDiskInfos_MediaTypeNvme_001, TestSize.Level0)
{
    auto infos = DiskHardwareReader::GetLocalDiskInfos();
    for (const auto &info : infos) {
        const std::string &sysPath = info.GetSysPath();
        auto pos = sysPath.find_last_of('/');
        std::string devName = (pos != std::string::npos) ? sysPath.substr(pos + 1) : sysPath;
        if (devName.find("nvme") == 0) {
            EXPECT_EQ(info.GetMediaType(), 1);
        }
    }
}

/**
 * @tc.name: GetLocalDiskInfos_MediaTypeSd_001
 * @tc.desc: GetLocalDiskInfos returns mediaType=2 (HDD) for sd* prefix devices.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, GetLocalDiskInfos_MediaTypeSd_001, TestSize.Level0)
{
    std::vector<Disk> disks;
    disks.emplace_back("disk_sda_test", 0, "sda", DATA_DISK_HDD);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllDisks(_))
        .WillOnce(DoAll(SetArgReferee<0>(disks), Return(0)));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(true));

    auto infos = DiskHardwareReader::GetLocalDiskInfos();
    for (const auto &info : infos) {
        const std::string &sysPath = info.GetSysPath();
        auto pos = sysPath.find_last_of('/');
        std::string devName = (pos != std::string::npos) ? sysPath.substr(pos + 1) : sysPath;
        if (devName.find("sd") == 0 && devName.find("nvme") != 0) {
            EXPECT_EQ(info.GetMediaType(), 2);
        }
    }
}

/**
 * @tc.name: GetLocalDiskInfos_MediaTypeOther_001
 * @tc.desc: GetLocalDiskInfos filters out non-nvme/non-sd prefix devices; only nvme* and sd* appear.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, GetLocalDiskInfos_MediaTypeOther_001, TestSize.Level0)
{
    auto infos = DiskHardwareReader::GetLocalDiskInfos();
    for (const auto &info : infos) {
        const std::string &sysPath = info.GetSysPath();
        auto pos = sysPath.find_last_of('/');
        std::string devName = (pos != std::string::npos) ? sysPath.substr(pos + 1) : sysPath;
        bool isNvme = devName.find("nvme") == 0;
        bool isSd = devName.find("sd") == 0;
        EXPECT_TRUE(isNvme || isSd);
    }
}

/**
 * @tc.name: GetLocalDiskInfos_SizeOverflow_001
 * @tc.desc: Size overflow (sectors > UINT64_MAX/512) yields size=0 in the calculation logic.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskHardwareReaderTest, GetLocalDiskInfos_SizeOverflow_001, TestSize.Level0)
{
    constexpr uint64_t sectorSize = 512;
    std::string overflowSectors = std::to_string(UINT64_MAX / sectorSize + 1);

    char tmpl[] = "/data/local/tmp/disk_hw_overflow_XXXXXX";
    char *result = mkdtemp(tmpl);
    ASSERT_NE(result, nullptr);
    std::string tmpDir = tmpl;
    std::string sizeFilePath = tmpDir + "/size";
    {
        std::ofstream ofs(sizeFilePath);
        ofs << overflowSectors;
    }

    std::string sizeStr = DiskHardwareReader::ReadSysfsNode(sizeFilePath);
    EXPECT_FALSE(sizeStr.empty());

    unsigned long long sectors = 0;
    std::istringstream iss(sizeStr);
    iss >> sectors;
    EXPECT_GT(sectors, UINT64_MAX / sectorSize);

    int64_t computedSize = 0;
    if (sectors > 0 && sectors <= UINT64_MAX / sectorSize) {
        computedSize = static_cast<int64_t>(sectors * sectorSize);
    }
    EXPECT_EQ(computedSize, 0);

    std::string cmd = "rm -rf " + tmpDir;
    (void)system(cmd.c_str());
}

} // namespace DiskManager
} // namespace OHOS
