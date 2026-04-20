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

/**
 * @file storage_daemon_proxy_test.cpp
 * @brief 单元测试：StorageDaemonProxy（与 StorageDaemon IPC 的序列化、SendRequest、应答解析）。
 *
 * 通过 MessageParcelMock / StorageDaemonMock 覆盖各 API 的失败分支与成功路径。
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "errors.h"
#include "message_parcel_mock.h"
#include "storage_daemon_mock.h"
#include "storage_daemon_proxy.h"

namespace OHOS {
namespace DiskManager {
using namespace testing;
using namespace testing::ext;
using ::testing::_;
using ::testing::NiceMock;

namespace {
constexpr int32_t IPC_FAILED = -1001;
constexpr int32_t REMOTE_FAILED = -2002;
} // namespace

class StorageDaemonProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GTEST_LOG_(INFO) << "StorageDaemonProxyTest SetUpTestCase";
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "StorageDaemonProxyTest TearDownTestCase";
    }

    void SetUp() override
    {
        GTEST_LOG_(INFO) << "StorageDaemonProxyTest SetUp";
        // NiceMock：未写 EXPECT_CALL 的成员调用不触发 GMOCK「未期望调用」告警，仅返回类型缺省。
        messageParcelMock_ = std::make_shared<NiceMock<MessageParcelMock>>();
        MockMessageParcel::messageParcel = messageParcelMock_;
        remote_ = new StorageDaemonMock();
        proxy_ = std::make_unique<StorageDaemon::StorageDaemonProxy>(remote_->AsObject());
    }

    void TearDown() override
    {
        GTEST_LOG_(INFO) << "StorageDaemonProxyTest TearDown";
        proxy_.reset();
        remote_ = nullptr;
        MockMessageParcel::messageParcel = nullptr;
        messageParcelMock_ = nullptr;
    }

protected:
    std::shared_ptr<MessageParcelMock> messageParcelMock_;
    sptr<StorageDaemonMock> remote_;
    std::unique_ptr<StorageDaemon::StorageDaemonProxy> proxy_;
};

/**
 * @tc.name: QueryUsbIsInUse_TestCase_001
 * @tc.desc: QueryUsbIsInUse: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, QueryUsbIsInUse_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    bool isInUse = false;
    int32_t ret = proxy_->QueryUsbIsInUse("/dev/block/sda", isInUse);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);

    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_001 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_002
 * @tc.desc: QueryUsbIsInUse: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, QueryUsbIsInUse_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    bool isInUse = false;
    int32_t ret = proxy_->QueryUsbIsInUse("/dev/block/sda", isInUse);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_002 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_003
 * @tc.desc: QueryUsbIsInUse: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, QueryUsbIsInUse_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *remote_,
        SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::COMMAND_QUERY_USB_IS_IN_USE), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    bool isInUse = false;
    int32_t ret = proxy_->QueryUsbIsInUse("/dev/block/sda", isInUse);
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_003 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_004
 * @tc.desc: QueryUsbIsInUse: reply ReadInt32 is not ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, QueryUsbIsInUse_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(REMOTE_FAILED));
    bool isInUse = false;
    int32_t ret = proxy_->QueryUsbIsInUse("/dev/block/sda", isInUse);
    EXPECT_EQ(ret, REMOTE_FAILED);
    EXPECT_FALSE(isInUse);

    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_004 End";
}

/**
 * @tc.name: QueryUsbIsInUse_TestCase_005
 * @tc.desc: QueryUsbIsInUse: success path reads bool payload.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, QueryUsbIsInUse_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadBool()).WillOnce(Return(true));
    bool isInUse = false;
    int32_t ret = proxy_->QueryUsbIsInUse("/dev/block/sda", isInUse);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_TRUE(isInUse);

    GTEST_LOG_(INFO) << "QueryUsbIsInUse_TestCase_005 End";
}

/**
 * @tc.name: MountUsbFuse_TestCase_001
 * @tc.desc: MountUsbFuse: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, MountUsbFuse_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    std::string fsUuid;
    int fuseFd = -1;
    int32_t ret = proxy_->MountUsbFuse("vol-1", fsUuid, fuseFd);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);

    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_001 End";
}

/**
 * @tc.name: MountUsbFuse_TestCase_002
 * @tc.desc: MountUsbFuse: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, MountUsbFuse_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    std::string fsUuid;
    int fuseFd = -1;
    int32_t ret = proxy_->MountUsbFuse("vol-1", fsUuid, fuseFd);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_002 End";
}

/**
 * @tc.name: MountUsbFuse_TestCase_003
 * @tc.desc: MountUsbFuse: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, MountUsbFuse_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *remote_,
        SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::COMMAND_MOUNT_USB_FUSE), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    std::string fsUuid;
    int fuseFd = -1;
    int32_t ret = proxy_->MountUsbFuse("vol-1", fsUuid, fuseFd);
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_003 End";
}

/**
 * @tc.name: MountUsbFuse_TestCase_004
 * @tc.desc: MountUsbFuse: reply ReadInt32 is not ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, MountUsbFuse_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(REMOTE_FAILED));
    std::string fsUuid;
    int fuseFd = -1;
    int32_t ret = proxy_->MountUsbFuse("vol-1", fsUuid, fuseFd);
    EXPECT_EQ(ret, REMOTE_FAILED);

    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_004 End";
}

/**
 * @tc.name: MountUsbFuse_TestCase_005
 * @tc.desc: MountUsbFuse: success path reads uuid and fd.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, MountUsbFuse_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadString16()).WillOnce(Return(u"uuid-x"));
    EXPECT_CALL(*messageParcelMock_, ReadFileDescriptor()).WillOnce(Return(88));
    std::string fsUuid;
    int fuseFd = -1;
    int32_t ret = proxy_->MountUsbFuse("vol-1", fsUuid, fuseFd);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(fsUuid, "uuid-x");
    EXPECT_EQ(fuseFd, 88);

    GTEST_LOG_(INFO) << "MountUsbFuse_TestCase_005 End";
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_001
 * @tc.desc: CreateBlockDeviceNode: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, CreateBlockDeviceNode_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    int32_t ret = proxy_->CreateBlockDeviceNode("/dev/block/x", 0600, 1, 2);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);

    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_001 End";
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_002
 * @tc.desc: CreateBlockDeviceNode: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, CreateBlockDeviceNode_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    int32_t ret = proxy_->CreateBlockDeviceNode("/dev/block/x", 0600, 1, 2);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_002 End";
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_003
 * @tc.desc: CreateBlockDeviceNode: WriteUint32 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, CreateBlockDeviceNode_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteUint32(_)).WillOnce(Return(false));
    int32_t ret = proxy_->CreateBlockDeviceNode("/dev/block/x", 0600, 1, 2);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_003 End";
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_004
 * @tc.desc: CreateBlockDeviceNode: first WriteInt32 (major) returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, CreateBlockDeviceNode_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteUint32(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(false));
    int32_t ret = proxy_->CreateBlockDeviceNode("/dev/block/x", 0600, 1, 2);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_004 End";
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_005
 * @tc.desc: CreateBlockDeviceNode: second WriteInt32 (minor) returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, CreateBlockDeviceNode_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteUint32(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).WillOnce(Return(true)).WillOnce(Return(false));
    int32_t ret = proxy_->CreateBlockDeviceNode("/dev/block/x", 0600, 1, 2);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_005 End";
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_006
 * @tc.desc: CreateBlockDeviceNode: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, CreateBlockDeviceNode_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_006 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteUint32(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_CREATE_BLOCK_DEVICE_NODE),
                            _, _, _))
        .WillOnce(Return(IPC_FAILED));
    int32_t ret = proxy_->CreateBlockDeviceNode("/dev/block/x", 0600, 1, 2);
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_006 End";
}

/**
 * @tc.name: CreateBlockDeviceNode_TestCase_007
 * @tc.desc: CreateBlockDeviceNode: success returns reply ReadInt32.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, CreateBlockDeviceNode_TestCase_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_007 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteUint32(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteInt32(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_CREATE_BLOCK_DEVICE_NODE),
                            _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    int32_t ret = proxy_->CreateBlockDeviceNode("/dev/block/x", 0600, 1, 2);
    EXPECT_EQ(ret, ERR_OK);

    GTEST_LOG_(INFO) << "CreateBlockDeviceNode_TestCase_007 End";
}

/**
 * @tc.name: DestroyBlockDeviceNode_TestCase_001
 * @tc.desc: DestroyBlockDeviceNode: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, DestroyBlockDeviceNode_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    int32_t ret = proxy_->DestroyBlockDeviceNode("/dev/block/x");
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);

    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_001 End";
}

/**
 * @tc.name: DestroyBlockDeviceNode_TestCase_002
 * @tc.desc: DestroyBlockDeviceNode: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, DestroyBlockDeviceNode_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    int32_t ret = proxy_->DestroyBlockDeviceNode("/dev/block/x");
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_002 End";
}

/**
 * @tc.name: DestroyBlockDeviceNode_TestCase_003
 * @tc.desc: DestroyBlockDeviceNode: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, DestroyBlockDeviceNode_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(static_cast<uint32_t>(
                                          StorageDaemon::IStorageDaemonIpcCode::ADDON_DESTROY_BLOCK_DEVICE_NODE),
                                      _, _, _))
        .WillOnce(Return(IPC_FAILED));
    int32_t ret = proxy_->DestroyBlockDeviceNode("/dev/block/x");
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_003 End";
}

/**
 * @tc.name: DestroyBlockDeviceNode_TestCase_004
 * @tc.desc: DestroyBlockDeviceNode: success returns reply ReadInt32.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, DestroyBlockDeviceNode_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(static_cast<uint32_t>(
                                          StorageDaemon::IStorageDaemonIpcCode::ADDON_DESTROY_BLOCK_DEVICE_NODE),
                                      _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    int32_t ret = proxy_->DestroyBlockDeviceNode("/dev/block/x");
    EXPECT_EQ(ret, ERR_OK);

    GTEST_LOG_(INFO) << "DestroyBlockDeviceNode_TestCase_004 End";
}

/**
 * @tc.name: ReadPartitionTable_TestCase_001
 * @tc.desc: ReadPartitionTable: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadPartitionTable_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    std::string output = "x";
    int32_t maxVolume = -1;
    int32_t ret = proxy_->ReadPartitionTable("/dev/block/sda", output, maxVolume);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);
    EXPECT_TRUE(output.empty());
    EXPECT_EQ(maxVolume, 0);

    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_001 End";
}

/**
 * @tc.name: ReadPartitionTable_TestCase_002
 * @tc.desc: ReadPartitionTable: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadPartitionTable_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    std::string output;
    int32_t maxVolume = -1;
    int32_t ret = proxy_->ReadPartitionTable("/dev/block/sda", output, maxVolume);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_002 End";
}

/**
 * @tc.name: ReadPartitionTable_TestCase_003
 * @tc.desc: ReadPartitionTable: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadPartitionTable_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *remote_,
        SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_READ_PARTITION_TABLE), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    std::string output;
    int32_t maxVolume = -1;
    int32_t ret = proxy_->ReadPartitionTable("/dev/block/sda", output, maxVolume);
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_003 End";
}

/**
 * @tc.name: ReadPartitionTable_TestCase_004
 * @tc.desc: ReadPartitionTable: first ReadInt32 is not ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadPartitionTable_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(REMOTE_FAILED));
    std::string output;
    int32_t maxVolume = -1;
    int32_t ret = proxy_->ReadPartitionTable("/dev/block/sda", output, maxVolume);
    EXPECT_EQ(ret, REMOTE_FAILED);

    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_004 End";
}

/**
 * @tc.name: ReadPartitionTable_TestCase_005
 * @tc.desc: ReadPartitionTable: success reads string and max volume.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadPartitionTable_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *remote_,
        SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_READ_PARTITION_TABLE), _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK)).WillOnce(Return(7));
    EXPECT_CALL(*messageParcelMock_, ReadString16()).WillOnce(Return(u"ptable"));
    std::string output;
    int32_t maxVolume = -1;
    int32_t ret = proxy_->ReadPartitionTable("/dev/block/sda", output, maxVolume);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(output, "ptable");
    EXPECT_EQ(maxVolume, 7);

    GTEST_LOG_(INFO) << "ReadPartitionTable_TestCase_005 End";
}

/**
 * @tc.name: ReadVolumeMetaData_TestCase_001
 * @tc.desc: ReadVolumeMetaData: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadVolumeMetaData_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    std::string fsUuid = "keep";
    std::string fsType = "keep";
    std::string fsLabel = "keep";
    int32_t ret = proxy_->ReadVolumeMetaData("/dev/block/sda1", fsUuid, fsType, fsLabel);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);
    EXPECT_TRUE(fsUuid.empty());
    EXPECT_TRUE(fsType.empty());
    EXPECT_TRUE(fsLabel.empty());

    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_001 End";
}

/**
 * @tc.name: ReadVolumeMetaData_TestCase_002
 * @tc.desc: ReadVolumeMetaData: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadVolumeMetaData_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    std::string fsUuid;
    std::string fsType;
    std::string fsLabel;
    int32_t ret = proxy_->ReadVolumeMetaData("/dev/block/sda1", fsUuid, fsType, fsLabel);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_002 End";
}

/**
 * @tc.name: ReadVolumeMetaData_TestCase_003
 * @tc.desc: ReadVolumeMetaData: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadVolumeMetaData_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *remote_,
        SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_READ_VOLUME_META_DATA), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    std::string fsUuid;
    std::string fsType;
    std::string fsLabel;
    int32_t ret = proxy_->ReadVolumeMetaData("/dev/block/sda1", fsUuid, fsType, fsLabel);
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_003 End";
}

/**
 * @tc.name: ReadVolumeMetaData_TestCase_004
 * @tc.desc: ReadVolumeMetaData: first ReadInt32 is not ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadVolumeMetaData_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(REMOTE_FAILED));
    std::string fsUuid;
    std::string fsType;
    std::string fsLabel;
    int32_t ret = proxy_->ReadVolumeMetaData("/dev/block/sda1", fsUuid, fsType, fsLabel);
    EXPECT_EQ(ret, REMOTE_FAILED);

    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_004 End";
}

/**
 * @tc.name: ReadVolumeMetaData_TestCase_005
 * @tc.desc: ReadVolumeMetaData: success reads uuid, type and label.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, ReadVolumeMetaData_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(
        *remote_,
        SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_READ_VOLUME_META_DATA), _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadString16())
        .WillOnce(Return(u"u-1"))
        .WillOnce(Return(u"ext4"))
        .WillOnce(Return(u"label-1"));
    std::string fsUuid;
    std::string fsType;
    std::string fsLabel;
    int32_t ret = proxy_->ReadVolumeMetaData("/dev/block/sda1", fsUuid, fsType, fsLabel);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(fsUuid, "u-1");
    EXPECT_EQ(fsType, "ext4");
    EXPECT_EQ(fsLabel, "label-1");

    GTEST_LOG_(INFO) << "ReadVolumeMetaData_TestCase_005 End";
}

/**
 * @tc.name: Eject_TestCase_001
 * @tc.desc: Eject: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Eject_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Eject_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    int32_t ret = proxy_->Eject("/dev/cdrom");
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);

    GTEST_LOG_(INFO) << "Eject_TestCase_001 End";
}

/**
 * @tc.name: Eject_TestCase_002
 * @tc.desc: Eject: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Eject_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Eject_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    int32_t ret = proxy_->Eject("/dev/cdrom");
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "Eject_TestCase_002 End";
}

/**
 * @tc.name: Eject_TestCase_003
 * @tc.desc: Eject: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Eject_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Eject_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_EJECT), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    int32_t ret = proxy_->Eject("/dev/cdrom");
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "Eject_TestCase_003 End";
}

/**
 * @tc.name: Eject_TestCase_004
 * @tc.desc: Eject: success returns reply ReadInt32.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Eject_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Eject_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_EJECT), _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    int32_t ret = proxy_->Eject("/dev/cdrom");
    EXPECT_EQ(ret, ERR_OK);

    GTEST_LOG_(INFO) << "Eject_TestCase_004 End";
}

/**
 * @tc.name: GetCDStatus_TestCase_001
 * @tc.desc: GetCDStatus: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, GetCDStatus_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    int32_t status = -1;
    int32_t ret = proxy_->GetCDStatus("/dev/cdrom", status);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);
    EXPECT_EQ(status, 0);

    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_001 End";
}

/**
 * @tc.name: GetCDStatus_TestCase_002
 * @tc.desc: GetCDStatus: WriteString16 returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, GetCDStatus_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    int32_t status = -1;
    int32_t ret = proxy_->GetCDStatus("/dev/cdrom", status);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_002 End";
}

/**
 * @tc.name: GetCDStatus_TestCase_003
 * @tc.desc: GetCDStatus: SendRequest returns value other than ERR_NONE.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, GetCDStatus_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_GET_CD_STATUS), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    int32_t status = -1;
    int32_t ret = proxy_->GetCDStatus("/dev/cdrom", status);
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_003 End";
}

/**
 * @tc.name: GetCDStatus_TestCase_004
 * @tc.desc: GetCDStatus: first ReadInt32 is not ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, GetCDStatus_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_, SendRequest(_, _, _, _)).WillOnce(Return(ERR_NONE));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(REMOTE_FAILED));
    int32_t status = -1;
    int32_t ret = proxy_->GetCDStatus("/dev/cdrom", status);
    EXPECT_EQ(ret, REMOTE_FAILED);

    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_004 End";
}

/**
 * @tc.name: GetCDStatus_TestCase_005
 * @tc.desc: GetCDStatus: success reads status after remote ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, GetCDStatus_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_GET_CD_STATUS), _, _, _))
        .WillOnce(Return(ERR_NONE));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK)).WillOnce(Return(3));
    int32_t status = -1;
    int32_t ret = proxy_->GetCDStatus("/dev/cdrom", status);
    EXPECT_EQ(ret, ERR_OK);
    EXPECT_EQ(status, 3);

    GTEST_LOG_(INFO) << "GetCDStatus_TestCase_005 End";
}

/**
 * @tc.name: Mount_TestCase_001
 * @tc.desc: Mount: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);

    GTEST_LOG_(INFO) << "Mount_TestCase_001 End";
}

/**
 * @tc.name: Mount_TestCase_002
 * @tc.desc: Mount: first WriteString16 (devPath) returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "Mount_TestCase_002 End";
}

/**
 * @tc.name: Mount_TestCase_003
 * @tc.desc: Mount: second WriteString16 (mountPath) returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true)).WillOnce(Return(false));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "Mount_TestCase_003 End";
}

/**
 * @tc.name: Mount_TestCase_004
 * @tc.desc: Mount: third WriteString16 (fsType) returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "Mount_TestCase_004 End";
}

/**
 * @tc.name: Mount_TestCase_005
 * @tc.desc: Mount: fourth WriteString16 (mountData) returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "Mount_TestCase_005 End";
}

/**
 * @tc.name: Mount_TestCase_006
 * @tc.desc: Mount: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_006 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_MOUNT), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "Mount_TestCase_006 End";
}

/**
 * @tc.name: Mount_TestCase_007
 * @tc.desc: Mount: returns reply ReadInt32 value (remote code propagated).
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_007 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_MOUNT), _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(REMOTE_FAILED));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, REMOTE_FAILED);

    GTEST_LOG_(INFO) << "Mount_TestCase_007 End";
}

/**
 * @tc.name: Mount_TestCase_008
 * @tc.desc: Mount: success returns ERR_OK from reply.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Mount_TestCase_008, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_008 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_MOUNT), _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    int32_t ret = proxy_->Mount("/dev/block/sda1", "/mnt/usb", "vfat", "rw");
    EXPECT_EQ(ret, ERR_OK);

    GTEST_LOG_(INFO) << "Mount_TestCase_008 End";
}

/**
 * @tc.name: Unmount_TestCase_001
 * @tc.desc: Unmount: WriteInterfaceToken returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Unmount_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_001 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(false));
    int32_t ret = proxy_->Unmount("/mnt/usb", true);
    EXPECT_EQ(ret, ERR_TRANSACTION_FAILED);

    GTEST_LOG_(INFO) << "Unmount_TestCase_001 End";
}

/**
 * @tc.name: Unmount_TestCase_002
 * @tc.desc: Unmount: WriteString16 (mountPath) returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Unmount_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_002 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(false));
    int32_t ret = proxy_->Unmount("/mnt/usb", true);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "Unmount_TestCase_002 End";
}

/**
 * @tc.name: Unmount_TestCase_003
 * @tc.desc: Unmount: WriteBool returns false.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Unmount_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_003 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteBool(_)).WillOnce(Return(false));
    int32_t ret = proxy_->Unmount("/mnt/usb", true);
    EXPECT_EQ(ret, ERR_INVALID_DATA);

    GTEST_LOG_(INFO) << "Unmount_TestCase_003 End";
}

/**
 * @tc.name: Unmount_TestCase_004
 * @tc.desc: Unmount: SendRequest returns non-ERR_OK.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Unmount_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_004 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteBool(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_UNMOUNT), _, _, _))
        .WillOnce(Return(IPC_FAILED));
    int32_t ret = proxy_->Unmount("/mnt/usb", true);
    EXPECT_EQ(ret, IPC_FAILED);

    GTEST_LOG_(INFO) << "Unmount_TestCase_004 End";
}

/**
 * @tc.name: Unmount_TestCase_005
 * @tc.desc: Unmount: success returns reply ReadInt32.
 * @tc.type: FUNC
 */
HWTEST_F(StorageDaemonProxyTest, Unmount_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_005 Start";

    EXPECT_CALL(*messageParcelMock_, WriteInterfaceToken(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteString16(_)).WillOnce(Return(true));
    EXPECT_CALL(*messageParcelMock_, WriteBool(_)).WillOnce(Return(true));
    EXPECT_CALL(*remote_,
                SendRequest(static_cast<uint32_t>(StorageDaemon::IStorageDaemonIpcCode::ADDON_UNMOUNT), _, _, _))
        .WillOnce(Return(ERR_OK));
    EXPECT_CALL(*messageParcelMock_, ReadInt32()).WillOnce(Return(ERR_OK));
    int32_t ret = proxy_->Unmount("/mnt/usb", true);
    EXPECT_EQ(ret, ERR_OK);

    GTEST_LOG_(INFO) << "Unmount_TestCase_005 End";
}

} // namespace DiskManager
} // namespace OHOS
