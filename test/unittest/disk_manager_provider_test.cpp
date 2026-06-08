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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ipc/disk_manager_provider.h"
#include "disk_manager.h"
#include "disk_manager_errno.h"
#include "disk.h"
#include "volume_external.h"
#include "volume_core.h"
#include "partition_types.h"
#include "storage_spec_models.h"
#include "block_info_table.h"
#include "mock_storage_daemon_adapter.h"
#include "mock_usb_fuse_adapter.h"
#include "mock_uevent_bootstrap.h"
#include "disk_manager_hilog.h"
#include "system_ability_definition.h"
#include "errors.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

namespace {
const int64_t SIZE = 4096;

Disk MakeUsbDisk(const std::string &diskId = "disk-ut-1")
{
    return Disk(diskId, SIZE, "/dev/block/" + diskId, USB_FLAG);
}

VolumeExternal MakeUsbVolume(const std::string &volId = "vol-ut-1",
                             const std::string &diskId = "disk-ut-1",
                             const std::string &fsUuid = "uuid-ut-1",
                             int32_t state = UNMOUNTED,
                             int32_t fsType = VFAT)
{
    VolumeCore core(volId, EXTERNAL, diskId, state);
    VolumeExternal v(core);
    v.SetFlags(USB_FLAG);
    v.SetFsUuid(fsUuid);
    v.SetFsType(fsType);
    v.SetPath("/mnt/data/external/" + fsUuid);
    return v;
}

void ClearDiskManagerState()
{
    auto &dm = DiskManager::GetInstance();
    std::vector<Disk> disks;
    dm.GetAllDisks(disks);
    for (auto &d : disks) {
        dm.OnDiskDestroyed(d.GetDiskId());
    }
    std::vector<VolumeExternal> volumes;
    dm.GetAllVolumes(volumes);
    for (auto &v : volumes) {
        dm.OnVolumeDestroyed(v.GetId());
    }
}

} // namespace

class DiskManagerProviderTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GTEST_LOG_(INFO) << "DiskManagerProviderTest SetUpTestCase";
        testing::Mock::AllowLeak(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::AllowLeak(&MockUsbFuseAdapter::GetInstance());
        testing::Mock::AllowLeak(&MockUeventBootstrap::GetInstance());
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "DiskManagerProviderTest TearDownTestCase";
    }

    void SetUp() override
    {
        ClearDiskManagerState();
        GTEST_LOG_(INFO) << "DiskManagerProviderTest SetUp";
    }

    void TearDown() override
    {
        ClearDiskManagerState();
        GTEST_LOG_(INFO) << "DiskManagerProviderTest TearDown";
    }
};

/**
 * @tc.name: Constructor_TestCase_001
 * @tc.desc: DiskManagerProvider can be constructed without crash.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Constructor_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Constructor_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    GTEST_LOG_(INFO) << "Constructor_TestCase_001 End";
}

/**
 * @tc.name: OnStop_TestCase_001
 * @tc.desc: OnStop completes without crash (no-op).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, OnStop_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnStop_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    provider.OnStop();
    GTEST_LOG_(INFO) << "OnStop_TestCase_001 End";
}

/**
 * @tc.name: GetPartitionTable_TestCase_001
 * @tc.desc: GetPartitionTable with empty diskId returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetPartitionTable_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionTableInfo out;
    EXPECT_EQ(provider.GetPartitionTable("", out), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_001 End";
}

/**
 * @tc.name: CreatePartition_TestCase_001
 * @tc.desc: CreatePartition with empty diskId returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_001 End";
}

/**
 * @tc.name: CreatePartition_TestCase_002
 * @tc.desc: CreatePartition with partitionNum <= 0 returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(0, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-1", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_002 End";
}

/**
 * @tc.name: CreatePartition_TestCase_003
 * @tc.desc: CreatePartition with negative partitionNum returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_003 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(-1, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-1", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_003 End";
}

/**
 * @tc.name: CreatePartition_TestCase_004
 * @tc.desc: CreatePartition with startSector <= 0 returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_004 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 0, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-1", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_004 End";
}

/**
 * @tc.name: CreatePartition_TestCase_005
 * @tc.desc: CreatePartition with endSector <= 0 returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_005 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 2048, 0, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-1", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_005 End";
}

/**
 * @tc.name: CreatePartition_TestCase_006
 * @tc.desc: CreatePartition with startSector >= endSector returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_006 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 500000, 2048, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-1", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_006 End";
}

/**
 * @tc.name: CreatePartition_TestCase_007
 * @tc.desc: CreatePartition with startSector == endSector returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_007 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 2048, 2048, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-1", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_007 End";
}

/**
 * @tc.name: CreatePartition_TestCase_008
 * @tc.desc: CreatePartition with empty typeCode returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_008, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_008 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 2048, 500000, "");
    EXPECT_EQ(provider.CreatePartition("disk-1", params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_008 End";
}

/**
 * @tc.name: DeletePartition_TestCase_001
 * @tc.desc: DeletePartition with empty diskId returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, DeletePartition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.DeletePartition("", 1), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_001 End";
}

/**
 * @tc.name: DeletePartition_TestCase_002
 * @tc.desc: DeletePartition with partitionNum <= 0 returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, DeletePartition_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.DeletePartition("disk-1", 0), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_002 End";
}

/**
 * @tc.name: DeletePartition_TestCase_003
 * @tc.desc: DeletePartition with negative partitionNum returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, DeletePartition_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_003 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.DeletePartition("disk-1", -1), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_003 End";
}

/**
 * @tc.name: FormatPartition_TestCase_001
 * @tc.desc: FormatPartition with empty diskId returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(provider.FormatPartition("", 1, params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_001 End";
}

/**
 * @tc.name: FormatPartition_TestCase_002
 * @tc.desc: FormatPartition with partitionNum <= 0 returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(provider.FormatPartition("disk-1", 0, params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_002 End";
}

/**
 * @tc.name: FormatPartition_TestCase_003
 * @tc.desc: FormatPartition with negative partitionNum returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_003 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(provider.FormatPartition("disk-1", -1, params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_003 End";
}

/**
 * @tc.name: FormatPartition_TestCase_004
 * @tc.desc: FormatPartition with empty fsType returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_004 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FormatParams params("", true, "volume");
    EXPECT_EQ(provider.FormatPartition("disk-1", 1, params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_004 End";
}

/**
 * @tc.name: IsUsbFuseByType_TestCase_001
 * @tc.desc: IsUsbFuseByType with known type (NTFS) maps via FS_TYPE_MAP.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, IsUsbFuseByType_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isUsbFuse = false;
    EXPECT_CALL(MockUsbFuseAdapter::GetInstance(), IsUsbFuseEnabledForFsType("ntfs"))
        .WillOnce(Return(true));
    EXPECT_EQ(provider.IsUsbFuseByType(NTFS, isUsbFuse), E_OK);
    EXPECT_TRUE(isUsbFuse);
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_001 End";
}

/**
 * @tc.name: IsUsbFuseByType_TestCase_002
 * @tc.desc: IsUsbFuseByType with unknown type falls back to "undefined".
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, IsUsbFuseByType_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isUsbFuse = true;
    EXPECT_CALL(MockUsbFuseAdapter::GetInstance(), IsUsbFuseEnabledForFsType("undefined"))
        .WillOnce(Return(false));
    EXPECT_EQ(provider.IsUsbFuseByType(-999, isUsbFuse), E_OK);
    EXPECT_FALSE(isUsbFuse);
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_002 End";
}

/**
 * @tc.name: IsUsbFuseByType_TestCase_003
 * @tc.desc: IsUsbFuseByType with EXFAT type delegates correctly.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, IsUsbFuseByType_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_003 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isUsbFuse = false;
    EXPECT_CALL(MockUsbFuseAdapter::GetInstance(), IsUsbFuseEnabledForFsType("exfat"))
        .WillOnce(Return(false));
    EXPECT_EQ(provider.IsUsbFuseByType(EXFAT, isUsbFuse), E_OK);
    EXPECT_FALSE(isUsbFuse);
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_003 End";
}

/**
 * @tc.name: IsUsbFuseByType_TestCase_004
 * @tc.desc: IsUsbFuseByType always returns E_OK regardless of adapter result.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, IsUsbFuseByType_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_004 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isUsbFuse = false;
    EXPECT_CALL(MockUsbFuseAdapter::GetInstance(), IsUsbFuseEnabledForFsType("vfat"))
        .WillOnce(Return(true));
    EXPECT_EQ(provider.IsUsbFuseByType(VFAT, isUsbFuse), E_OK);
    EXPECT_TRUE(isUsbFuse);
    GTEST_LOG_(INFO) << "IsUsbFuseByType_TestCase_004 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_001
 * @tc.desc: QueryUsbIsInUse delegates to StorageDaemonAdapter and returns result.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isInUse = false;
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(true), Return(ERR_OK)));
    int32_t ret = provider.QueryUsbIsInUse("/dev/block/disk-1", isInUse);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_TRUE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_001 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_002
 * @tc.desc: QueryUsbIsInUse initializes isInUse=false before delegating.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isInUse = true;
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(Return(E_DAEMON_IPC_FAILED));
    int32_t ret = provider.QueryUsbIsInUse("/dev/block/disk-2", isInUse);
    EXPECT_NE(ret, E_OK);
    EXPECT_FALSE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_002 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_003
 * @tc.desc: QueryUsbIsInUse returns adapter error code when IPC fails.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_003 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isInUse = false;
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(false), Return(ERR_OK)));
    int32_t ret = provider.QueryUsbIsInUse("/dev/block/disk-3", isInUse);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_FALSE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_003 End";
}

/**
 * @tc.name: OnBlockDiskUevent_TestCase_001
 * @tc.desc: OnBlockDiskUevent delegates to UeventBootstrap and returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, OnBlockDiskUevent_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnBlockDiskUevent_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_CALL(MockUeventBootstrap::GetInstance(), OnBlockDiskUeventImpl(_))
        .WillOnce(Return(E_OK));
    EXPECT_EQ(provider.OnBlockDiskUevent("ACTION=add;DEVNAME=sda;MAJOR=8;MINOR=0"), E_OK);
    GTEST_LOG_(INFO) << "OnBlockDiskUevent_TestCase_001 End";
}

/**
 * @tc.name: OnBlockDiskUevent_TestCase_002
 * @tc.desc: OnBlockDiskUevent propagates UeventBootstrap parse error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, OnBlockDiskUevent_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnBlockDiskUevent_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_CALL(MockUeventBootstrap::GetInstance(), OnBlockDiskUeventImpl(_))
        .WillOnce(Return(E_UEVENT_PARSE_FAILED));
    EXPECT_EQ(provider.OnBlockDiskUevent("invalid-msg"), E_UEVENT_PARSE_FAILED);
    GTEST_LOG_(INFO) << "OnBlockDiskUevent_TestCase_002 End";
}

/**
 * @tc.name: NotifyMtpMounted_TestCase_001
 * @tc.desc: NotifyMtpMounted always returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpMounted_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.NotifyMtpMounted("mtp-id", "/mnt/path", "desc", "uuid", "mtpfs"), E_OK);
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_001 End";
}

/**
 * @tc.name: NotifyMtpUnmounted_TestCase_001
 * @tc.desc: NotifyMtpUnmounted with isBadRemove=false returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpUnmounted_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.NotifyMtpUnmounted("mtp-id", false), E_OK);
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_001 End";
}

/**
 * @tc.name: NotifyMtpUnmounted_TestCase_002
 * @tc.desc: NotifyMtpUnmounted with isBadRemove=true returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpUnmounted_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.NotifyMtpUnmounted("mtp-id", true), E_OK);
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_002 End";
}

/**
 * @tc.name: Mount_TestCase_001
 * @tc.desc: Provider.Mount delegates to DiskManager::GetInstance().Mount.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Mount_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-p1"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-p1", "disk-mt-p1", "uuid-mt-p1", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), Mount(_, _, _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(MockUsbFuseAdapter::GetInstance(), IsUsbFuseEnabledForFsType(_))
        .WillRepeatedly(Return(false));
    EXPECT_EQ(provider.Mount("vol-mt-p1"), E_OK);
    GTEST_LOG_(INFO) << "Mount_TestCase_001 End";
}

/**
 * @tc.name: Unmount_TestCase_001
 * @tc.desc: Provider.Unmount delegates to DiskManager::GetInstance().Unmount.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Unmount_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-um-p1"));
    VolumeExternal vol = MakeUsbVolume("vol-um-p1", "disk-um-p1", "uuid-um-p1", MOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), Unmount(_, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_EQ(provider.Unmount("vol-um-p1"), E_OK);
    GTEST_LOG_(INFO) << "Unmount_TestCase_001 End";
}

/**
 * @tc.name: GetAllVolumes_TestCase_001
 * @tc.desc: Provider.GetAllVolumes delegates to DiskManager and returns empty list.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetAllVolumes_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    std::vector<VolumeExternal> volumes;
    EXPECT_EQ(provider.GetAllVolumes(volumes), E_OK);
    EXPECT_EQ(volumes.size(), static_cast<size_t>(0));
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_001 End";
}

/**
 * @tc.name: GetAllDisks_TestCase_001
 * @tc.desc: Provider.GetAllDisks delegates to DiskManager and returns empty list.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetAllDisks_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    std::vector<Disk> disks;
    EXPECT_EQ(provider.GetAllDisks(disks), E_OK);
    EXPECT_EQ(disks.size(), static_cast<size_t>(0));
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_001 End";
}

/**
 * @tc.name: GetAllDisks_TestCase_002
 * @tc.desc: Provider.GetAllDisks returns populated disk list from DiskManager.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetAllDisks_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ad-p1"));
    dm.OnDiskCreated(MakeUsbDisk("disk-ad-p2"));
    std::vector<Disk> disks;
    EXPECT_EQ(provider.GetAllDisks(disks), E_OK);
    EXPECT_EQ(disks.size(), static_cast<size_t>(2));
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_002 End";
}

/**
 * @tc.name: EraseVolume_TestCase_001
 * @tc.desc: Provider.EraseVolume delegates to DiskManager and returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, EraseVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "EraseVolume_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.EraseVolume("any-vol"), E_OK);
    GTEST_LOG_(INFO) << "EraseVolume_TestCase_001 End";
}

/**
 * @tc.name: EjectVolume_TestCase_001
 * @tc.desc: Provider.EjectVolume delegates to DiskManager and returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, EjectVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "EjectVolume_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.EjectVolume("any-vol"), E_OK);
    GTEST_LOG_(INFO) << "EjectVolume_TestCase_001 End";
}

/**
 * @tc.name: GetVolumeOpProcess_TestCase_001
 * @tc.desc: Provider.GetVolumeOpProcess delegates to DiskManager and returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeOpProcess_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeOpProcess_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    int32_t progress = -1;
    EXPECT_EQ(provider.GetVolumeOpProcess("any-vol", progress), E_OK);
    EXPECT_EQ(progress, 0);
    GTEST_LOG_(INFO) << "GetVolumeOpProcess_TestCase_001 End";
}

/**
 * @tc.name: VerifyBurnData_TestCase_001
 * @tc.desc: Provider.VerifyBurnData delegates to DiskManager and returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, VerifyBurnData_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "VerifyBurnData_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.VerifyBurnData("any-vol", 0), E_OK);
    GTEST_LOG_(INFO) << "VerifyBurnData_TestCase_001 End";
}

} // namespace DiskManager
} // namespace OHOS

namespace OHOS {
namespace DiskManager {

BlockInfoTable &BlockInfoTable::GetInstance()
{
    static BlockInfoTable tableInstance;
    return tableInstance;
}

int32_t BlockInfoTable::ReloadFromDaemon()
{
    return E_OK;
}

} // namespace DiskManager
} // namespace OHOS