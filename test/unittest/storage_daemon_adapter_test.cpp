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
#include "storage_daemon_adapter.h"
#include "storage_daemon_mock.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class StorageDaemonAdapterTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(StorageDaemonAdapterTest, Connect_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Connect(), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Connect_ErrorPath_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Connect(), E_OK);
    EXPECT_NE(adapter.Connect(), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, EnsureProxyReady_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.EnsureProxyReady(), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, EnsureProxyReady_ErrorPath_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    adapter.storageDaemon_ = nullptr;
    EXPECT_NE(adapter.EnsureProxyReady(), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, ResetSdProxy_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_EQ(adapter.ResetSdProxy(), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, ResetSdProxy_Nullptr_002, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_EQ(adapter.ResetSdProxy(), E_OK);
    EXPECT_EQ(adapter.ResetSdProxy(), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, QueryUsbIsInUse_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    bool isInUse = false;
    EXPECT_NE(adapter.QueryUsbIsInUse("/dev/block/sda", isInUse), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, CreateBlockDeviceNode_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.CreateBlockDeviceNode("/dev/block/sda", 0600, 8, 1), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, DestroyBlockDeviceNode_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.DestroyBlockDeviceNode("/dev/block/sda"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, ReadPartitionTable_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string output;
    int32_t maxVolume = 0;
    EXPECT_NE(adapter.ReadPartitionTable("/dev/block/sda", output, maxVolume), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, QueryCDStatus_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t status = 0;
    EXPECT_NE(adapter.QueryCDStatus("/dev/sr0", status), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Mount_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Mount("/dev/block/sda1", "/mnt/data", "ext4", 0, ""), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Unmount_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Unmount("/mnt/data", "ext4", false), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, FormatVolume_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.FormatVolume("/dev/block/sda1", "ext4"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Check_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Check("/dev/block/sda1", "ext4", false), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Repair_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Repair("/dev/block/sda1", "ext4"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, SetLabel_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.SetLabel("/dev/block/sda1", "ext4", "mylabel"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, ReadMetadata_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string uuid, type, label;
    EXPECT_NE(adapter.ReadMetadata("/dev/block/sda1", uuid, type, label), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, GetCapacity_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int64_t totalSize = 0;
    int64_t freeSize = 0;
    EXPECT_NE(adapter.GetCapacity("/mnt/data", totalSize, freeSize), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, MountFuseDevice_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t fuseFd = -1;
    EXPECT_NE(adapter.MountFuseDevice("/mnt/fuse", fuseFd), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Partition_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Partition("/dev/block/sda", "gpt"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, GetBlockInfoByType2Param_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_NE(adapter.GetBlockInfoByType("usb", blockInfos, ""), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, GetBlockInfoByType3Param_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_NE(adapter.GetBlockInfoByType("usb", blockInfos, "disk-1"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, GetPartitionTableInfo_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string execRet;
    EXPECT_NE(adapter.GetPartitionTableInfo("/dev/block/sda", execRet), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, CreatePartition_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.CreatePartition("/dev/block/sda", 1, 2048, 500000, "0700"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, DeletePartition_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.DeletePartition("/dev/block/sda", "sda", 1), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, FormatPartition_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.FormatPartition("/dev/block/sda1", "ext4", "volume1", false), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Erase_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Erase("/dev/block/sda"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Eject_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Eject("disk-1"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, CreateIsoImage_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.CreateIsoImage("/dev/sr0", "/tmp/image.iso", "udf", "/mnt/cdrom"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, Burn_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.Burn("/dev/sr0", "-speed=4", "udf"), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, GetVolumeOpProcess_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t progressPct = 0;
    EXPECT_NE(adapter.GetVolumeOpProcess("vol-1", progressPct), E_OK);
}

HWTEST_F(StorageDaemonAdapterTest, VerifyBurnData_ErrorPath_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_NE(adapter.VerifyBurnData("/dev/sr0", 1), E_OK);
}

class StorageDaemonAdapterProxyTest : public testing::Test {
public:
    void SetUp() override
    {
        auto &adapter = StorageDaemonAdapter::GetInstance();
        adapter.ResetSdProxy();
        adapter.deathRecipient_ = nullptr;
        mockRemote_ = new StorageDaemonMock();
        testing::Mock::AllowLeak(mockRemote_.GetRefPtr());
        adapter.storageDaemon_ = mockRemote_;
        adapter.deathRecipient_ = new SdDeathRecipient();
        mockRemote_->AsObject()->AddDeathRecipient(adapter.deathRecipient_);
    }

    void TearDown() override
    {
        auto &adapter = StorageDaemonAdapter::GetInstance();
        adapter.ResetSdProxy();
        adapter.deathRecipient_ = nullptr;
    }

    sptr<StorageDaemonMock> mockRemote_;
};

HWTEST_F(StorageDaemonAdapterProxyTest, EnsureProxyReady_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_EQ(adapter.EnsureProxyReady(), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Connect_AlreadyConnected_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_EQ(adapter.Connect(), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, ResetSdProxy_WithMock_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_EQ(adapter.ResetSdProxy(), E_OK);
    EXPECT_EQ(adapter.storageDaemon_, nullptr);
}

HWTEST_F(StorageDaemonAdapterProxyTest, EnsureProxyReady_NullAfterReset_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    adapter.ResetSdProxy();
    adapter.storageDaemon_ = nullptr;
    EXPECT_NE(adapter.EnsureProxyReady(), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, QueryUsbIsInUse_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    bool isInUse = false;
    EXPECT_CALL(*mockRemote_, QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(true), Return(E_OK)));
    EXPECT_EQ(adapter.QueryUsbIsInUse("/dev/block/sda", isInUse), E_OK);
    EXPECT_TRUE(isInUse);
}

HWTEST_F(StorageDaemonAdapterProxyTest, CreateBlockDeviceNode_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, CreateBlockDeviceNode(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.CreateBlockDeviceNode("/dev/block/sda", 0600, 8, 1), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, DestroyBlockDeviceNode_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, DestroyBlockDeviceNode(_)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.DestroyBlockDeviceNode("/dev/block/sda"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, ReadPartitionTable_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string output;
    int32_t maxVolume = 0;
    EXPECT_CALL(*mockRemote_, ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("partition-data"), SetArgReferee<2>(4), Return(E_OK)));
    EXPECT_EQ(adapter.ReadPartitionTable("/dev/block/sda", output, maxVolume), E_OK);
    EXPECT_EQ(output, "partition-data");
    EXPECT_EQ(maxVolume, 4);
}

HWTEST_F(StorageDaemonAdapterProxyTest, QueryCDStatus_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t status = 0;
    EXPECT_CALL(*mockRemote_, QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_EQ(adapter.QueryCDStatus("/dev/sr0", status), E_OK);
    EXPECT_EQ(status, 1);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Mount_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Mount(_, _, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Mount("/dev/block/sda1", "/mnt/data", "ext4", 0, ""), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Unmount_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Unmount(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Unmount("/mnt/data", "ext4", false), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, FormatVolume_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, FormatVolume(_, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.FormatVolume("/dev/block/sda1", "ext4"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Check_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Check(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Check("/dev/block/sda1", "ext4", false), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Repair_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Repair(_, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Repair("/dev/block/sda1", "ext4"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, SetLabel_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, SetLabel(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.SetLabel("/dev/block/sda1", "ext4", "mylabel"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, ReadMetadata_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string uuid, type, label;
    EXPECT_CALL(*mockRemote_, ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("uuid-1234"), SetArgReferee<2>("ext4"), SetArgReferee<3>("mylabel"),
                        Return(E_OK)));
    EXPECT_EQ(adapter.ReadMetadata("/dev/block/sda1", uuid, type, label), E_OK);
    EXPECT_EQ(uuid, "uuid-1234");
    EXPECT_EQ(type, "ext4");
    EXPECT_EQ(label, "mylabel");
}

HWTEST_F(StorageDaemonAdapterProxyTest, GetCapacity_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int64_t totalSize = 0;
    int64_t freeSize = 0;
    EXPECT_CALL(*mockRemote_, GetCapacity(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(1024 * 1024), SetArgReferee<2>(128 * 1024), Return(E_OK)));
    EXPECT_EQ(adapter.GetCapacity("/mnt/data", totalSize, freeSize), E_OK);
    EXPECT_EQ(totalSize, 1024 * 1024);
    EXPECT_EQ(freeSize, 128 * 1024);
}

HWTEST_F(StorageDaemonAdapterProxyTest, MountFuseDevice_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t fuseFd = -1;
    EXPECT_CALL(*mockRemote_, MountFuseDevice(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(42), Return(E_OK)));
    EXPECT_EQ(adapter.MountFuseDevice("/mnt/fuse", fuseFd), E_OK);
    EXPECT_EQ(fuseFd, 42);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Partition_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Partition(_, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Partition("/dev/block/sda", "gpt"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, GetBlockInfoByType2Param_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_CALL(*mockRemote_, GetBlockInfoByType(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>("block-info-data"), Return(E_OK)));
    EXPECT_EQ(adapter.GetBlockInfoByType("usb", blockInfos, ""), E_OK);
    EXPECT_EQ(blockInfos, "block-info-data");
}

HWTEST_F(StorageDaemonAdapterProxyTest, GetBlockInfoByType3Param_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string blockInfos;
    EXPECT_CALL(*mockRemote_, GetBlockInfoByType(_, _, _))
        .WillOnce(DoAll(SetArgReferee<2>("block-info-data2"), Return(E_OK)));
    EXPECT_EQ(adapter.GetBlockInfoByType("usb", blockInfos, "disk-1"), E_OK);
    EXPECT_EQ(blockInfos, "block-info-data2");
}

HWTEST_F(StorageDaemonAdapterProxyTest, GetPartitionTableInfo_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    std::string execRet;
    EXPECT_CALL(*mockRemote_, GetPartitionTableInfo(_, _))
        .WillOnce(DoAll(SetArgReferee<1>("table-data"), Return(E_OK)));
    EXPECT_EQ(adapter.GetPartitionTableInfo("/dev/block/sda", execRet), E_OK);
    EXPECT_EQ(execRet, "table-data");
}

HWTEST_F(StorageDaemonAdapterProxyTest, CreatePartition_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, CreatePartition(_, _, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.CreatePartition("/dev/block/sda", 1, 2048, 500000, "0700"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, DeletePartition_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, DeletePartitionInfo(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.DeletePartition("/dev/block/sda", "sda", 1), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, FormatPartition_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, FormatPartition(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.FormatPartition("/dev/block/sda1", "ext4", "volume1", false), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Erase_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Erase(_)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Erase("/dev/block/sda"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Eject_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Eject(_)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Eject("disk-1"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, CreateIsoImage_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, CreateIsoImage(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.CreateIsoImage("/dev/sr0", "/tmp/image.iso", "udf", "/mnt/cdrom"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, Burn_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, Burn(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.Burn("/dev/sr0", "-speed=4", "udf"), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, GetVolumeOpProcess_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    int32_t progressPct = 0;
    EXPECT_CALL(*mockRemote_, GetVolumeOpProcess(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(50), Return(E_OK)));
    EXPECT_EQ(adapter.GetVolumeOpProcess("vol-1", progressPct), E_OK);
    EXPECT_EQ(progressPct, 50);
}

HWTEST_F(StorageDaemonAdapterProxyTest, VerifyBurnData_Success_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    EXPECT_CALL(*mockRemote_, VerifyBurnData(_, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(adapter.VerifyBurnData("/dev/sr0", 1), E_OK);
}

HWTEST_F(StorageDaemonAdapterProxyTest, SdDeathRecipient_OnRemoteDied_001, TestSize.Level0)
{
    auto &adapter = StorageDaemonAdapter::GetInstance();
    SdDeathRecipient recipient;
    wptr<IRemoteObject> weakObj(mockRemote_->AsObject());
    recipient.OnRemoteDied(weakObj);
    EXPECT_EQ(adapter.storageDaemon_, nullptr);
}

} // namespace DiskManager
} // namespace OHOS