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

#include "disk_manager_dfx_types.h"

namespace OHOS {
namespace DiskManager {
using namespace testing::ext;

class DiskManagerDfxTypesTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(DiskManagerDfxTypesTest, VolumeReportInfo_WithAndMerge_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-1").WithDiskId("disk-1").WithDevPath("/dev/block/sda1").WithFsType("exfat");
    info.fsUuid = "uuid-1";
    info.extra = "a=1";

    VolumeReportInfo other;
    other.WithVolumeId("vol-2").WithDiskId("disk-2").WithDevPath("/dev/block/sdb1").WithFsType("ntfs");
    other.fsUuid = "uuid-2";
    other.extra = "b=2";
    info.MergeFrom(other);
    EXPECT_EQ(info.volumeId, "vol-2");
    EXPECT_EQ(info.diskId, "disk-2");
    EXPECT_EQ(info.devPath, "/dev/block/sdb1");
    EXPECT_EQ(info.fsType, "ntfs");
    EXPECT_EQ(info.fsUuid, "uuid-2");
    EXPECT_EQ(info.extra, "a=1;b=2");

    VolumeReportInfo emptyExtra;
    emptyExtra.extra = "only";
    VolumeReportInfo base;
    base.MergeFrom(emptyExtra);
    EXPECT_EQ(base.extra, "only");
}

HWTEST_F(DiskManagerDfxTypesTest, VolumeReportInfo_MergeEmptyOther_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-1").WithDiskId("disk-1");
    info.fsUuid = "uuid-keep";
    info.extra = "keep-extra";
    VolumeReportInfo empty;
    info.MergeFrom(empty);
    EXPECT_EQ(info.volumeId, "vol-1");
    EXPECT_EQ(info.diskId, "disk-1");
    EXPECT_EQ(info.fsUuid, "uuid-keep");
    EXPECT_EQ(info.extra, "keep-extra");
}

HWTEST_F(DiskManagerDfxTypesTest, VolumeReportInfo_MergePartialOther_001, TestSize.Level0)
{
    VolumeReportInfo info;
    info.WithVolumeId("vol-1").WithDiskId("disk-1").WithDevPath("/dev/a").WithFsType("vfat");
    info.fsUuid = "uuid-1";
    info.extra = "a";

    VolumeReportInfo other;
    other.WithFsType("exfat");
    other.extra = "b";
    info.MergeFrom(other);
    EXPECT_EQ(info.volumeId, "vol-1");
    EXPECT_EQ(info.diskId, "disk-1");
    EXPECT_EQ(info.devPath, "/dev/a");
    EXPECT_EQ(info.fsType, "exfat");
    EXPECT_EQ(info.fsUuid, "uuid-1");
    EXPECT_EQ(info.extra, "a;b");
}

HWTEST_F(DiskManagerDfxTypesTest, VolumeReportInfo_ToExtraData_001, TestSize.Level0)
{
    VolumeReportInfo empty;
    EXPECT_TRUE(empty.ToExtraData().empty());

    VolumeReportInfo info;
    info.WithVolumeId("v1").WithDiskId("d1").WithDevPath("/dev/block/sda1").WithFsType("vfat");
    info.fsUuid = "u1";
    info.extra = "x";
    const std::string json = info.ToExtraData();
    EXPECT_NE(json.find("\"volumeId\":\"v1\""), std::string::npos);
    EXPECT_NE(json.find("\"diskId\":\"d1\""), std::string::npos);
    EXPECT_NE(json.find("\"devPath\":\"/dev/block/sda1\""), std::string::npos);
    EXPECT_NE(json.find("\"fsType\":\"vfat\""), std::string::npos);
    EXPECT_NE(json.find("\"fsUuid\":\"u1\""), std::string::npos);
    EXPECT_NE(json.find("\"extra\":\"x\""), std::string::npos);
}

HWTEST_F(DiskManagerDfxTypesTest, VolumeReportInfo_ToExtraData_PartialFields_001, TestSize.Level0)
{
    VolumeReportInfo onlyVol;
    onlyVol.WithVolumeId("v-only");
    EXPECT_NE(onlyVol.ToExtraData().find("\"volumeId\":\"v-only\""), std::string::npos);
    EXPECT_EQ(onlyVol.ToExtraData().find("diskId"), std::string::npos);

    VolumeReportInfo onlyDisk;
    onlyDisk.WithDiskId("disk-only");
    EXPECT_NE(onlyDisk.ToExtraData().find("\"diskId\":\"disk-only\""), std::string::npos);

    VolumeReportInfo onlyDev;
    onlyDev.WithDevPath("/dev/block/only");
    EXPECT_NE(onlyDev.ToExtraData().find("\"devPath\":\"/dev/block/only\""), std::string::npos);

    VolumeReportInfo onlyFs;
    onlyFs.WithFsType("exfat");
    EXPECT_NE(onlyFs.ToExtraData().find("\"fsType\":\"exfat\""), std::string::npos);

    VolumeReportInfo onlyExtra;
    onlyExtra.extra = "e";
    EXPECT_NE(onlyExtra.ToExtraData().find("\"extra\":\"e\""), std::string::npos);
    EXPECT_EQ(onlyExtra.ToExtraData().find("volumeId"), std::string::npos);

    VolumeReportInfo onlyUuid;
    onlyUuid.fsUuid = "uuid-only";
    EXPECT_NE(onlyUuid.ToExtraData().find("\"fsUuid\":\"uuid-only\""), std::string::npos);
}

HWTEST_F(DiskManagerDfxTypesTest, DfxTruncate_001, TestSize.Level0)
{
    EXPECT_EQ(DfxTruncate("abc", 10), "abc");
    EXPECT_EQ(DfxTruncate("abcdefghij", 5), "abcde");
    EXPECT_EQ(DfxTruncate("", 5), "");
    EXPECT_EQ(DfxTruncate("short"), "short");
    EXPECT_EQ(DfxTruncate(std::string(DFX_TRUNCATE_MAX_LEN, 'z')).size(), DFX_TRUNCATE_MAX_LEN);
    EXPECT_EQ(DfxTruncate(std::string(DFX_TRUNCATE_MAX_LEN + 10, 'z')).size(), DFX_TRUNCATE_MAX_LEN);
}

HWTEST_F(DiskManagerDfxTypesTest, ParseOpDiagText_JsonAndRaw_001, TestSize.Level0)
{
    VolumeReportInfo empty = ParseOpDiagText("");
    EXPECT_TRUE(empty.devPath.empty());

    VolumeReportInfo raw = ParseOpDiagText("not-json");
    EXPECT_EQ(raw.extra, "not-json");

    const std::string js = R"({"devPath":"/dev/sda1","fsType":"exfat","tools":[{"cmd":"fsck"}]})";
    VolumeReportInfo parsed = ParseOpDiagText(js);
    EXPECT_EQ(parsed.devPath, "/dev/sda1");
    EXPECT_EQ(parsed.fsType, "exfat");
    EXPECT_FALSE(parsed.extra.empty());
}

HWTEST_F(DiskManagerDfxTypesTest, BuildAutoMountSkipReportInfo_001, TestSize.Level0)
{
    AutoMountSkipContext ctx;
    ctx.volId = "vol-1";
    ctx.diskId = "disk-8-0";
    ctx.volDevPath = "/dev/block/vol-1";
    ctx.type = "exfat";
    ctx.uuid = "uuid-1";
    VolumeReportInfo info = BuildAutoMountSkipReportInfo(ctx);
    EXPECT_EQ(info.volumeId, "vol-1");
    EXPECT_EQ(info.fsType, "exfat");
    EXPECT_EQ(info.fsUuid, "uuid-1");
}

HWTEST_F(DiskManagerDfxTypesTest, ResolveAutoMountSkipReason_001, TestSize.Level0)
{
    EXPECT_EQ(ResolveAutoMountSkipReason(false, "exfat", "uuid"),
        AutoMountSkipReason::AUTO_MOUNT_DISABLED);
    EXPECT_EQ(ResolveAutoMountSkipReason(true, "", ""), AutoMountSkipReason::MISSING_FS_TYPE_AND_UUID);
    EXPECT_EQ(ResolveAutoMountSkipReason(true, "", "uuid"), AutoMountSkipReason::MISSING_FS_TYPE);
    EXPECT_EQ(ResolveAutoMountSkipReason(true, "exfat", ""), AutoMountSkipReason::MISSING_UUID);
    EXPECT_STREQ(AutoMountSkipReasonToString(AutoMountSkipReason::MISSING_FS_TYPE), "missingFsType");
}

HWTEST_F(DiskManagerDfxTypesTest, ParseOpDiagReport_ValidAndInvalid_001, TestSize.Level0)
{
    EXPECT_FALSE(ParseOpDiagReport("").valid);
    EXPECT_FALSE(ParseOpDiagReport("not-json").valid);
    EXPECT_FALSE(ParseOpDiagReport(R"({"devPath":"/dev/sda1"})").valid);
    EXPECT_FALSE(ParseOpDiagReport(R"({"ret":"bad","funcName":"x"})").valid);
    EXPECT_FALSE(ParseOpDiagReport(R"({"ret":1,"funcName":1})").valid);

    const std::string js =
        R"({"ret":-1,"funcName":"StorageDaemonProvider::Mount","bizStage":41,"opType":0,)"
        R"("devPath":"/dev/sda1","fsType":"exfat","tools":[{"cmd":"fsck"}]})";
    OpDiagReport report = ParseOpDiagReport(js);
    EXPECT_TRUE(report.valid);
    EXPECT_EQ(report.ret, -1);
    EXPECT_EQ(report.funcName, "StorageDaemonProvider::Mount");
    EXPECT_EQ(report.bizStage, 41);
    EXPECT_EQ(report.opType, VolumeOpType::MOUNT);
    EXPECT_EQ(report.info.devPath, "/dev/sda1");
    EXPECT_EQ(report.info.fsType, "exfat");
    EXPECT_FALSE(report.info.extra.empty());
}

HWTEST_F(DiskManagerDfxTypesTest, ParseOpDiagText_ExtraAndIds_001, TestSize.Level0)
{
    const std::string js =
        R"({"devPath":"/dev/a","fsType":"vfat","volumeId":"vol-1","diskId":"disk-1","extra":"e1"})";
    VolumeReportInfo info = ParseOpDiagText(js);
    EXPECT_EQ(info.volumeId, "vol-1");
    EXPECT_EQ(info.diskId, "disk-1");
    EXPECT_EQ(info.extra, "e1");

    VolumeReportInfo withTools = ParseOpDiagText(R"({"tools":[{"cmd":"sgdisk","exitCode":1}]})");
    EXPECT_NE(withTools.extra.find("sgdisk"), std::string::npos);

    OpDiagReport noStage = ParseOpDiagReport(R"({"ret":1,"funcName":"StorageDaemonProvider::Check"})");
    EXPECT_TRUE(noStage.valid);
    EXPECT_EQ(noStage.bizStage, 0);
}

} // namespace DiskManager
} // namespace OHOS
