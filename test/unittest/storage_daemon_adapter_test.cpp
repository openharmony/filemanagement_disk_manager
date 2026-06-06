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

#include "disk_manager_errno.h"
#include "mock_storage_daemon_adapter.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class StorageDaemonAdapterTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: Connect_TestCase_001
 * @tc.desc: Connect returns E_SA_IS_NULLPTR when sam is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Connect_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Connect()).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.Connect(), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: Connect_TestCase_002
 * @tc.desc: Connect returns E_REMOTE_IS_NULLPTR when remote object is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Connect_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Connect()).WillOnce(Return(E_REMOTE_IS_NULLPTR));
    EXPECT_EQ(adapter.Connect(), E_REMOTE_IS_NULLPTR);
}

/**
 * @tc.name: Connect_TestCase_003
 * @tc.desc: Connect returns E_SERVICE_IS_NULLPTR when iface_cast fails.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Connect_TestCase_003, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Connect()).WillOnce(Return(E_SERVICE_IS_NULLPTR));
    EXPECT_EQ(adapter.Connect(), E_SERVICE_IS_NULLPTR);
}

/**
 * @tc.name: Connect_TestCase_004
 * @tc.desc: Connect returns E_DEATH_RECIPIENT_IS_NULLPTR on OOM.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Connect_TestCase_004, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Connect()).WillOnce(Return(E_DEATH_RECIPIENT_IS_NULLPTR));
    EXPECT_EQ(adapter.Connect(), E_DEATH_RECIPIENT_IS_NULLPTR);
}

/**
 * @tc.name: Connect_TestCase_005
 * @tc.desc: Connect returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Connect_TestCase_005, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Connect()).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Connect(), E_OK);
}

/**
 * @tc.name: Connect_TestCase_006
 * @tc.desc: Connect returns E_OK when called twice (already connected).
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Connect_TestCase_006, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Connect()).Times(2).WillRepeatedly(Return(E_OK));
    EXPECT_EQ(adapter.Connect(), E_OK);
    EXPECT_EQ(adapter.Connect(), E_OK);
}

/**
 * @tc.name: EnsureProxyReady_TestCase_001
 * @tc.desc: EnsureProxyReady returns E_SA_IS_NULLPTR when Connect fails.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, EnsureProxyReady_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, EnsureProxyReady()).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.EnsureProxyReady(), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: EnsureProxyReady_TestCase_002
 * @tc.desc: EnsureProxyReady returns E_SERVICE_IS_NULLPTR when proxy is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, EnsureProxyReady_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, EnsureProxyReady()).WillOnce(Return(E_SERVICE_IS_NULLPTR));
    EXPECT_EQ(adapter.EnsureProxyReady(), E_SERVICE_IS_NULLPTR);
}

/**
 * @tc.name: EnsureProxyReady_TestCase_003
 * @tc.desc: EnsureProxyReady returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, EnsureProxyReady_TestCase_003, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, EnsureProxyReady()).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.EnsureProxyReady(), E_OK);
}

/**
 * @tc.name: ResetSdProxy_TestCase_001
 * @tc.desc: ResetSdProxy returns E_OK normally.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, ResetSdProxy_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, ResetSdProxy()).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.ResetSdProxy(), E_OK);
}

/**
 * @tc.name: ResetSdProxy_TestCase_002
 * @tc.desc: ResetSdProxy returns E_OK when proxy already null.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, ResetSdProxy_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, ResetSdProxy()).Times(2).WillRepeatedly(Return(E_OK));
    EXPECT_EQ(adapter.ResetSdProxy(), E_OK);
    EXPECT_EQ(adapter.ResetSdProxy(), E_OK);
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_001
 * @tc.desc: QueryUsbIsInUse returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, QueryUsbIsInUse_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    bool isInUse = false;
    EXPECT_CALL(adapter, QueryUsbIsInUse(_, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.QueryUsbIsInUse("/dev/block/sda", isInUse), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_002
 * @tc.desc: QueryUsbIsInUse returns E_OK with isInUse=true on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, QueryUsbIsInUse_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    bool isInUse = false;
    EXPECT_CALL(adapter, QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(true), Return(E_OK)));
    EXPECT_EQ(adapter.QueryUsbIsInUse("/dev/block/sda", isInUse), E_OK);
    EXPECT_TRUE(isInUse);
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_001
 * @tc.desc: CreateBlockDeviceNode returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, CreateBlockDeviceNode_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, CreateBlockDeviceNode(_, _, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.CreateBlockDeviceNode("/dev/block/sda", 0600, 8, 1), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_002
 * @tc.desc: CreateBlockDeviceNode returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, CreateBlockDeviceNode_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, CreateBlockDeviceNode(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.CreateBlockDeviceNode("/dev/block/sda", 0600, 8, 1), E_OK);
}

/**
 * @tc.name: DestroyBlockDeviceNode_TestCase_001
 * @tc.desc: DestroyBlockDeviceNode returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, DestroyBlockDeviceNode_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, DestroyBlockDeviceNode(_)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.DestroyBlockDeviceNode("/dev/block/sda"), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: DestroyBlockDeviceNode_TestCase_002
 * @tc.desc: DestroyBlockDeviceNode returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, DestroyBlockDeviceNode_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, DestroyBlockDeviceNode(_)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.DestroyBlockDeviceNode("/dev/block/sda"), E_OK);
}

/**
 * @tc.name: ReadPartitionTable_TestCase_001
 * @tc.desc: ReadPartitionTable returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, ReadPartitionTable_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string output;
    int32_t maxVolume = 0;
    EXPECT_CALL(adapter, ReadPartitionTable(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.ReadPartitionTable("/dev/block/sda", output, maxVolume), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: ReadPartitionTable_TestCase_002
 * @tc.desc: ReadPartitionTable returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, ReadPartitionTable_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string output;
    int32_t maxVolume = 0;
    EXPECT_CALL(adapter, ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("partition1\npartition2\n"),
                        SetArgReferee<2>(2), Return(E_OK)));
    EXPECT_EQ(adapter.ReadPartitionTable("/dev/block/sda", output, maxVolume), E_OK);
    EXPECT_EQ(maxVolume, 2);
}

/**
 * @tc.name: Eject_TestCase_001
 * @tc.desc: Eject returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Eject_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Eject(_)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.Eject("vol-1"), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: Eject_TestCase_002
 * @tc.desc: Eject returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Eject_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Eject(_)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Eject("vol-1"), E_OK);
}

/**
 * @tc.name: QueryCDStatus_TestCase_001
 * @tc.desc: QueryCDStatus returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, QueryCDStatus_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t status = 0;
    EXPECT_CALL(adapter, QueryCDStatus(_, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.QueryCDStatus("/dev/sr0", status), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: QueryCDStatus_TestCase_002
 * @tc.desc: QueryCDStatus returns E_OK with status on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, QueryCDStatus_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t status = 0;
    EXPECT_CALL(adapter, QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_EQ(adapter.QueryCDStatus("/dev/sr0", status), E_OK);
    EXPECT_EQ(status, 1);
}

/**
 * @tc.name: Mount_TestCase_001
 * @tc.desc: Mount returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Mount_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Mount(_, _, _, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.Mount("/dev/block/sda1", "/mnt/data", "ext4", 0, ""), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: Mount_TestCase_002
 * @tc.desc: Mount returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Mount_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Mount(_, _, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Mount("/dev/block/sda1", "/mnt/data", "ext4", 0, ""), E_OK);
}

/**
 * @tc.name: Unmount_TestCase_001
 * @tc.desc: Unmount returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Unmount_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Unmount(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.Unmount("/mnt/data", "ext4", false), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: Unmount_TestCase_002
 * @tc.desc: Unmount returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Unmount_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Unmount(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Unmount("/mnt/data", "ext4", false), E_OK);
}

/**
 * @tc.name: FormatVolume_TestCase_001
 * @tc.desc: FormatVolume returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, FormatVolume_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, FormatVolume(_, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.FormatVolume("/dev/block/sda1", "ext4"), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: FormatVolume_TestCase_002
 * @tc.desc: FormatVolume returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, FormatVolume_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, FormatVolume(_, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.FormatVolume("/dev/block/sda1", "ext4"), E_OK);
}

/**
 * @tc.name: Check_TestCase_001
 * @tc.desc: Check returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Check_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Check(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.Check("/dev/block/sda1", "ext4", false), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: Check_TestCase_002
 * @tc.desc: Check returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Check_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Check(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Check("/dev/block/sda1", "ext4", false), E_OK);
}

/**
 * @tc.name: Repair_TestCase_001
 * @tc.desc: Repair returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Repair_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Repair(_, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.Repair("/dev/block/sda1", "ext4"), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: Repair_TestCase_002
 * @tc.desc: Repair returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Repair_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Repair(_, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Repair("/dev/block/sda1", "ext4"), E_OK);
}

/**
 * @tc.name: SetLabel_TestCase_001
 * @tc.desc: SetLabel returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, SetLabel_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, SetLabel(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.SetLabel("/dev/block/sda1", "ext4", "mylabel"), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: SetLabel_TestCase_002
 * @tc.desc: SetLabel returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, SetLabel_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, SetLabel(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.SetLabel("/dev/block/sda1", "ext4", "mylabel"), E_OK);
}

/**
 * @tc.name: ReadMetadata_TestCase_001
 * @tc.desc: ReadMetadata returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, ReadMetadata_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string uuid, type, label;
    EXPECT_CALL(adapter, ReadMetadata(_, _, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.ReadMetadata("/dev/block/sda1", uuid, type, label), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: ReadMetadata_TestCase_002
 * @tc.desc: ReadMetadata returns E_OK with metadata on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, ReadMetadata_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string uuid, type, label;
    EXPECT_CALL(adapter, ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("uuid-1234"),
                        SetArgReferee<2>("ext4"),
                        SetArgReferee<3>("mylabel"), Return(E_OK)));
    EXPECT_EQ(adapter.ReadMetadata("/dev/block/sda1", uuid, type, label), E_OK);
    EXPECT_EQ(uuid, "uuid-1234");
    EXPECT_EQ(type, "ext4");
    EXPECT_EQ(label, "mylabel");
}

/**
 * @tc.name: GetCapacity_TestCase_001
 * @tc.desc: GetCapacity returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetCapacity_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int64_t totalSize = 0;
    int64_t freeSize = 0;
    EXPECT_CALL(adapter, GetCapacity(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.GetCapacity("/mnt/data", totalSize, freeSize), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: GetCapacity_TestCase_002
 * @tc.desc: GetCapacity returns E_OK with sizes on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetCapacity_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int64_t totalSize = 0;
    int64_t freeSize = 0;
    EXPECT_CALL(adapter, GetCapacity(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(4096), SetArgReferee<2>(2048), Return(E_OK)));
    EXPECT_EQ(adapter.GetCapacity("/mnt/data", totalSize, freeSize), E_OK);
    EXPECT_EQ(totalSize, 4096);
    EXPECT_EQ(freeSize, 2048);
}

/**
 * @tc.name: MountFuseDevice_TestCase_001
 * @tc.desc: MountFuseDevice returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, MountFuseDevice_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t fuseFd = -1;
    EXPECT_CALL(adapter, MountFuseDevice(_, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.MountFuseDevice("/mnt/fuse", fuseFd), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: MountFuseDevice_TestCase_002
 * @tc.desc: MountFuseDevice returns E_OK with fuseFd on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, MountFuseDevice_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t fuseFd = -1;
    EXPECT_CALL(adapter, MountFuseDevice(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(42), Return(E_OK)));
    EXPECT_EQ(adapter.MountFuseDevice("/mnt/fuse", fuseFd), E_OK);
    EXPECT_EQ(fuseFd, 42);
}

/**
 * @tc.name: Partition_TestCase_001
 * @tc.desc: Partition returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Partition_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Partition(_, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.Partition("/dev/block/sda", "gpt"), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: Partition_TestCase_002
 * @tc.desc: Partition returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, Partition_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, Partition(_, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Partition("/dev/block/sda", "gpt"), E_OK);
}

/**
 * @tc.name: GetBlockInfoByType2Param_TestCase_001
 * @tc.desc: GetBlockInfoByType (2-param) returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetBlockInfoByType2Param_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_CALL(adapter, GetBlockInfoByType(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.GetBlockInfoByType("usb", blockInfos, ""), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: GetBlockInfoByType2Param_TestCase_002
 * @tc.desc: GetBlockInfoByType (2-param) returns E_OK with blockInfos on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetBlockInfoByType2Param_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_CALL(adapter, GetBlockInfoByType(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("block-info-data"), Return(E_OK)));
    EXPECT_EQ(adapter.GetBlockInfoByType("usb", blockInfos, ""), E_OK);
    EXPECT_EQ(blockInfos, "block-info-data");
}

/**
 * @tc.name: GetBlockInfoByType3Param_TestCase_001
 * @tc.desc: GetBlockInfoByType (3-param) returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetBlockInfoByType3Param_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_CALL(adapter, GetBlockInfoByType(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.GetBlockInfoByType("usb", blockInfos, "disk-1"), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: GetBlockInfoByType3Param_TestCase_002
 * @tc.desc: GetBlockInfoByType (3-param) returns E_OK with blockInfos on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetBlockInfoByType3Param_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_CALL(adapter, GetBlockInfoByType(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("block-info-data"), Return(E_OK)));
    EXPECT_EQ(adapter.GetBlockInfoByType("usb", blockInfos, "disk-1"), E_OK);
    EXPECT_EQ(blockInfos, "block-info-data");
}

/**
 * @tc.name: GetPartitionTableInfo_TestCase_001
 * @tc.desc: GetPartitionTableInfo returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetPartitionTableInfo_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string execRet;
    EXPECT_CALL(adapter, GetPartitionTableInfo(_, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.GetPartitionTableInfo("/dev/block/sda", execRet), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: GetPartitionTableInfo_TestCase_002
 * @tc.desc: GetPartitionTableInfo returns E_OK with execRet on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, GetPartitionTableInfo_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string execRet;
    EXPECT_CALL(adapter, GetPartitionTableInfo(_, _))
        .WillOnce(DoAll(SetArgReferee<1>("partition-table-json"), Return(E_OK)));
    EXPECT_EQ(adapter.GetPartitionTableInfo("/dev/block/sda", execRet), E_OK);
    EXPECT_EQ(execRet, "partition-table-json");
}

/**
 * @tc.name: CreatePartition_TestCase_001
 * @tc.desc: CreatePartition returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, CreatePartition_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, CreatePartition(_, _, _, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.CreatePartition("/dev/block/sda", 1, 2048, 500000, "0700"),
              E_SA_IS_NULLPTR);
}

/**
 * @tc.name: CreatePartition_TestCase_002
 * @tc.desc: CreatePartition returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, CreatePartition_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, CreatePartition(_, _, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.CreatePartition("/dev/block/sda", 1, 2048, 500000, "0700"), E_OK);
}

/**
 * @tc.name: DeletePartition_TestCase_001
 * @tc.desc: DeletePartition returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, DeletePartition_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, DeletePartition(_, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.DeletePartition("/dev/block/sda", "sda", 1), E_SA_IS_NULLPTR);
}

/**
 * @tc.name: DeletePartition_TestCase_002
 * @tc.desc: DeletePartition returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, DeletePartition_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, DeletePartition(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.DeletePartition("/dev/block/sda", "sda", 1), E_OK);
}

/**
 * @tc.name: FormatPartition_TestCase_001
 * @tc.desc: FormatPartition returns error when proxy not ready.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, FormatPartition_TestCase_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, FormatPartition(_, _, _, _)).WillOnce(Return(E_SA_IS_NULLPTR));
    EXPECT_EQ(adapter.FormatPartition("/dev/block/sda1", "ext4", "volume1", false),
              E_SA_IS_NULLPTR);
}

/**
 * @tc.name: FormatPartition_TestCase_002
 * @tc.desc: FormatPartition returns E_OK on success.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonAdapterTest, FormatPartition_TestCase_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(adapter, FormatPartition(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.FormatPartition("/dev/block/sda1", "ext4", "volume1", false), E_OK);
}

} // namespace DiskManager
} // namespace OHOS