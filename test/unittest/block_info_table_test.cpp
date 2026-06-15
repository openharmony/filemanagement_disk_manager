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

#include "block_info_table.h"
#include "mock_storage_daemon_adapter.h"
#include "disk_manager_errno.h"
#include "ipc_types.h"

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using namespace OHOS::DiskManager;
using namespace testing;
using namespace testing::ext;

static const char *K_VALID_DISK_JSON =
    R"([{"diskId":"disk-8-1","sizeBytes":1073741824,"vendor":"Samsung","model":"SSD 870",)"
    R"("devnum":"1","busnum":"2","devNode":"sda","scsiBusNum":"0","fwVersion":"1.0",)"
    R"("interfaceType":"SATA","rpm":0,"removable":false,"serialNumber":"SN123",)"
    R"("devicePath":"/dev/sda","port":"0"}])";

static const char *K_VALID_DISK_OBJ_JSON =
    R"({"diskId":"disk-8-1","sizeBytes":1073741824,"vendor":"Samsung","model":"SSD 870",)"
    R"("interfaceType":"SATA","rpm":0,"removable":false,"serialNumber":"SN123",)"
    R"("devicePath":"/dev/sda","port":"0"})";

static const char *K_BLOCKS_WRAPPER_JSON =
    R"({"blocks":[{"diskId":"disk-8-1","sizeBytes":500,"vendor":"V"},)"
    R"({"diskId":"disk-8-2","sizeBytes":600,"vendor":"W"}]})";

class BlockInfoTableTest : public Test {
public:
    static void SetUpTestCase()
    {
        testing::Mock::AllowLeak(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::AllowLeak(&BlockInfoTable::GetInstance());
    }
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(BlockInfoTableTest, GetInstance_TestCase_001, TestSize.Level0)
{
    auto &inst1 = BlockInfoTable::GetInstance();
    auto &inst2 = BlockInfoTable::GetInstance();
    EXPECT_EQ(&inst1, &inst2);
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_RPCError_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(Return(-1));
    EXPECT_NE(table.ReloadFromDaemon(), E_OK);
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_EmptyResponse_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string emptyJson = "";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(emptyJson), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_InvalidJson_TestCase_003, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string badJson = "{invalid json}";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(badJson), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), OHOS::ERR_INVALID_DATA);
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_ArrayJson_TestCase_004, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = K_VALID_DISK_JSON;
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("disk-8-1", info));
    EXPECT_EQ(info.sizeBytes, UINT64_C(1073741824));
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_ObjectJson_TestCase_005, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = K_VALID_DISK_OBJ_JSON;
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("disk-8-1", info));
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_NullJson_TestCase_006, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = "null";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_FALSE(table.TryCopyByDiskId("disk-8-1", info));
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_BlocksWrapper_TestCase_007, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = K_BLOCKS_WRAPPER_JSON;
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info1;
    EXPECT_TRUE(table.TryCopyByDiskId("disk-8-1", info1));
    BlockInfo info2;
    EXPECT_TRUE(table.TryCopyByDiskId("disk-8-2", info2));
}

HWTEST_F(BlockInfoTableTest, ReloadFromDaemon_NonObjectNonArray_TestCase_008, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = "42";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_FALSE(table.TryCopyByDiskId("any", info));
}

HWTEST_F(BlockInfoTableTest, TryCopyByDiskId_NotFound_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    BlockInfo info;
    EXPECT_FALSE(table.TryCopyByDiskId("nonexistent", info));
}

HWTEST_F(BlockInfoTableTest, TryCopyByDiskId_Found_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = K_VALID_DISK_JSON;
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    table.ReloadFromDaemon();
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("disk-8-1", info));
    EXPECT_EQ(info.vendor, "Samsung");
}

HWTEST_F(BlockInfoTableTest, ToJsonStringWithExtras_NoExtras_TestCase_001, TestSize.Level0)
{
    BlockInfo info {};
    info.diskId = "disk-8-1";
    info.sizeBytes = 1024;
    info.vendor = "V";
    std::string result = BlockInfoTable::ToJsonStringWithExtras(info);
    EXPECT_NE(result.find("\"diskId\":\"disk-8-1\""), std::string::npos);
    EXPECT_NE(result.find("\"sizeBytes\":1024"), std::string::npos);
}

HWTEST_F(BlockInfoTableTest, ToJsonStringWithExtras_WithExtras_TestCase_002, TestSize.Level0)
{
    BlockInfo info {};
    info.diskId = "disk-8-1";
    info.sizeBytes = 2048;
    std::unordered_map<std::string, std::string> extras = {{std::string("extraKey1"), std::string("val1")},
                                                            {std::string("extraKey2"), std::string("val2")}};
    std::string result = BlockInfoTable::ToJsonStringWithExtras(info, extras);
    EXPECT_NE(result.find("\"extraKey1\":\"val1\""), std::string::npos);
    EXPECT_NE(result.find("\"extraKey2\":\"val2\""), std::string::npos);
}

HWTEST_F(BlockInfoTableTest, ToJsonStringWithExtras_ODDInfo_TestCase_003, TestSize.Level0)
{
    BlockInfo info {};
    info.diskId = "disk-8-1";
    info.ODD_INFO = nlohmann::json{{std::string("cdrom"), true}};
    std::string result = BlockInfoTable::ToJsonStringWithExtras(info);
    EXPECT_NE(result.find("\"ODD_INFO\""), std::string::npos);
}

HWTEST_F(BlockInfoTableTest, ReadExtDiskInfoFromDaemon_RPCError_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    BlockInfo info {};
    info.diskId = "disk-8-1";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(Return(-1));
    EXPECT_NE(table.ReadExtDiskInfoFromDaemon("sda", info), E_OK);
}

HWTEST_F(BlockInfoTableTest, ReadExtDiskInfoFromDaemon_InvalidJson_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    BlockInfo info {};
    info.diskId = "disk-8-1";
    std::string badJson = "{bad}";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(badJson), Return(E_OK)));
    EXPECT_EQ(table.ReadExtDiskInfoFromDaemon("sda", info), OHOS::ERR_INVALID_DATA);
}

HWTEST_F(BlockInfoTableTest, ReadExtDiskInfoFromDaemon_EmptyResponse_TestCase_003, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    BlockInfo info {};
    info.diskId = "disk-8-1";
    std::string emptyStr = "";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(emptyStr), Return(E_OK)));
    EXPECT_EQ(table.ReadExtDiskInfoFromDaemon("sda", info), E_OK);
}

HWTEST_F(BlockInfoTableTest, ReadExtDiskInfoFromDaemon_Success_TestCase_004, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    BlockInfo info {};
    info.diskId = "disk-8-1";
    std::string jsonStr = K_VALID_DISK_OBJ_JSON;
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReadExtDiskInfoFromDaemon("sda", info), E_OK);
    EXPECT_EQ(info.sizeBytes, UINT64_C(1073741824));
}

HWTEST_F(BlockInfoTableTest, ReadExtDiskInfoFromDaemon_ArrayEmptyMap_TestCase_005, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    BlockInfo info {};
    info.diskId = "disk-8-999";
    std::string jsonStr = K_VALID_DISK_JSON;
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReadExtDiskInfoFromDaemon("sda", info), E_OK);
}

HWTEST_F(BlockInfoTableTest, FillBlockInfo_NonObject_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = "42";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    table.ReloadFromDaemon();
    BlockInfo info;
    EXPECT_FALSE(table.TryCopyByDiskId("any", info));
}

HWTEST_F(BlockInfoTableTest, FillBlockInfo_EmptyDiskId_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"sizeBytes":100,"vendor":"V"}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    table.ReloadFromDaemon();
    BlockInfo info;
    EXPECT_FALSE(table.TryCopyByDiskId("any", info));
}

HWTEST_F(BlockInfoTableTest, FillBlockInfo_ODDInfo_TestCase_003, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"disk-8-1","ODD_INFO":{"cdrom":true}}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("disk-8-1", info));
    EXPECT_TRUE(info.ODD_INFO.contains("cdrom"));
}

HWTEST_F(BlockInfoTableTest, FillBlockInfo_NullODDInfo_TestCase_004, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"disk-8-1","ODD_INFO":null}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("disk-8-1", info));
}

HWTEST_F(BlockInfoTableTest, ReadUInt64Unsigned_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","sizeBytes":4294967296}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.sizeBytes, UINT64_C(4294967296));
}

HWTEST_F(BlockInfoTableTest, ReadUInt64Negative_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","sizeBytes":-5}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.sizeBytes, UINT64_C(0));
}

HWTEST_F(BlockInfoTableTest, ReadUInt64Null_TestCase_003, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","sizeBytes":null}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.sizeBytes, UINT64_C(0));
}

HWTEST_F(BlockInfoTableTest, ReadUInt64StringType_TestCase_004, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","sizeBytes":"abc"}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.sizeBytes, UINT64_C(0));
}

HWTEST_F(BlockInfoTableTest, ReadUInt32Overflow_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","rpm":4294967297}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.rpm, UINT32_C(0));
}

HWTEST_F(BlockInfoTableTest, ReadUInt32Normal_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","rpm":5400}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.rpm, UINT32_C(5400));
}

HWTEST_F(BlockInfoTableTest, ReadBoolLikeBooleanTrue_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","removable":true}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_TRUE(info.removable);
}

HWTEST_F(BlockInfoTableTest, ReadBoolLikeBooleanFalse_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","removable":false}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_FALSE(info.removable);
}

HWTEST_F(BlockInfoTableTest, ReadBoolLikeIntegerNonZero_TestCase_003, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","removable":1}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_TRUE(info.removable);
}

HWTEST_F(BlockInfoTableTest, ReadBoolLikeIntegerZero_TestCase_004, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","removable":0}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_FALSE(info.removable);
}

HWTEST_F(BlockInfoTableTest, ReadBoolLikeNull_TestCase_005, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","removable":null}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_FALSE(info.removable);
}

HWTEST_F(BlockInfoTableTest, ReadStringOrEmpty_MissingKey_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1"}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.vendor, "");
}

HWTEST_F(BlockInfoTableTest, ReadStringOrEmpty_Null_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","vendor":null}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.vendor, "");
}

HWTEST_F(BlockInfoTableTest, ReadStringOrEmpty_NonString_TestCase_003, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","vendor":42}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.vendor, "");
}

HWTEST_F(BlockInfoTableTest, ReadStringOrEmpty_TrimsSpaces_TestCase_004, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","vendor":"  Samsung  "}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.vendor, "Samsung");
}

HWTEST_F(BlockInfoTableTest, UpsertTruncated_TestCase_001, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string bigJson = "[";
    for (size_t i = 0; i < 25; ++i) {
        bigJson += "{\"diskId\":\"d-" + std::to_string(i) + "\",\"sizeBytes\":" + std::to_string(i) + "},";
    }
    bigJson.pop_back();
    bigJson += "]";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(bigJson), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
}

HWTEST_F(BlockInfoTableTest, UpsertArrayNonObjectElement_TestCase_002, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([42,"hello",{"diskId":"d1","sizeBytes":100}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
}

HWTEST_F(BlockInfoTableTest, UpsertDuplicateDiskId_TestCase_003, TestSize.Level0)
{
    auto &table = BlockInfoTable::GetInstance();
    auto &mockAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string jsonStr = R"([{"diskId":"d1","sizeBytes":100},{"diskId":"d1","sizeBytes":200}])";
    EXPECT_CALL(mockAdapter, GetBlockInfoByTypeImpl(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(jsonStr), Return(E_OK)));
    EXPECT_EQ(table.ReloadFromDaemon(), E_OK);
    BlockInfo info;
    EXPECT_TRUE(table.TryCopyByDiskId("d1", info));
    EXPECT_EQ(info.sizeBytes, UINT64_C(200));
}