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

#include "disk/uevent_bootstrap.h"
#include "disk/uevent_env_parser.h"
#include "disk/disk_config.h"
#include "disk/disk_manager.h"
#include "disk/storage_spec_models.h"
#include "disk_manager_errno.h"
#include "errors.h"
#include "mock_storage_daemon_adapter.h"
#include "disk_manager_mock.h"
#include "mock_block_info_table.h"
#include "mock_common_event_publisher.h"
#include "volume_core.h"
#include "volume_external.h"

#include <string>
#include <vector>

using namespace OHOS::DiskManager;
using namespace testing;
using namespace testing::ext;

constexpr int DISK_METADATA_ARG_UUID = 1;
constexpr int DISK_METADATA_ARG_TYPE = 2;
constexpr int DISK_METADATA_ARG_LABEL = 3;

class UeventBootstrapTest : public Test {
protected:
    static void SetUpTestCase()
    {
        testing::Mock::AllowLeak(&DiskManager::GetInstance());
        testing::Mock::AllowLeak(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::AllowLeak(&BlockInfoTable::GetInstance());
        testing::Mock::AllowLeak(&CommonEventPublisher::GetInstance());
    }

    void SetUp() override
    {
        UeventBootstrap::diskConfigList_.clear();
        ON_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillByDefault(Return(E_OK));
        ON_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillByDefault(Return(E_OK));
        ON_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillByDefault(Return(false));
        ON_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillByDefault(Return(E_OK));
        ON_CALL(DiskManager::GetInstance(), GetAllVolumes(_)).WillByDefault(Return(E_OK));
        ON_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
            .WillByDefault(DoAll(SetArgReferee<DISK_METADATA_ARG_UUID>(std::string("test-uuid")),
                                 SetArgReferee<DISK_METADATA_ARG_TYPE>(std::string("")),
                                 SetArgReferee<DISK_METADATA_ARG_LABEL>(std::string("")),
                                 Return(E_OK)));
        ON_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
            .WillByDefault(Return(E_OK));
        ON_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _)).WillByDefault(Return(-1));
        ON_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
            .WillByDefault(Return(std::string("{}")));
    }

    void TearDown() override
    {
        testing::Mock::VerifyAndClearExpectations(&DiskManager::GetInstance());
        testing::Mock::VerifyAndClearExpectations(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::VerifyAndClearExpectations(&BlockInfoTable::GetInstance());
        testing::Mock::VerifyAndClearExpectations(&CommonEventPublisher::GetInstance());
        UeventBootstrap::diskConfigList_.clear();
    }
};

static UeventEnv MakeUenv(const std::string &action,
                          unsigned int major,
                          unsigned int minor,
                          const std::string &devPath = "",
                          const std::string &devType = "disk",
                          const std::string &subsystem = "block",
                          const std::string &devName = "sda",
                          bool ejectRequest = false,
                          const std::string &sysPath = "")
{
    UeventEnv env;
    env.action = action;
    env.major = major;
    env.minor = minor;
    env.devPath = devPath;
    env.devType = devType;
    env.subsystem = subsystem;
    env.devName = devName;
    env.ejectRequest = ejectRequest;
    env.sysPath = sysPath;
    return env;
}

HWTEST_F(UeventBootstrapTest, MatchConfig_NoMatch_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk");
    EXPECT_EQ(UeventBootstrap::MatchConfig(env), 0u);
}

HWTEST_F(UeventBootstrapTest, MatchConfig_UsbMatch_TestCase_002, TestSize.Level0)
{
    DiskConfig cfg("/devices/usb*", "usb", USB_FLAG);
    UeventBootstrap::diskConfigList_.push_back(cfg);
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/usb/sda", "disk");
    uint32_t flag = UeventBootstrap::MatchConfig(env);
    EXPECT_TRUE((flag & USB_FLAG) != 0);
}

HWTEST_F(UeventBootstrapTest, MatchConfig_MmcMatch_TestCase_003, TestSize.Level0)
{
    DiskConfig cfg("/devices/mmc*", "sd", SD_FLAG);
    UeventBootstrap::diskConfigList_.push_back(cfg);
    UeventEnv env = MakeUenv("add", 179, 0, "/devices/mmcblk0", "disk");
    uint32_t flag = UeventBootstrap::MatchConfig(env);
    EXPECT_TRUE((flag & SD_FLAG) != 0);
}

HWTEST_F(UeventBootstrapTest, MatchConfig_CdMatch_TestCase_004, TestSize.Level0)
{
    DiskConfig cfg("/devices/sr*", "cd", CD_FLAG);
    UeventBootstrap::diskConfigList_.push_back(cfg);
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk");
    uint32_t flag = UeventBootstrap::MatchConfig(env);
    EXPECT_TRUE((flag & CD_FLAG) != 0);
}

HWTEST_F(UeventBootstrapTest, MatchConfig_DvrUsb_TestCase_005, TestSize.Level0)
{
    DiskConfig cfg("/devices/usb*", "dvr", DVR_USB);
    UeventBootstrap::diskConfigList_.push_back(cfg);
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/usb/sda", "disk");
    uint32_t flag = UeventBootstrap::MatchConfig(env);
    EXPECT_EQ(flag, static_cast<uint32_t>(DVR_USB));
}

HWTEST_F(UeventBootstrapTest, SplitLine_TestCase_001, TestSize.Level0)
{
    std::string line = "sysPattern /devices/usb* label MyUSB flag 1";
    std::string token = " ";
    auto result = UeventBootstrap::SplitLine(line, token);
    EXPECT_EQ(result.size(), 6u);
    EXPECT_EQ(result[0], "sysPattern");
    EXPECT_EQ(result[1], "/devices/usb*");
}

HWTEST_F(UeventBootstrapTest, SplitLine_SingleToken_TestCase_002, TestSize.Level0)
{
    std::string line = "hello";
    std::string token = " ";
    auto result = UeventBootstrap::SplitLine(line, token);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], "hello");
}

HWTEST_F(UeventBootstrapTest, SplitLine_EmptyString_TestCase_003, TestSize.Level0)
{
    std::string line = "";
    std::string token = " ";
    auto result = UeventBootstrap::SplitLine(line, token);
    EXPECT_EQ(result.size(), 0u);
}

HWTEST_F(UeventBootstrapTest, SplitLine_TrailingToken_TestCase_004, TestSize.Level0)
{
    std::string line = "a b ";
    std::string token = " ";
    auto result = UeventBootstrap::SplitLine(line, token);
    EXPECT_EQ(result.size(), 2u);
}

HWTEST_F(UeventBootstrapTest, OnBlockDiskUevent_ParseFail_TestCase_001, TestSize.Level0)
{
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(""), DiskManagerErrNo::E_UEVENT_PARSE_FAILED);
}

HWTEST_F(UeventBootstrapTest, OnBlockDiskUevent_NonBlockDisk_TestCase_002, TestSize.Level0)
{
    std::string msg = "ACTION=add\nSUBSYSTEM=net\nDEVPATH=/devices/net\nDEVTYPE=interface\nMAJOR=8\nMINOR=0\n";
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(msg), DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, OnBlockDiskUevent_DefaultAction_TestCase_003, TestSize.Level0)
{
    std::string msg = "ACTION=online\nSUBSYSTEM=block\nDEVPATH=/devices/block\nDEVTYPE=disk\nMAJOR=8\nMINOR=1\n";
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(msg), DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskChange_IsPartitioning_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("change", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(true));
    int32_t ret = UeventBootstrap::HandleDiskChange(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_DestroyFail_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_NE(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_Success_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_NoDisk_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskAdd_Success_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
            .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskAdd_HasDiskExist_TestCase_007, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(true));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskChange_HasDiskExist_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false))
        .WillOnce(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(true));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskChange(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, MatchConfig_NoMatchWithConfig_TestCase_006, TestSize.Level0)
{
    DiskConfig cfg("/devices/other*", "other", 99);
    UeventBootstrap::diskConfigList_.push_back(cfg);
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk");
    EXPECT_EQ(UeventBootstrap::MatchConfig(env), 0u);
}

HWTEST_F(UeventBootstrapTest, UpsertDisk_HasBlockInfo_TestCase_004, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &out) {
            out.sizeBytes = 1024;
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{\"sizeBytes\":1024}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, UpsertDisk_ReadExtDiskInfoSuccess_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.sizeBytes = 2048;
            info.vendor = "TestVendor";
            info.model = "TestModel";
            return OHOS::ERR_OK;
        }))
        .WillOnce(Return(-1));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{\"sizeBytes\":2048}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, UpsertDisk_MatchConfigSuccess_TestCase_006, TestSize.Level0)
{
    DiskConfig cfg("/devices/usb*", "usb", USB_FLAG);
    UeventBootstrap::diskConfigList_.push_back(cfg);
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/usb/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_QueryCDStatusFail_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(0), Return(-1)));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_NonEmptyDisc_TestCase_004, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("udf")),
                        SetArgReferee<3>(std::string("DVD")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_EmptyDisc_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(3), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_NonEmptyDisc_CreateVolumeFail_TestCase_006, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(-1));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_NonEmptyDisc_TypeEmpty_TestCase_007, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_NonEmptyDisc_MountFail_TestCase_008, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("udf")),
                        SetArgReferee<3>(std::string("DVD")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_EjectFail_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Eject(_))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitions_ReadPartitionTableFail_TestCase_004, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(-1)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitions_NonEmptyShortDump_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string shortDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(shortDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithNormalPartition_MountSuccess_TestCase_006, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("vfat")),
                        SetArgReferee<3>(std::string("MyVol")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_CreateVolumeFail_TestCase_007, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(-1));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_IsPartitioning_FormatFail_TestCase_008, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_IsPartitioning_FormatSuccess_TestCase_009, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_TypeEmpty_TestCase_010, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_MountFail_TestCase_011, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("vfat")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_DiskNotFound_TestCase_012, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_ReadExtDiskInfoSuccess_TestCase_013, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.vendor = "TestVendor";
            info.model = "TestModel";
            return OHOS::ERR_OK;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_ReadMetadataFail_TestCase_014, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(partitionDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Discover_WithPartition_IsUserData_TestCase_017, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 179, 0, "/devices/mmcblk0", "disk", "block", "mmcblk0");
    std::string largeDump;
    largeDump += "DISK 179 0 gpt\n";
    for (int i = 0; i < 33; ++i) {
        largeDump += "179-" + std::to_string(i) + " 0 1024 0700 part" + std::to_string(i) + "\n";
    }
    size_t pos = largeDump.find("179-1");
    if (pos != std::string::npos) {
        largeDump.replace(pos, 25, "179-1 0 2048 0700 userdata\n");
    }
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(largeDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_UnmountFail_TestCase_007, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", UNMOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), Unmount(_))
        .WillOnce(Return(-1));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishVolumeChangeImpl(REMOVED, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_DestroyVolumeNodeFail_TestCase_008, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", UNMOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), Unmount(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(-1))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, Init_TestCase_001, TestSize.Level0)
{
    UeventBootstrap::Init();
}

HWTEST_F(UeventBootstrapTest, HandleDiskAdd_CreateNodeFail_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_NE(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskChange_NotPartitioning_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, OnBlockDiskUevent_Remove_TestCase_004, TestSize.Level0)
{
    std::string msg = "ACTION=remove\nSUBSYSTEM=block\nDEVPATH=/devices/sda\nDEVTYPE=disk\nMAJOR=8\nMINOR=1\n";
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(msg), DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, OnBlockDiskUevent_Add_TestCase_005, TestSize.Level0)
{
    std::string msg = "ACTION=add\nSUBSYSTEM=block\nDEVPATH=/devices/sda\nDEVTYPE=disk\nMAJOR=8\nMINOR=1\n";
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(msg), DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, OnBlockDiskUevent_Change_TestCase_006, TestSize.Level0)
{
    std::string msg = "ACTION=change\nSUBSYSTEM=block\nDEVPATH=/devices/sda\nDEVTYPE=disk\nMAJOR=8\nMINOR=1\n";
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(true));
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(msg), DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_CD_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_CD_Eject_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Eject(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_NoPartNoPublish_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, RediscoverDiskVolumes_TestCase_001, TestSize.Level0)
{
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(false));
    int32_t ret = UeventBootstrap::RediscoverDiskVolumes("disk-8-1");
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_WithVolumes_TestCase_004, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", UNMOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), Unmount(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishVolumeChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_BadRemoval_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", MOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), Unmount(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishVolumeChangeImpl(BAD_REMOVAL, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_VolumeZeroId_TestCase_006, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("0", EXTERNAL, "disk-8-1", UNMOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskDestroyed(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskRemove(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}