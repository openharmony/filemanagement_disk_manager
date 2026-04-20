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

#include "disk.h"
#include "disk_manager_client.h"
#include "disk_manager_errno.h"
#include "disk_manager_napi_errno.h"
#include "volume_external.h"

namespace OHOS {
namespace DiskManager {

using namespace testing::ext;

namespace {
constexpr const char *TEST_VOLUME_ID = "vol-test-001";
constexpr const char *TEST_DISK_ID = "disk-test-001";
constexpr const char *TEST_FS_UUID = "uuid-test-001";
constexpr const char *TEST_FS_TYPE = "vfat";
constexpr const char *TEST_DESCRIPTION = "test-volume";
constexpr int32_t TEST_PARTITION_TYPE = 0;
} // namespace

class DiskManagerClientTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GTEST_LOG_(INFO) << "DiskManagerClientTest SetUpTestCase";
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "DiskManagerClientTest TearDownTestCase";
    }

    void SetUp() override
    {
        GTEST_LOG_(INFO) << "DiskManagerClientTest SetUp";
    }

    void TearDown() override
    {
        GTEST_LOG_(INFO) << "DiskManagerClientTest TearDown";
    }
};

/**
 * @tc.name: ResetProxyTest001
 * @tc.desc: 测试 ResetProxy 方法正常调用，返回 E_OK。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, ResetProxyTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ResetProxyTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    int32_t ret = client.ResetProxy();
    EXPECT_EQ(ret, E_OK);

    GTEST_LOG_(INFO) << "ResetProxyTest001 End";
}

/**
 * @tc.name: ResetProxyTest002
 * @tc.desc: 测试多次调用 ResetProxy，每次应返回 E_OK。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, ResetProxyTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ResetProxyTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    for (int i = 0; i < 3; ++i) {
        int32_t ret = client.ResetProxy();
        EXPECT_EQ(ret, E_OK);
    }

    GTEST_LOG_(INFO) << "ResetProxyTest002 End";
}

/**
 * @tc.name: MountTest001
 * @tc.desc: 测试 Mount 方法传入空 volumeId，预期返回连接错误码（服务不可达）。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, MountTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "MountTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Mount("");
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "MountTest001 End";
}

/**
 * @tc.name: MountTest002
 * @tc.desc: 测试 Mount 方法传入有效 volumeId，无系统服务时预期返回 E_SA_IS_NULLPTR。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, MountTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "MountTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Mount(TEST_VOLUME_ID);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "MountTest002 End";
}

/**
 * @tc.name: UnmountTest001
 * @tc.desc: 测试 Unmount 方法传入空 volumeId，预期返回连接错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, UnmountTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnmountTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Unmount("");
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "UnmountTest001 End";
}

/**
 * @tc.name: UnmountTest002
 * @tc.desc: 测试 Unmount 方法传入有效 volumeId，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, UnmountTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnmountTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Unmount(TEST_VOLUME_ID);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "UnmountTest002 End";
}

/**
 * @tc.name: FormatTest001
 * @tc.desc: 测试 Format 方法传入空 volumeId，预期返回连接错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, FormatTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FormatTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Format("", TEST_FS_TYPE);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "FormatTest001 End";
}

/**
 * @tc.name: FormatTest002
 * @tc.desc: 测试 Format 方法传入有效参数，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, FormatTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FormatTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Format(TEST_VOLUME_ID, TEST_FS_TYPE);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "FormatTest002 End";
}

/**
 * @tc.name: FormatTest003
 * @tc.desc: 测试 Format 方法传入空 fsType，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, FormatTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "FormatTest003 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Format(TEST_VOLUME_ID, "");
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "FormatTest003 End";
}

/**
 * @tc.name: SetVolumeDescriptionTest001
 * @tc.desc: 测试 SetVolumeDescription 方法传入空 uuid，预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, SetVolumeDescriptionTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetVolumeDescriptionTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.SetVolumeDescription("", TEST_DESCRIPTION);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "SetVolumeDescriptionTest001 End";
}

/**
 * @tc.name: SetVolumeDescriptionTest002
 * @tc.desc: 测试 SetVolumeDescription 方法传入有效参数，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, SetVolumeDescriptionTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetVolumeDescriptionTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.SetVolumeDescription(TEST_FS_UUID, TEST_DESCRIPTION);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "SetVolumeDescriptionTest002 End";
}

/**
 * @tc.name: GetAllVolumesTest001
 * @tc.desc: 测试 GetAllVolumes 方法
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetAllVolumesTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetAllVolumesTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    std::vector<VolumeExternal> volumes;
    int32_t ret = client.GetAllVolumes(volumes);
    EXPECT_EQ(ret, E_OK);
    EXPECT_TRUE(volumes.empty());

    GTEST_LOG_(INFO) << "GetAllVolumesTest001 End";
}

/**
 * @tc.name: GetVolumeByUuidTest001
 * @tc.desc: 测试 GetVolumeByUuid 方法传入空 uuid，预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetVolumeByUuidTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetVolumeByUuidTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    VolumeExternal vol;
    int32_t ret = client.GetVolumeByUuid("", vol);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "GetVolumeByUuidTest001 End";
}

/**
 * @tc.name: GetVolumeByUuidTest002
 * @tc.desc: 测试 GetVolumeByUuid 方法传入有效 uuid，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetVolumeByUuidTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetVolumeByUuidTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    VolumeExternal vol;
    int32_t ret = client.GetVolumeByUuid(TEST_FS_UUID, vol);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "GetVolumeByUuidTest002 End";
}

/**
 * @tc.name: GetVolumeByIdTest001
 * @tc.desc: 测试 GetVolumeById 方法传入空 volumeId，预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetVolumeByIdTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetVolumeByIdTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    VolumeExternal vol;
    int32_t ret = client.GetVolumeById("", vol);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "GetVolumeByIdTest001 End";
}

/**
 * @tc.name: GetVolumeByIdTest002
 * @tc.desc: 测试 GetVolumeById 方法传入有效 volumeId，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetVolumeByIdTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetVolumeByIdTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    VolumeExternal vol;
    int32_t ret = client.GetVolumeById(TEST_VOLUME_ID, vol);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "GetVolumeByIdTest002 End";
}

/**
 * @tc.name: GetFreeSizeOfVolumeTest001
 * @tc.desc: 测试 GetFreeSizeOfVolume 方法传入空 uuid
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetFreeSizeOfVolumeTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolumeTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int64_t freeSize = 0;
    int32_t ret = client.GetFreeSizeOfVolume("", freeSize);
    EXPECT_EQ(ret, E_OK);

    GTEST_LOG_(INFO) << "GetFreeSizeOfVolumeTest001 End";
}

/**
 * @tc.name: GetFreeSizeOfVolumeTest002
 * @tc.desc: 测试 GetFreeSizeOfVolume 方法传入有效 uuid
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetFreeSizeOfVolumeTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolumeTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int64_t freeSize = 0;
    int32_t ret = client.GetFreeSizeOfVolume(TEST_FS_UUID, freeSize);
    EXPECT_EQ(ret, E_OK);

    GTEST_LOG_(INFO) << "GetFreeSizeOfVolumeTest002 End";
}

/**
 * @tc.name: GetTotalSizeOfVolumeTest001
 * @tc.desc: 测试 GetTotalSizeOfVolume 方法传入空 uuid
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetTotalSizeOfVolumeTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolumeTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int64_t totalSize = 0;
    int32_t ret = client.GetTotalSizeOfVolume("", totalSize);
    EXPECT_EQ(ret, E_OK);

    GTEST_LOG_(INFO) << "GetTotalSizeOfVolumeTest001 End";
}

/**
 * @tc.name: GetTotalSizeOfVolumeTest002
 * @tc.desc: 测试 GetTotalSizeOfVolume 方法传入有效 uuid
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, GetTotalSizeOfVolumeTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolumeTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int64_t totalSize = 0;
    int32_t ret = client.GetTotalSizeOfVolume(TEST_FS_UUID, totalSize);
    EXPECT_EQ(ret, E_OK);

    GTEST_LOG_(INFO) << "GetTotalSizeOfVolumeTest002 End";
}

/**
 * @tc.name: PartitionTest001
 * @tc.desc: 测试 Partition 方法传入空 diskId，预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, PartitionTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PartitionTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Partition("", TEST_PARTITION_TYPE);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "PartitionTest001 End";
}

/**
 * @tc.name: PartitionTest002
 * @tc.desc: 测试 Partition 方法传入有效 diskId，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, PartitionTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PartitionTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.Partition(TEST_DISK_ID, TEST_PARTITION_TYPE);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "PartitionTest002 End";
}

/**
 * @tc.name: PartitionTest003
 * @tc.desc: 测试 Partition 方法传入不同分区类型参数。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, PartitionTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PartitionTest003 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();

    int32_t types[] = {0, 1, 2};
    for (int32_t type : types) {
        int32_t ret = client.Partition(TEST_DISK_ID, type);
        EXPECT_NE(ret, E_OK);
    }

    GTEST_LOG_(INFO) << "PartitionTest003 End";
}

/**
 * @tc.name: OnBlockDiskUeventTest001
 * @tc.desc: 测试 OnBlockDiskUevent 方法传入空消息，预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, OnBlockDiskUeventTest001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnBlockDiskUeventTest001 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    int32_t ret = client.OnBlockDiskUevent("");
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "OnBlockDiskUeventTest001 End";
}

/**
 * @tc.name: OnBlockDiskUeventTest002
 * @tc.desc: 测试 OnBlockDiskUevent 方法传入有效 uevent 消息，无系统服务时预期返回错误码。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, OnBlockDiskUeventTest002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnBlockDiskUeventTest002 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();
    std::string ueventMsg = "ACTION=add\nDEVTYPE=disk\nDISK=test-disk\n";
    int32_t ret = client.OnBlockDiskUevent(ueventMsg);
    EXPECT_NE(ret, E_OK);

    GTEST_LOG_(INFO) << "OnBlockDiskUeventTest002 End";
}

/**
 * @tc.name: OnBlockDiskUeventTest003
 * @tc.desc: 测试 OnBlockDiskUevent 方法传入不同格式消息。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerClientTest, OnBlockDiskUeventTest003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "OnBlockDiskUeventTest003 Start";

    DiskManagerClient &client = *DelayedSingleton<DiskManagerClient>::GetInstance();
    client.ResetProxy();

    std::vector<std::string> messages = {
        "ACTION=add",
        "ACTION=remove\nDEVTYPE=disk",
        "INVALID_MESSAGE",
    };
    for (const auto &msg : messages) {
        int32_t ret = client.OnBlockDiskUevent(msg);
        EXPECT_NE(ret, E_OK);
    }

    GTEST_LOG_(INFO) << "OnBlockDiskUeventTest003 End";
}
} // namespace DiskManager
} // namespace OHOS