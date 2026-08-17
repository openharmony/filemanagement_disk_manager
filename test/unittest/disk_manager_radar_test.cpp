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

#include "disk_manager_dfx_types.h"
#include "disk_manager_errno.h"
#include "disk_manager_radar.h"

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

class DiskManagerRadarTest : public testing::Test {
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

HWTEST_F(DiskManagerRadarTest, GetInstance_001, TestSize.Level0)
{
    auto &a = DiskManagerRadar::GetInstance();
    auto &b = DiskManagerRadar::GetInstance();
    EXPECT_EQ(&a, &b);
}

HWTEST_F(DiskManagerRadarTest, ReportBehavior_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-b").WithFsType("exfat");
    info.WithDevPath("/dev/block/sda1");
    info.fsUuid = "fs-uuid-b";
    info.extra = "k=1";
    DiskManagerRadar::GetInstance().ReportBehavior("DiskManager::Mount", DFX_STAGE_MOUNT, "START", info, E_OK);
    DiskManagerRadar::GetInstance().ReportBehavior("DiskManager::Mount", DFX_STAGE_MOUNT, "SUCCESS", info, E_OK);
    DiskManagerRadar::GetInstance().ReportBehavior("DiskManager::Mount", DFX_STAGE_MOUNT, "FAIL", info, E_NON_EXIST);

    // empty extra → ReportBehavior skips "|extra" append branch
    VolumeReportInfo empty;
    DiskManagerRadar::GetInstance().ReportBehavior("DiskManager::Unmount", DFX_STAGE_UNMOUNT, "SUCCESS", empty, E_OK);
    SUCCEED();
}

HWTEST_F(DiskManagerRadarTest, RecordFault_001, TestSize.Level0)
{
    RadarParameter param;
    param.funcName = "DiskManagerRadar::RecordFault";
    param.bizStage = DFX_STAGE_MOUNT;
    param.errorCode = E_OTHER_MOUNT;
    param.fileStatus = R"({"devPath":"/dev/block/sda1"})";
    (void)DiskManagerRadar::GetInstance().RecordFault(param);
    SUCCEED();
}

HWTEST_F(DiskManagerRadarTest, ReportVolumeFault_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithDevPath("/dev/block/sda1").WithFsType("ntfs");
    DiskManagerRadar::GetInstance().ReportVolumeFault("DiskManager::Format", DFX_STAGE_FORMAT, VolumeOpType::FORMAT,
                                                      E_PARAMS_INVALID, info);
    SUCCEED();
}

HWTEST_F(DiskManagerRadarTest, ReportMetadataFault_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithDevPath("/dev/block/sda1");
    DiskManagerRadar::GetInstance().ReportMetadataFault("DiskManager::ReadMetadata", E_NON_EXIST, info);
    SUCCEED();
}

HWTEST_F(DiskManagerRadarTest, RecordFault_CustomFields_001, TestSize.Level1)
{
    RadarParameter param;
    param.orgPkg = "customPkg";
    param.userId = 200;
    param.funcName = "DiskManagerRadar::RecordFaultCustom";
    param.bizScene = BizScene::EXTERNAL_VOLUME_MANAGER;
    param.bizStage = DFX_STAGE_FORMAT;
    param.keyElxLevel = "EL1";
    param.toCallPkg = "storage_daemon";
    param.fileStatus = R"({"fsType":"exfat"})";
    param.errorCode = E_PARAMS_INVALID;
    (void)DiskManagerRadar::GetInstance().RecordFault(param);
    SUCCEED();
}

HWTEST_F(DiskManagerRadarTest, ReportVolumeFault_AllOpTypes_001, TestSize.Level1)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-op");
    info.fsUuid = "uuid-op";
    const VolumeOpType ops[] = {
        VolumeOpType::MOUNT, VolumeOpType::UNMOUNT, VolumeOpType::FORMAT, VolumeOpType::SET_VOLUME_DESCRIPTION,
        VolumeOpType::OTHER, VolumeOpType::GET_PARTITION_TABLE, VolumeOpType::CREATE_PARTITION,
        VolumeOpType::DELETE_PARTITION, VolumeOpType::FORMAT_PARTITION,
    };
    for (auto op : ops) {
        DiskManagerRadar::GetInstance().ReportVolumeFault("DiskManagerRadar::ReportVolumeFault", DFX_STAGE_MOUNT, op,
                                                          E_OTHER_MOUNT, info);
    }
    SUCCEED();
}

HWTEST_F(DiskManagerRadarTest, ReportUeventParseFault_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.extra = "ueventMsg=ACTION=add";
    DiskManagerRadar::GetInstance().ReportUeventParseFault(info);
    SUCCEED();
}

HWTEST_F(DiskManagerRadarTest, ReportDiscoverAutoMountSkipFault_001, TestSize.Level0)
{
    AutoMountSkipContext ctx;
    ctx.volId = "vol-1";
    ctx.diskId = "disk-8-0";
    ctx.volDevPath = "/dev/block/vol-1";
    ctx.autoMountEnabled = true;
    DiskManagerRadar::GetInstance().ReportDiscoverAutoMountSkipFault(ctx);

    ctx.volId = "vol-2";
    ctx.volDevPath = "/dev/block/vol-2";
    ctx.type = "exfat";
    DiskManagerRadar::GetInstance().ReportDiscoverAutoMountSkipFault(ctx);

    ctx.type.clear();
    ctx.uuid = "uuid-3";
    DiskManagerRadar::GetInstance().ReportDiscoverAutoMountSkipFault(ctx);
    SUCCEED();
}
} // namespace DiskManager
} // namespace OHOS
