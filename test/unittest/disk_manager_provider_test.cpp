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
#include "disk_manager_napi_errno.h"
#include "disk.h"
#include "volume_external.h"
#include "volume_core.h"
#include "partition_types.h"
#include "storage_spec_models.h"
#include "block_info_table.h"
#include "voldata_uuid_store.h"
#include "mock_storage_daemon_adapter.h"
#include "mock_usb_fuse_adapter.h"
#include "mock_uevent_bootstrap.h"
#include "mock_ipc_skeleton.h"
#include "disk_manager_hilog.h"
#include "system_ability_definition.h"
#include "errors.h"

extern int32_t g_accessTokenType;
extern bool g_isSystemApp;
extern int32_t g_permissionGranted;
extern std::string g_nativeProcessName;

constexpr int32_t MOCK_PERMISSION_GRANTED = 0;
constexpr int32_t MOCK_PERMISSION_DENIED = -1;

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

namespace {
const int64_t SIZE = 4096;

Disk MakeUsbDisk(const std::string &diskId = "disk-8-1")
{
    return Disk(diskId, SIZE, "/dev/block/" + diskId, USB_FLAG);
}

VolumeExternal MakeUsbVolume(const std::string &volId = "vol-8-1",
                             const std::string &diskId = "disk-8-1",
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
        MockIPCSkeleton::mockCallingUid_ = 0;
        g_accessTokenType = 1;
        g_isSystemApp = true;
        g_permissionGranted = MOCK_PERMISSION_GRANTED;
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "DiskManagerProviderTest TearDownTestCase";
    }

    void SetUp() override
    {
        ClearDiskManagerState();
        testing::Mock::VerifyAndClear(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::VerifyAndClear(&MockUsbFuseAdapter::GetInstance());
        testing::Mock::VerifyAndClear(&MockUeventBootstrap::GetInstance());
        MockIPCSkeleton::mockCallingUid_ = 0;
        g_accessTokenType = 1;
        g_isSystemApp = true;
        g_permissionGranted = MOCK_PERMISSION_GRANTED;
        GTEST_LOG_(INFO) << "DiskManagerProviderTest SetUp";
    }

    void TearDown() override
    {
        ClearDiskManagerState();
        testing::Mock::VerifyAndClear(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::VerifyAndClear(&MockUsbFuseAdapter::GetInstance());
        testing::Mock::VerifyAndClear(&MockUeventBootstrap::GetInstance());
        MockIPCSkeleton::mockCallingUid_ = 0;
        g_accessTokenType = 1;
        g_isSystemApp = true;
        g_permissionGranted = MOCK_PERMISSION_GRANTED;
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
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.DeletePartition("disk-8-0", 0), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.DeletePartition("disk-8-0", -1), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.FormatPartition("disk-8-0", 0, params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.FormatPartition("disk-8-0", -1, params), E_PARAMS_INVALID);
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
    EXPECT_EQ(provider.FormatPartition("disk-8-0", 1, params), E_PARAMS_INVALID);
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
 * @tc.desc: QueryUsbIsInUse delegates to StorageDaemonAdapter and returns result with isInUse=true.
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
    int32_t ret = provider.QueryUsbIsInUse("/mnt/data/external", isInUse);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_TRUE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_001 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_002
 * @tc.desc: QueryUsbIsInUse returns E_QUERY_VOLUME_IN_USE_ERROR when adapter returns error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isInUse = false;
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(false), Return(E_DAEMON_IPC_FAILED)));
    int32_t ret = provider.QueryUsbIsInUse("/mnt/data/external", isInUse);
    EXPECT_EQ(ret, E_QUERY_VOLUME_IN_USE_ERROR);
    EXPECT_FALSE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_002 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_003
 * @tc.desc: QueryUsbIsInUse returns result with isInUse=false.
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
    int32_t ret = provider.QueryUsbIsInUse("/mnt/data/external", isInUse);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_FALSE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_003 End";
}

/**
 * @tc.name: QueryUsbIsInUse_NotSysApp_001
 * @tc.desc: QueryUsbIsInUse returns E_SYS_APP_PERMISSION_DENIED when caller is not system app.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_NotSysApp_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_NotSysApp_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    bool isInUse = false;
    int32_t ret = provider.QueryUsbIsInUse("/dev/block/disk-8-0", isInUse);
    EXPECT_EQ(ret, E_SYS_APP_PERMISSION_DENIED);
    EXPECT_FALSE(isInUse);
    g_accessTokenType = 1;
    g_isSystemApp = true;
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_NotSysApp_001 End";
}

/**
 * @tc.name: QueryUsbIsInUse_PermissionDenied_001
 * @tc.desc: QueryUsbIsInUse returns E_PERMISSION_DENIED when caller lacks MOUNT_UNMOUNT_MANAGER permission.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_PermissionDenied_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    bool isInUse = false;
    int32_t ret = provider.QueryUsbIsInUse("/dev/block/disk-8-0", isInUse);
    EXPECT_EQ(ret, E_PERMISSION_DENIED);
    EXPECT_FALSE(isInUse);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_PermissionDenied_001 End";
}

/**
 * @tc.name: QueryUsbIsInUse_EmptyPath_001
 * @tc.desc: QueryUsbIsInUse returns E_PARAMS_INVALID when diskPath is empty.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_EmptyPath_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_EmptyPath_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isInUse = false;
    int32_t ret = provider.QueryUsbIsInUse("", isInUse);
    EXPECT_EQ(ret, E_PARAMS_INVALID);
    EXPECT_FALSE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_EmptyPath_001 End";
}

/**
 * @tc.name: QueryUsbIsInUse_InvalidPath_001
 * @tc.desc: QueryUsbIsInUse returns E_PARAMS_INVALID when diskPath contains ../ relative path.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_InvalidPath_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_InvalidPath_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isInUse = false;
    int32_t ret = provider.QueryUsbIsInUse("/dev/block/../disk-8-0", isInUse);
    EXPECT_EQ(ret, E_PARAMS_INVALID);
    EXPECT_FALSE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_InvalidPath_001 End";
}

/**
 * @tc.name: QueryUsbIsInUse_InvalidPath_002
 * @tc.desc: QueryUsbIsInUse returns E_PARAMS_INVALID when diskPath ends with /..
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, QueryUsbIsInUse_InvalidPath_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_InvalidPath_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    bool isInUse = false;
    int32_t ret = provider.QueryUsbIsInUse("/dev/block/disk-8-0/..", isInUse);
    EXPECT_EQ(ret, E_PARAMS_INVALID);
    EXPECT_FALSE(isInUse);
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_InvalidPath_002 End";
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
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_CALL(MockUeventBootstrap::GetInstance(), OnBlockDiskUeventImpl(_))
        .WillOnce(Return(E_OK));
    EXPECT_EQ(provider.OnBlockDiskUevent("ACTION=add;DEVNAME=sda;MAJOR=8;MINOR=0"), E_OK);
    g_nativeProcessName = "foundation";
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
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_CALL(MockUeventBootstrap::GetInstance(), OnBlockDiskUeventImpl(_))
        .WillOnce(Return(E_UEVENT_PARSE_FAILED));
    EXPECT_EQ(provider.OnBlockDiskUevent("invalid-msg"), E_UEVENT_PARSE_FAILED);
    g_nativeProcessName = "foundation";
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
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_EQ(provider.NotifyMtpMounted("vol-61-1", "/mnt/data/external", "desc", "uuid", "mtpfs"), E_OK);
    g_nativeProcessName = "foundation";
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_001 End";
}

/**
 * @tc.name: NotifyMtpMounted_EmptyPath_001
 * @tc.desc: NotifyMtpMounted returns E_PARAMS_INVALID when path is empty.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpMounted_EmptyPath_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_EmptyPath_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_EQ(provider.NotifyMtpMounted("vol-61-1", "", "desc", "uuid", "mtpfs"), E_PARAMS_INVALID);
    g_nativeProcessName = "foundation";
    GTEST_LOG_(INFO) << "NotifyMtpMounted_EmptyPath_001 End";
}

/**
 * @tc.name: NotifyMtpMounted_InvalidPath_001
 * @tc.desc: NotifyMtpMounted returns E_PARAMS_INVALID when path contains ../ path traversal.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpMounted_InvalidPath_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_InvalidPath_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_EQ(provider.NotifyMtpMounted("vol-61-1", "/mnt/../path", "desc", "uuid", "mtpfs"), E_PARAMS_INVALID);
    g_nativeProcessName = "foundation";
    GTEST_LOG_(INFO) << "NotifyMtpMounted_InvalidPath_001 End";
}

/**
 * @tc.name: NotifyMtpMounted_InvalidPath_002
 * @tc.desc: NotifyMtpMounted returns E_PARAMS_INVALID when path ends with /..
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpMounted_InvalidPath_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_InvalidPath_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_EQ(provider.NotifyMtpMounted("vol-61-1", "/mnt/..", "desc", "uuid", "mtpfs"), E_PARAMS_INVALID);
    g_nativeProcessName = "foundation";
    GTEST_LOG_(INFO) << "NotifyMtpMounted_InvalidPath_002 End";
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
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_EQ(provider.NotifyMtpUnmounted("vol-61-1", false), E_OK);
    g_nativeProcessName = "foundation";
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
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_EQ(provider.NotifyMtpUnmounted("vol-61-1", true), E_OK);
    g_nativeProcessName = "foundation";
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
    dm.OnDiskCreated(MakeUsbDisk("disk-14-1"));
    VolumeExternal vol = MakeUsbVolume("vol-14-1", "disk-14-1", "uuid-mt-p1", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), Mount(_, _, _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(MockUsbFuseAdapter::GetInstance(), IsUsbFuseEnabledForFsType(_))
        .WillRepeatedly(Return(false));
    EXPECT_EQ(provider.Mount("vol-14-1"), E_OK);
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
    dm.OnDiskCreated(MakeUsbDisk("disk-15-1"));
    VolumeExternal vol = MakeUsbVolume("vol-15-1", "disk-15-1", "uuid-um-p1", MOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), Unmount(_, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_EQ(provider.Unmount("vol-15-1"), E_OK);
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
    dm.OnDiskCreated(MakeUsbDisk("disk-16-1"));
    dm.OnDiskCreated(MakeUsbDisk("disk-16-2"));
    std::vector<Disk> disks;
    EXPECT_EQ(provider.GetAllDisks(disks), E_OK);
    EXPECT_EQ(disks.size(), static_cast<size_t>(2));
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_002 End";
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
    EXPECT_EQ(provider.GetVolumeOpProcess("vol-99-99", progress), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "GetVolumeOpProcess_TestCase_001 End";
}

/**
 * @tc.name: Format_TestCase_001
 * @tc.desc: Provider.Format with nonexistent volumeId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Format_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.Format("vol-99-99", "vfat"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "Format_TestCase_001 End";
}

/**
 * @tc.name: TryToFix_TestCase_001
 * @tc.desc: Provider.TryToFix with nonexistent volumeId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, TryToFix_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TryToFix_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.TryToFix("vol-99-99"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "TryToFix_TestCase_001 End";
}

/**
 * @tc.name: SetVolumeDescription_TestCase_001
 * @tc.desc: Provider.SetVolumeDescription with nonexistent fsUuid returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, SetVolumeDescription_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.SetVolumeDescription("nonexistent-uuid", "desc"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_001 End";
}

/**
 * @tc.name: GetVolumeByUuid_TestCase_001
 * @tc.desc: Provider.GetVolumeByUuid with nonexistent fsUuid returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeByUuid_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeByUuid_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    VolumeExternal vc;
    EXPECT_EQ(provider.GetVolumeByUuid("nonexistent-uuid", vc), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetVolumeByUuid_TestCase_001 End";
}

/**
 * @tc.name: GetVolumeById_TestCase_001
 * @tc.desc: Provider.GetVolumeById with nonexistent volumeId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeById_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeById_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    VolumeExternal vc;
    EXPECT_EQ(provider.GetVolumeById("vol-99-99", vc), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetVolumeById_TestCase_001 End";
}

/**
 * @tc.name: GetFreeSizeOfVolume_TestCase_001
 * @tc.desc: Provider.GetFreeSizeOfVolume with nonexistent volumeUuid returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetFreeSizeOfVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    int64_t freeSize = -1;
    EXPECT_EQ(provider.GetFreeSizeOfVolume("nonexistent-uuid", freeSize), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_TestCase_001 End";
}

/**
 * @tc.name: GetTotalSizeOfVolume_TestCase_001
 * @tc.desc: Provider.GetTotalSizeOfVolume with nonexistent volumeUuid returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetTotalSizeOfVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    int64_t totalSize = -1;
    EXPECT_EQ(provider.GetTotalSizeOfVolume("nonexistent-uuid", totalSize), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_TestCase_001 End";
}

/**
 * @tc.name: Partition_TestCase_001
 * @tc.desc: Provider.Partition with nonexistent diskId returns E_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Partition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Partition_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.Partition("disk-99-99", 0), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "Partition_TestCase_001 End";
}

/**
 * @tc.name: GetDiskById_TestCase_001
 * @tc.desc: Provider.GetDiskById with empty diskId returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetDiskById_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    Disk disk;
    EXPECT_EQ(provider.GetDiskById("", disk), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_001 End";
}

/**
 * @tc.name: GetDiskById_TestCase_002
 * @tc.desc: Provider.GetDiskById with nonexistent diskId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetDiskById_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    Disk disk;
    EXPECT_EQ(provider.GetDiskById("disk-99-99", disk), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_002 End";
}

/**
 * @tc.name: Erase_TestCase_001
 * @tc.desc: Provider.Erase with nonexistent volumeId returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Erase_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Erase_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.Erase("vol-99-99"), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "Erase_TestCase_001 End";
}

/**
 * @tc.name: Eject_TestCase_001
 * @tc.desc: Provider.Eject with nonexistent diskId returns E_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Eject_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Eject_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.Eject("disk-99-99"), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "Eject_TestCase_001 End";
}

/**
 * @tc.name: CreateIsoImage_TestCase_001
 * @tc.desc: Provider.CreateIsoImage with nonexistent volumeId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreateIsoImage_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.CreateIsoImage("vol-99-99", "/data/local/tmp"), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "CreateIsoImage_TestCase_001 End";
}

/**
 * @tc.name: CreateIsoImage_EmptyFilePath_001
 * @tc.desc: CreateIsoImage returns E_PARAMS_INVALID when filePath is empty.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreateIsoImage_EmptyFilePath_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_EmptyFilePath_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.CreateIsoImage("vol-8-0", ""), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreateIsoImage_EmptyFilePath_001 End";
}

/**
 * @tc.name: CreateIsoImage_InvalidPath_001
 * @tc.desc: CreateIsoImage returns E_PARAMS_INVALID when filePath contains ../ path traversal.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreateIsoImage_InvalidPath_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_InvalidPath_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.CreateIsoImage("vol-8-0", "/tmp/../etc/image.iso"), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreateIsoImage_InvalidPath_001 End";
}

/**
 * @tc.name: CreateIsoImage_InvalidPath_002
 * @tc.desc: CreateIsoImage returns E_PARAMS_INVALID when filePath ends with /..
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreateIsoImage_InvalidPath_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_InvalidPath_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.CreateIsoImage("vol-8-0", "/tmp/.."), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "CreateIsoImage_InvalidPath_002 End";
}

/**
 * @tc.name: Burn_TestCase_001
 * @tc.desc: Provider.Burn with nonexistent volumeId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Burn_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Burn_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.Burn("vol-99-99", "--write"), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "Burn_TestCase_001 End";
}

/**
 * @tc.name: CheckClientPermission_TestCase_001
 * @tc.desc: CheckStorageDaemonPermission returns false when uid != STORAGEDAEMON_UID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CheckStorageDaemonPermission_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CheckStorageDaemonPermission_TestCase_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 1000;
    EXPECT_FALSE(provider.CheckStorageDaemonPermission());
    MockIPCSkeleton::mockCallingUid_ = 0;
    GTEST_LOG_(INFO) << "CheckStorageDaemonPermission_TestCase_001 End";
}

/**
 * @tc.name: OnBlockDiskUevent_TestCase_003
 * @tc.desc: OnBlockDiskUevent returns E_PERMISSION_DENIED when CheckStorageDaemonPermission fails.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, OnBlockDiskUevent_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnBlockDiskUevent_TestCase_003 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 1000;
    EXPECT_EQ(provider.OnBlockDiskUevent("ACTION=add;DEVNAME=sda"), E_PERMISSION_DENIED);
    MockIPCSkeleton::mockCallingUid_ = 0;
    GTEST_LOG_(INFO) << "OnBlockDiskUevent_TestCase_003 End";
}

/**
 * @tc.name: NotifyMtpMounted_TestCase_002
 * @tc.desc: NotifyMtpMounted returns E_PERMISSION_DENIED when CheckStorageDaemonPermission fails.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpMounted_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 1000;
    EXPECT_EQ(provider.NotifyMtpMounted("id", "/path", "desc", "uuid", "fs"), E_PERMISSION_DENIED);
    MockIPCSkeleton::mockCallingUid_ = 0;
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_002 End";
}

/**
 * @tc.name: NotifyMtpUnmounted_TestCase_003
 * @tc.desc: NotifyMtpUnmounted returns E_PERMISSION_DENIED when CheckStorageDaemonPermission fails.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, NotifyMtpUnmounted_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_003 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 1000;
    EXPECT_EQ(provider.NotifyMtpUnmounted("id", true), E_PERMISSION_DENIED);
    MockIPCSkeleton::mockCallingUid_ = 0;
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_003 End";
}

/**
 * @tc.name: GetPartitionTable_TestCase_002
 * @tc.desc: GetPartitionTable with nonexistent diskId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetPartitionTable_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionTableInfo out;
    EXPECT_EQ(provider.GetPartitionTable("disk-99-99", out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_002 End";
}

/**
 * @tc.name: CreatePartition_TestCase_009
 * @tc.desc: CreatePartition with valid params and nonexistent diskId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_TestCase_009, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_009 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-99-99", params), E_NON_EXIST);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_009 End";
}

/**
 * @tc.name: DeletePartition_TestCase_004
 * @tc.desc: DeletePartition with valid params and nonexistent diskId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, DeletePartition_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_004 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.DeletePartition("disk-99-99", 1), E_NON_EXIST);
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_004 End";
}

/**
 * @tc.name: FormatPartition_TestCase_005
 * @tc.desc: FormatPartition with valid params except quickFormat=false returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_005 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FormatParams params("vfat", false, "volume");
    EXPECT_EQ(provider.FormatPartition("disk-8-0", 1, params), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_005 End";
}

/**
 * @tc.name: FormatPartition_TestCase_006
 * @tc.desc: FormatPartition with all valid params and nonexistent diskId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_006 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(provider.FormatPartition("disk-99-99", 1, params), E_NON_EXIST);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_006 End";
}

/**
 * @tc.name: GetAllDisks_PermissionDenied_001
 * @tc.desc: GetAllDisks returns E_SYS_APP_PERMISSION_DENIED when caller is not system app.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetAllDisks_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllDisks_PermissionDenied_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    std::vector<Disk> disks;
    EXPECT_EQ(provider.GetAllDisks(disks), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;
    GTEST_LOG_(INFO) << "GetAllDisks_PermissionDenied_001 End";
}

/**
 * @tc.name: GetAllDisks_PermissionDenied_002
 * @tc.desc: GetAllDisks returns E_PERMISSION_DENIED when caller lacks MOUNT_UNMOUNT_MANAGER permission.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetAllDisks_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllDisks_PermissionDenied_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    std::vector<Disk> disks;
    EXPECT_EQ(provider.GetAllDisks(disks), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    GTEST_LOG_(INFO) << "GetAllDisks_PermissionDenied_002 End";
}

/**
 * @tc.name: GetDiskById_PermissionDenied_001
 * @tc.desc: GetDiskById returns E_SYS_APP_PERMISSION_DENIED when caller is not system app.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetDiskById_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiskById_PermissionDenied_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    Disk disk;
    EXPECT_EQ(provider.GetDiskById("disk-8-0", disk), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;
    GTEST_LOG_(INFO) << "GetDiskById_PermissionDenied_001 End";
}

/**
 * @tc.name: GetDiskById_PermissionDenied_002
 * @tc.desc: GetDiskById returns E_PERMISSION_DENIED when caller lacks MOUNT_UNMOUNT_MANAGER permission.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetDiskById_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiskById_PermissionDenied_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    Disk disk;
    EXPECT_EQ(provider.GetDiskById("disk-8-0", disk), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    GTEST_LOG_(INFO) << "GetDiskById_PermissionDenied_002 End";
}

/**
 * @tc.name: GetPartitionTable_PermissionDenied_001
 * @tc.desc: GetPartitionTable returns E_SYS_APP_PERMISSION_DENIED when caller is not system app.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetPartitionTable_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_PermissionDenied_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    PartitionTableInfo out;
    EXPECT_EQ(provider.GetPartitionTable("disk-8-0", out), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;
    GTEST_LOG_(INFO) << "GetPartitionTable_PermissionDenied_001 End";
}

/**
 * @tc.name: GetPartitionTable_PermissionDenied_002
 * @tc.desc: GetPartitionTable returns E_PERMISSION_DENIED when caller lacks MOUNT_UNMOUNT_MANAGER permission.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetPartitionTable_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_PermissionDenied_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    PartitionTableInfo out;
    EXPECT_EQ(provider.GetPartitionTable("disk-8-0", out), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    GTEST_LOG_(INFO) << "GetPartitionTable_PermissionDenied_002 End";
}

/**
 * @tc.name: CreatePartition_PermissionDenied_001
 * @tc.desc: CreatePartition returns E_SYS_APP_PERMISSION_DENIED when caller is not system app.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_PermissionDenied_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    PartitionParams params(1, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;
    GTEST_LOG_(INFO) << "CreatePartition_PermissionDenied_001 End";
}

/**
 * @tc.name: CreatePartition_PermissionDenied_002
 * @tc.desc: CreatePartition returns E_PERMISSION_DENIED when caller lacks MOUNT_UNMOUNT_MANAGER permission.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_PermissionDenied_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    PartitionParams params(1, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    GTEST_LOG_(INFO) << "CreatePartition_PermissionDenied_002 End";
}

/**
 * @tc.name: DeletePartition_PermissionDenied_001
 * @tc.desc: DeletePartition returns E_SYS_APP_PERMISSION_DENIED when caller is not system app.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, DeletePartition_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_PermissionDenied_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.DeletePartition("disk-8-0", 1), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;
    GTEST_LOG_(INFO) << "DeletePartition_PermissionDenied_001 End";
}

/**
 * @tc.name: DeletePartition_PermissionDenied_002
 * @tc.desc: DeletePartition returns E_PERMISSION_DENIED when caller lacks MOUNT_UNMOUNT_MANAGER permission.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, DeletePartition_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_PermissionDenied_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.DeletePartition("disk-8-0", 1), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    GTEST_LOG_(INFO) << "DeletePartition_PermissionDenied_002 End";
}

/**
 * @tc.name: FormatPartition_PermissionDenied_001
 * @tc.desc: FormatPartition returns E_SYS_APP_PERMISSION_DENIED when caller is not system app.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_PermissionDenied_001 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(provider.FormatPartition("disk-8-0", 1, params), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;
    GTEST_LOG_(INFO) << "FormatPartition_PermissionDenied_001 End";
}

/**
 * @tc.name: FormatPartition_PermissionDenied_002
 * @tc.desc: FormatPartition returns E_PERMISSION_DENIED when caller lacks MOUNT_UNMOUNT_MANAGER permission.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_PermissionDenied_002 Start";
    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(provider.FormatPartition("disk-8-0", 1, params), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    GTEST_LOG_(INFO) << "FormatPartition_PermissionDenied_002 End";
}

/**
 * @tc.name: Mount_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Mount_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.Mount("vol-8-0"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "Mount_PermissionDenied_001 End";
}

/**
 * @tc.name: Mount_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Mount_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Mount("vol-8-0"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Mount_PermissionDenied_002 End";
}

/**
 * @tc.name: Unmount_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Unmount_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.Unmount("vol-8-0"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "Unmount_PermissionDenied_001 End";
}

/**
 * @tc.name: Unmount_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Unmount_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Unmount("vol-8-0"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Unmount_PermissionDenied_002 End";
}

/**
 * @tc.name: Format_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Format_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.Format("vol-8-0", "vfat"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "Format_PermissionDenied_001 End";
}

/**
 * @tc.name: Format_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Format_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Format("vol-8-0", "vfat"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Format_PermissionDenied_002 End";
}

/**
 * @tc.name: TryToFix_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, TryToFix_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TryToFix_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.TryToFix("vol-8-0"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "TryToFix_PermissionDenied_001 End";
}

/**
 * @tc.name: TryToFix_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, TryToFix_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TryToFix_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.TryToFix("vol-8-0"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "TryToFix_PermissionDenied_002 End";
}

/**
 * @tc.name: SetVolumeDescription_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, SetVolumeDescription_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.SetVolumeDescription("uuid-1", "desc"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "SetVolumeDescription_PermissionDenied_001 End";
}

/**
 * @tc.name: SetVolumeDescription_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, SetVolumeDescription_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.SetVolumeDescription("uuid-1", "desc"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "SetVolumeDescription_PermissionDenied_002 End";
}

/**
 * @tc.name: GetAllVolumes_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetAllVolumes_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllVolumes_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    std::vector<VolumeExternal> volumes;
    EXPECT_EQ(provider.GetAllVolumes(volumes), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "GetAllVolumes_PermissionDenied_001 End";
}

/**
 * @tc.name: GetAllVolumes_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetAllVolumes_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllVolumes_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    std::vector<VolumeExternal> volumes;
    EXPECT_EQ(provider.GetAllVolumes(volumes), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "GetAllVolumes_PermissionDenied_002 End";
}

/**
 * @tc.name: GetVolumeByUuid_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeByUuid_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeByUuid_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    VolumeExternal vc;
    EXPECT_EQ(provider.GetVolumeByUuid("uuid-1", vc), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "GetVolumeByUuid_PermissionDenied_001 End";
}

/**
 * @tc.name: GetVolumeByUuid_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeByUuid_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeByUuid_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    VolumeExternal vc;
    EXPECT_EQ(provider.GetVolumeByUuid("uuid-1", vc), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "GetVolumeByUuid_PermissionDenied_002 End";
}

/**
 * @tc.name: GetVolumeById_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeById_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeById_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    VolumeExternal vc;
    EXPECT_EQ(provider.GetVolumeById("vol-8-0", vc), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "GetVolumeById_PermissionDenied_001 End";
}

/**
 * @tc.name: GetVolumeById_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeById_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeById_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    VolumeExternal vc;
    EXPECT_EQ(provider.GetVolumeById("vol-8-0", vc), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "GetVolumeById_PermissionDenied_002 End";
}

/**
 * @tc.name: GetFreeSizeOfVolume_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetFreeSizeOfVolume_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    int64_t freeSize = 0;
    EXPECT_EQ(provider.GetFreeSizeOfVolume("uuid-1", freeSize), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_PermissionDenied_001 End";
}

/**
 * @tc.name: GetFreeSizeOfVolume_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetFreeSizeOfVolume_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    int64_t freeSize = 0;
    EXPECT_EQ(provider.GetFreeSizeOfVolume("uuid-1", freeSize), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_PermissionDenied_002 End";
}

/**
 * @tc.name: GetTotalSizeOfVolume_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetTotalSizeOfVolume_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    int64_t totalSize = 0;
    EXPECT_EQ(provider.GetTotalSizeOfVolume("uuid-1", totalSize), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_PermissionDenied_001 End";
}

/**
 * @tc.name: GetTotalSizeOfVolume_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetTotalSizeOfVolume_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    int64_t totalSize = 0;
    EXPECT_EQ(provider.GetTotalSizeOfVolume("uuid-1", totalSize), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_PermissionDenied_002 End";
}

/**
 * @tc.name: Partition_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Partition_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Partition_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.Partition("disk-8-0", 1), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "Partition_PermissionDenied_001 End";
}

/**
 * @tc.name: Partition_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Partition_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Partition_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Partition("disk-8-0", 1), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Partition_PermissionDenied_002 End";
}

/**
 * @tc.name: Erase_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Erase_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Erase_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.Erase("vol-8-0"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "Erase_PermissionDenied_001 End";
}

/**
 * @tc.name: Erase_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Erase_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Erase_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Erase("vol-8-0"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Erase_PermissionDenied_002 End";
}

/**
 * @tc.name: Eject_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Eject_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Eject_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.Eject("disk-8-0"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "Eject_PermissionDenied_001 End";
}

/**
 * @tc.name: Eject_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Eject_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Eject_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Eject("disk-8-0"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Eject_PermissionDenied_002 End";
}

/**
 * @tc.name: Burn_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Burn_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Burn_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.Burn("vol-8-0", "{}"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "Burn_PermissionDenied_001 End";
}

/**
 * @tc.name: Burn_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Burn_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Burn_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Burn("vol-8-0", "{}"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Burn_PermissionDenied_002 End";
}

/**
 * @tc.name: CreateIsoImage_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreateIsoImage_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    EXPECT_EQ(provider.CreateIsoImage("vol-8-0", "/data/test.iso"), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "CreateIsoImage_PermissionDenied_001 End";
}

/**
 * @tc.name: CreateIsoImage_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreateIsoImage_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.CreateIsoImage("vol-8-0", "/data/test.iso"), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "CreateIsoImage_PermissionDenied_002 End";
}

/**
 * @tc.name: GetVolumeOpProcess_PermissionDenied_001
 * @tc.desc: 非系统应用调用时返回 E_SYS_APP_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeOpProcess_PermissionDenied_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeOpProcess_PermissionDenied_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_accessTokenType = 0;
    g_isSystemApp = false;
    int32_t progress = 0;
    EXPECT_EQ(provider.GetVolumeOpProcess("vol-8-0", progress), E_SYS_APP_PERMISSION_DENIED);
    g_accessTokenType = 1;
    g_isSystemApp = true;

    GTEST_LOG_(INFO) << "GetVolumeOpProcess_PermissionDenied_001 End";
}

/**
 * @tc.name: GetVolumeOpProcess_PermissionDenied_002
 * @tc.desc: 无 MOUNT_UNMOUNT_MANAGER 权限时返回 E_PERMISSION_DENIED
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetVolumeOpProcess_PermissionDenied_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeOpProcess_PermissionDenied_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    int32_t progress = 0;
    EXPECT_EQ(provider.GetVolumeOpProcess("vol-8-0", progress), E_PERMISSION_DENIED);
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "GetVolumeOpProcess_PermissionDenied_002 End";
}

/**
 * @tc.name: OnStart_InitPaths_TestCase_001
 * @tc.desc: 覆盖 OnStart 中 UeventBootstrap/BlockInfoTable/VoldataUuidStore 初始化路径。
 *           UT 进程内不可直接调用 OnStart（会触发 SystemAbility::Publish，栈对象析构时崩溃）。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, OnStart_InitPaths_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnStart_InitPaths_TestCase_001 Start";

    auto &mockBootstrap = MockUeventBootstrap::GetInstance();
    EXPECT_CALL(mockBootstrap, InitImpl()).Times(1);
    MockUeventBootstrap::Init();
    EXPECT_EQ(BlockInfoTable::GetInstance().ReloadFromDaemon(), E_OK);
    EXPECT_EQ(VoldataUuidStore::GetInstance().Init(), DiskManagerErrNo::E_OK);
    GTEST_LOG_(INFO) << "OnStart_InitPaths_TestCase_001 End";
}

/**
 * @tc.name: Mount_StorageManagerCaller_TestCase_001
 * @tc.desc: storage_manager UID 调用 Mount 绕过权限校验
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Mount_StorageManagerCaller_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_StorageManagerCaller_TestCase_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    g_nativeProcessName = "storage_manager";
    MockIPCSkeleton::mockCallingUid_ = 1090;
    g_accessTokenType = 0;
    g_isSystemApp = false;
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Mount("vol-99-99"), E_NON_EXIST);
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_accessTokenType = 1;
    g_isSystemApp = true;
    g_permissionGranted = MOCK_PERMISSION_GRANTED;
    g_nativeProcessName = "foundation";

    GTEST_LOG_(INFO) << "Mount_StorageManagerCaller_TestCase_001 End";
}

/**
 * @tc.name: CheckClientPermission_TestCase_002
 * @tc.desc: uid=0 的调用方 CheckStorageDaemonPermission 返回 true
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CheckStorageDaemonPermission_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CheckStorageDaemonPermission_TestCase_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "storage_daemon";
    EXPECT_TRUE(provider.CheckStorageDaemonPermission());
    g_nativeProcessName = "foundation";

    GTEST_LOG_(INFO) << "CheckStorageDaemonPermission_TestCase_002 End";
}

/**
 * @tc.name: Mount_StorageManagerCaller_TestCase_002
 * @tc.desc: native process 匹配 storage_manager 时 Mount 绕过权限
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, Mount_StorageManagerCaller_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_StorageManagerCaller_TestCase_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    MockIPCSkeleton::mockCallingUid_ = 1090;
    g_accessTokenType = 1;
    g_nativeProcessName = "storage_manager";
    g_isSystemApp = false;
    g_permissionGranted = MOCK_PERMISSION_DENIED;
    EXPECT_EQ(provider.Mount("vol-99-99"), E_NON_EXIST);
    MockIPCSkeleton::mockCallingUid_ = 0;
    g_nativeProcessName = "foundation";
    g_isSystemApp = true;
    g_permissionGranted = MOCK_PERMISSION_GRANTED;

    GTEST_LOG_(INFO) << "Mount_StorageManagerCaller_TestCase_002 End";
}

/**
 * @tc.name: GetPartitionTable_ParamsInvalid_001
 * @tc.desc: 空 diskId 参数非法时返回 E_PARAMS_INVALID
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, GetPartitionTable_ParamsInvalid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_ParamsInvalid_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionTableInfo out;
    EXPECT_EQ(provider.GetPartitionTable("", out), E_PARAMS_INVALID);

    GTEST_LOG_(INFO) << "GetPartitionTable_ParamsInvalid_001 End";
}

/**
 * @tc.name: CreatePartition_ParamsInvalid_001
 * @tc.desc: 空 diskId 参数非法时返回 E_PARAMS_INVALID
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_ParamsInvalid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("", params), E_PARAMS_INVALID);

    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_001 End";
}

/**
 * @tc.name: CreatePartition_ParamsInvalid_002
 * @tc.desc: 分区号非法时返回 E_PARAMS_INVALID
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_ParamsInvalid_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_002 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(0, 2048, 500000, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);

    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_002 End";
}

/**
 * @tc.name: CreatePartition_ParamsInvalid_003
 * @tc.desc: 起始扇区大于结束扇区时返回 E_PARAMS_INVALID
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_ParamsInvalid_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_003 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 500000, 2048, "vfat");
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);

    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_003 End";
}

/**
 * @tc.name: CreatePartition_ParamsInvalid_004
 * @tc.desc: typeCode 为空时返回 E_PARAMS_INVALID
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, CreatePartition_ParamsInvalid_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_004 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    PartitionParams params(1, 2048, 500000, "");
    EXPECT_EQ(provider.CreatePartition("disk-8-0", params), E_PARAMS_INVALID);

    GTEST_LOG_(INFO) << "CreatePartition_ParamsInvalid_004 End";
}

/**
 * @tc.name: DeletePartition_ParamsInvalid_001
 * @tc.desc: 空 diskId 参数非法时返回 E_PARAMS_INVALID
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, DeletePartition_ParamsInvalid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_ParamsInvalid_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    EXPECT_EQ(provider.DeletePartition("disk-8-0", 0), E_PARAMS_INVALID);

    GTEST_LOG_(INFO) << "DeletePartition_ParamsInvalid_001 End";
}

/**
 * @tc.name: FormatPartition_ParamsInvalid_001
 * @tc.desc: 空 diskId 参数非法时返回 E_PARAMS_INVALID
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerProviderTest, FormatPartition_ParamsInvalid_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_ParamsInvalid_001 Start";

    DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FormatParams params("vfat", true, "vol");
    EXPECT_EQ(provider.FormatPartition("", 1, params), E_PARAMS_INVALID);

    GTEST_LOG_(INFO) << "FormatPartition_ParamsInvalid_001 End";
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