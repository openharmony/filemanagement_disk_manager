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

#include "volume_external.h"
#include "message_parcel_mock.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class VolumeExternalTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(VolumeExternalTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    EXPECT_EQ(ve.GetId(), "");
    EXPECT_EQ(ve.GetType(), 0);
    EXPECT_EQ(ve.GetDiskId(), "");
    EXPECT_EQ(ve.GetState(), UNMOUNTED);
    EXPECT_EQ(ve.GetFsType(), UNDEFINED);
    EXPECT_EQ(ve.GetFlags(), DISK_TYPE_UNKNOWN);
    EXPECT_EQ(ve.GetUuid(), "");
    EXPECT_EQ(ve.GetPath(), "");
    EXPECT_EQ(ve.GetDescription(), "");
    EXPECT_EQ(ve.GetExtraInfo(), "");
    EXPECT_EQ(ve.GetPartitionNum(), 0);
    EXPECT_EQ(ve.GetFreeSize(), 0);
    EXPECT_FALSE(ve.GetUserData());
}

HWTEST_F(VolumeExternalTest, ConstructorFromVolumeCore_TestCase_001, TestSize.Level0)
{
    VolumeCore vc("vol-1", EXTERNAL, "disk-1", MOUNTED, "vfat");
    VolumeExternal ve(vc);
    EXPECT_EQ(ve.GetId(), "vol-1");
    EXPECT_EQ(ve.GetType(), EXTERNAL);
    EXPECT_EQ(ve.GetDiskId(), "disk-1");
    EXPECT_EQ(ve.GetState(), MOUNTED);
    EXPECT_EQ(ve.GetFsType(), UNDEFINED);
}

HWTEST_F(VolumeExternalTest, SetFlags_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFlags(1);
    EXPECT_EQ(ve.GetFlags(), 1);
}

HWTEST_F(VolumeExternalTest, SetFsType_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(VFAT);
    EXPECT_EQ(ve.GetFsType(), VFAT);
}

HWTEST_F(VolumeExternalTest, SetFsUuid_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsUuid("uuid-123");
    EXPECT_EQ(ve.GetUuid(), "uuid-123");
}

HWTEST_F(VolumeExternalTest, SetPath_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetPath("/mnt/media");
    EXPECT_EQ(ve.GetPath(), "/mnt/media");
}

HWTEST_F(VolumeExternalTest, SetDescription_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetDescription("USB storage");
    EXPECT_EQ(ve.GetDescription(), "USB storage");
}

HWTEST_F(VolumeExternalTest, SetUserData_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    EXPECT_FALSE(ve.GetUserData());
    ve.SetUserData(true);
    EXPECT_TRUE(ve.GetUserData());
    ve.SetUserData(false);
    EXPECT_FALSE(ve.GetUserData());
}

HWTEST_F(VolumeExternalTest, SetFreeSize_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFreeSize(1024);
    EXPECT_EQ(ve.GetFreeSize(), 1024);
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Ntfs_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(NTFS);
    EXPECT_EQ(ve.GetFsTypeString(), "ntfs");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Exfat_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(EXFAT);
    EXPECT_EQ(ve.GetFsTypeString(), "exfat");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Vfat_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(VFAT);
    EXPECT_EQ(ve.GetFsTypeString(), "vfat");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Hmfs_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(HMFS);
    EXPECT_EQ(ve.GetFsTypeString(), "hmfs");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_F2fs_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(F2FS);
    EXPECT_EQ(ve.GetFsTypeString(), "f2fs");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Mtp_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(MTP);
    EXPECT_EQ(ve.GetFsTypeString(), "mtp");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Udf_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(UDF);
    EXPECT_EQ(ve.GetFsTypeString(), "udf");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Iso9660_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(ISO9660);
    EXPECT_EQ(ve.GetFsTypeString(), "iso9660");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Ptp_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(PTP);
    EXPECT_EQ(ve.GetFsTypeString(), "ptp");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Ext4_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(EXT4);
    EXPECT_EQ(ve.GetFsTypeString(), "ext4");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_Undefined_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(UNDEFINED);
    EXPECT_EQ(ve.GetFsTypeString(), "undefined");
}

HWTEST_F(VolumeExternalTest, GetFsTypeString_UnknownValue_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFsType(999);
    EXPECT_EQ(ve.GetFsTypeString(), "undefined");
}

HWTEST_F(VolumeExternalTest, GetFsTypeByStr_Ntfs_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    EXPECT_EQ(ve.GetFsTypeByStr("ntfs"), NTFS);
}

HWTEST_F(VolumeExternalTest, GetFsTypeByStr_Vfat_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    EXPECT_EQ(ve.GetFsTypeByStr("vfat"), VFAT);
}

HWTEST_F(VolumeExternalTest, GetFsTypeByStr_Ext4_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    EXPECT_EQ(ve.GetFsTypeByStr("ext4"), EXT4);
}

HWTEST_F(VolumeExternalTest, GetFsTypeByStr_Undefined_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    EXPECT_EQ(ve.GetFsTypeByStr("unknown"), -1);
}

HWTEST_F(VolumeExternalTest, GetFsTypeByStr_Empty_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    EXPECT_EQ(ve.GetFsTypeByStr(""), -1);
}

HWTEST_F(VolumeExternalTest, Reset_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetPath("/mnt/data");
    EXPECT_EQ(ve.GetPath(), "/mnt/data");
    ve.Reset();
    EXPECT_EQ(ve.GetPath(), "");
}

HWTEST_F(VolumeExternalTest, SetExtraInfo_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetExtraInfo("extra-info");
    EXPECT_EQ(ve.GetExtraInfo(), "extra-info");
}

HWTEST_F(VolumeExternalTest, SetPartitionNum_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetPartitionNum(3);
    EXPECT_EQ(ve.GetPartitionNum(), 3);
}

HWTEST_F(VolumeExternalTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFlags(1);
    ve.SetFsType(VFAT);
    ve.SetFsUuid("uuid-1");
    ve.SetPath("/mnt");
    ve.SetDescription("desc");
    ve.SetExtraInfo("extra");
    ve.SetPartitionNum(2);
    Parcel parcel;
    EXPECT_TRUE(ve.Marshalling(parcel));
}

HWTEST_F(VolumeExternalTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    VolumeExternal ve;
    ve.SetFlags(1);
    ve.SetFsType(VFAT);
    ve.SetFsUuid("uuid-1");
    ve.SetPath("/mnt");
    ve.SetDescription("desc");
    ve.SetExtraInfo("extra");
    ve.SetPartitionNum(2);
    Parcel parcel;
    EXPECT_TRUE(ve.Marshalling(parcel));
    VolumeExternal *result = VolumeExternal::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetFlags(), 1);
    EXPECT_EQ(result->GetFsType(), VFAT);
    EXPECT_EQ(result->GetUuid(), "uuid-1");
    EXPECT_EQ(result->GetPath(), "/mnt");
    EXPECT_EQ(result->GetDescription(), "desc");
    EXPECT_EQ(result->GetExtraInfo(), "extra");
    EXPECT_EQ(result->GetPartitionNum(), 2);
    delete result;
}
} // namespace DiskManager
} // namespace OHOS