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

#include "volume_core.h"
#include "message_parcel_mock.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class VolumeCoreTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(VolumeCoreTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    VolumeCore vc;
    EXPECT_EQ(vc.GetId(), "");
    EXPECT_EQ(vc.GetType(), 0);
    EXPECT_EQ(vc.GetDiskId(), "");
    EXPECT_EQ(vc.GetState(), UNMOUNTED);
    EXPECT_EQ(vc.GetFsType(), "");
}

HWTEST_F(VolumeCoreTest, Constructor3Param_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("vol-1", EXTERNAL, "disk-1");
    EXPECT_EQ(vc.GetId(), "vol-1");
    EXPECT_EQ(vc.GetType(), EXTERNAL);
    EXPECT_EQ(vc.GetDiskId(), "disk-1");
    EXPECT_EQ(vc.GetState(), UNMOUNTED);
}

HWTEST_F(VolumeCoreTest, Constructor4Param_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("vol-2", EMULATED, "disk-2", MOUNTED);
    EXPECT_EQ(vc.GetId(), "vol-2");
    EXPECT_EQ(vc.GetType(), EMULATED);
    EXPECT_EQ(vc.GetDiskId(), "disk-2");
    EXPECT_EQ(vc.GetState(), MOUNTED);
}

HWTEST_F(VolumeCoreTest, Constructor5Param_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("vol-3", EXTERNAL, "disk-3", CHECKING, "vfat");
    EXPECT_EQ(vc.GetId(), "vol-3");
    EXPECT_EQ(vc.GetType(), EXTERNAL);
    EXPECT_EQ(vc.GetDiskId(), "disk-3");
    EXPECT_EQ(vc.GetState(), CHECKING);
    EXPECT_EQ(vc.GetFsType(), "vfat");
}

HWTEST_F(VolumeCoreTest, GetId_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("test-id", 0, "");
    EXPECT_EQ(vc.GetId(), "test-id");
}

HWTEST_F(VolumeCoreTest, GetType_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("v", EMULATED, "d");
    EXPECT_EQ(vc.GetType(), EMULATED);
}

HWTEST_F(VolumeCoreTest, GetDiskId_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("v", 0, "my-disk");
    EXPECT_EQ(vc.GetDiskId(), "my-disk");
}

HWTEST_F(VolumeCoreTest, GetState_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("v", 0, "d", EJECTING);
    EXPECT_EQ(vc.GetState(), EJECTING);
}

HWTEST_F(VolumeCoreTest, SetState_TestCase_001, TestSize.Level0)
{
    VolumeCore vc;
    vc.SetState(MOUNTED);
    EXPECT_EQ(vc.GetState(), MOUNTED);
    vc.SetState(UNMOUNTED);
    EXPECT_EQ(vc.GetState(), UNMOUNTED);
}

HWTEST_F(VolumeCoreTest, GetFsType_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("v", 0, "d", 0, "ext4");
    EXPECT_EQ(vc.GetFsType(), "ext4");
}

HWTEST_F(VolumeCoreTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("vol-1", EXTERNAL, "disk-1", MOUNTED, "vfat");
    Parcel parcel;
    EXPECT_TRUE(vc.Marshalling(parcel));
}

HWTEST_F(VolumeCoreTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("vol-1", EMULATED, "disk-1", MOUNTED, "ext4");
    Parcel parcel;
    EXPECT_TRUE(vc.Marshalling(parcel));
    VolumeCore *result = VolumeCore::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetId(), "vol-1");
    EXPECT_EQ(result->GetType(), EMULATED);
    EXPECT_EQ(result->GetDiskId(), "disk-1");
    EXPECT_EQ(result->GetState(), MOUNTED);
    EXPECT_EQ(result->GetFsType(), "ext4");
    delete result;
}

class VolumeInfoStrTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(VolumeInfoStrTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    VolumeInfoStr vis;
    EXPECT_EQ(vis.volumeId, "");
    EXPECT_EQ(vis.fsTypeStr, "");
    EXPECT_EQ(vis.fsUuid, "");
    EXPECT_EQ(vis.path, "");
    EXPECT_EQ(vis.description, "");
    EXPECT_FALSE(vis.isDamaged);
}

HWTEST_F(VolumeInfoStrTest, ParameterizedConstructor_TestCase_001, TestSize.Level0)
{
    VolumeInfoStr vis("vol-1", "vfat", "uuid-1", "/mnt", "desc", true);
    EXPECT_EQ(vis.volumeId, "vol-1");
    EXPECT_EQ(vis.fsTypeStr, "vfat");
    EXPECT_EQ(vis.fsUuid, "uuid-1");
    EXPECT_EQ(vis.path, "/mnt");
    EXPECT_EQ(vis.description, "desc");
    EXPECT_TRUE(vis.isDamaged);
}

HWTEST_F(VolumeInfoStrTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    VolumeInfoStr vis("vol-1", "ext4", "uuid-1", "/data", "data", false);
    Parcel parcel;
    EXPECT_TRUE(vis.Marshalling(parcel));
}

HWTEST_F(VolumeInfoStrTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    VolumeInfoStr vis("vol-1", "ext4", "uuid-1", "/data", "data", true);
    Parcel parcel;
    EXPECT_TRUE(vis.Marshalling(parcel));
    VolumeInfoStr *result = VolumeInfoStr::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->volumeId, "vol-1");
    EXPECT_EQ(result->fsTypeStr, "ext4");
    EXPECT_EQ(result->fsUuid, "uuid-1");
    EXPECT_EQ(result->path, "/data");
    EXPECT_EQ(result->description, "data");
    EXPECT_TRUE(result->isDamaged);
    delete result;
}

} // namespace DiskManager
} // namespace OHOS