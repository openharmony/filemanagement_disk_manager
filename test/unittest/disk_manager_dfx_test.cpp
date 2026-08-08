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
#include <sys/stat.h>
#include <unistd.h>

#include "disk_manager_dfx.h"
#include "disk_manager_errno.h"

namespace OHOS {
namespace DiskManager {
using namespace testing::ext;

namespace {
void EnsureHiAuditDir()
{
    if (access("/data/log", F_OK) != 0) {
        mkdir("/data/log", S_IRWXU | S_IRWXG | S_IXOTH);
    }
    if (access("/data/log/hiaudit", F_OK) != 0) {
        mkdir("/data/log/hiaudit", S_IRWXU | S_IRWXG | S_IXOTH);
    }
    if (access("/data/log/hiaudit/disk_manager/", F_OK) != 0) {
        mkdir("/data/log/hiaudit/disk_manager/", S_IRWXU | S_IRWXG | S_IXOTH);
    }
}
} // namespace

class DiskManagerDfxTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        EnsureHiAuditDir();
    }
    static void TearDownTestCase(void) {}
    void SetUp() override
    {
        EnsureHiAuditDir();
    }
    void TearDown() override {}
};

HWTEST_F(DiskManagerDfxTest, FinishSuccessAndFail_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-scope");
    {
        IpcDfxScope dfx("DiskManager::Format", DFX_STAGE_FORMAT, VolumeOpType::FORMAT, info);
        EXPECT_EQ(dfx.Finish(E_OK), E_OK);
        EXPECT_EQ(dfx.Finish(E_NON_EXIST), E_NON_EXIST);
    }
    {
        IpcDfxScope dfx("DiskManager::Mount", DFX_STAGE_MOUNT, VolumeOpType::MOUNT, info);
        VolumeReportInfo merge;
        merge.WithFsType("exfat");
        merge.extra = "k=1";
        dfx.MergeFrom(merge);
        EXPECT_EQ(dfx.Finish(E_VOL_STATE), E_VOL_STATE);
    }
}

HWTEST_F(DiskManagerDfxTest, DestructorFallback_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithDiskId("disk-x");
    {
        IpcDfxScope dfx("DiskManager::CreatePartition", DFX_STAGE_CREATE_PARTITION, VolumeOpType::CREATE_PARTITION,
                        info);
    }
    SUCCEED();
}

HWTEST_F(DiskManagerDfxTest, DestructorSkipAfterFinish_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-done");
    {
        IpcDfxScope dfx("DiskManager::Unmount", DFX_STAGE_UNMOUNT, VolumeOpType::UNMOUNT, info);
        EXPECT_EQ(dfx.Finish(E_OK), E_OK);
        // Destructor should not report again when finished_ is true.
    }
    SUCCEED();
}

HWTEST_F(DiskManagerDfxTest, MergeFromEmpty_001, TestSize.Level1)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-keep");
    IpcDfxScope dfx("DiskManager::FormatPartition", DFX_STAGE_FORMAT_PARTITION, VolumeOpType::FORMAT_PARTITION, info);
    VolumeReportInfo empty;
    dfx.MergeFrom(empty);
    EXPECT_EQ(dfx.Finish(E_OK), E_OK);
}

HWTEST_F(DiskManagerDfxTest, FinishSuccessPath_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-ok").WithDiskId("disk-ok").WithDevPath("/dev/block/sda1").WithFsType("exfat");
    info.fsUuid = "uuid-ok";
    IpcDfxScope dfx("DiskManager::GetPartitionTable", DFX_STAGE_GET_PARTITION_TABLE,
                    VolumeOpType::GET_PARTITION_TABLE, info);
    EXPECT_EQ(dfx.Finish(E_OK), E_OK);
}

HWTEST_F(DiskManagerDfxTest, AllStagesConstruct_001, TestSize.Level1)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-stage");
    const int32_t stages[] = {
        DFX_STAGE_MOUNT, DFX_STAGE_UNMOUNT, DFX_STAGE_FORMAT, DFX_STAGE_SET_VOLUME_DESCRIPTION,
        DFX_STAGE_UEVENT_PARSE, DFX_STAGE_GET_PARTITION_TABLE, DFX_STAGE_CREATE_PARTITION,
        DFX_STAGE_DELETE_PARTITION, DFX_STAGE_FORMAT_PARTITION,
    };
    for (int32_t stage : stages) {
        IpcDfxScope dfx("DiskManagerDfxTest::Stage", stage, VolumeOpType::OTHER, info);
        EXPECT_EQ(dfx.Finish(E_OK), E_OK);
    }
}
} // namespace DiskManager
} // namespace OHOS
