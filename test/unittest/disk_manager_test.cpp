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
#include <chrono>
#include <cstring>
#include <securec.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <thread>

#define private public
#define protected public
#include "disk_manager.h"
#undef private
#undef protected

#include "disk.h"
#include "volume_external.h"
#include "volume_core.h"
#include "partition_types.h"
#include "storage_spec_models.h"
#include "disk_manager_errno.h"
#include "disk_manager_napi_errno.h"
#include "errors.h"
#include "mock_storage_daemon_adapter.h"
#include "mock_usb_fuse_adapter.h"
#include "mock_parameter_find.h"
#include "notification/common_event_publisher.h"
#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

namespace {
const int64_t SIZE = 4096;
const uint32_t VALUESIZE = 5;

Disk MakeUsbDisk(const std::string &diskId = "disk-ut-1")
{
    return Disk(diskId, SIZE, "/dev/block/" + diskId, USB_FLAG);
}

Disk MakeSdDisk(const std::string &diskId = "disk-sd-1")
{
    return Disk(diskId, SIZE, "/dev/block/" + diskId, SD_FLAG);
}

Disk MakeCdDisk(const std::string &diskId = "disk-cd-1")
{
    return Disk(diskId, SIZE, "/dev/block/" + diskId, CD_FLAG);
}

Disk MakeSsdDisk(const std::string &diskId = "disk-ssd-1")
{
    Disk d(diskId, SIZE, "/dev/block/" + diskId, DATA_DISK_SSD);
    d.SetDiskType(DATA_DISK_SSD);
    return d;
}

Disk MakeHddDisk(const std::string &diskId = "disk-hdd-1")
{
    Disk d(diskId, SIZE, "/dev/block/" + diskId, DATA_DISK_HDD);
    d.SetDiskType(DATA_DISK_HDD);
    return d;
}

Disk MakeDvrDisk(const std::string &diskId = "disk-dvr-1")
{
    Disk d(diskId, SIZE, "/dev/block/" + diskId, DVR_USB);
    d.SetDiskType(DVR_USB);
    return d;
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

VolumeExternal MakeMtpVolume(const std::string &volId = "vol-mtp-1",
                             const std::string &diskId = "disk-ut-1",
                             const std::string &fsUuid = "uuid-mtp-1")
{
    VolumeCore core(volId, EXTERNAL, diskId, UNMOUNTED);
    VolumeExternal v(core);
    v.SetFlags(USB_FLAG);
    v.SetFsType(static_cast<int32_t>(MTP));
    v.SetFsUuid(fsUuid);
    return v;
}

VolumeExternal MakeUdfVolume(const std::string &volId = "vol-udf-1",
                             const std::string &diskId = "disk-cd-1",
                             const std::string &fsUuid = "uuid-udf-1")
{
    VolumeCore core(volId, EXTERNAL, diskId, MOUNTED);
    VolumeExternal v(core);
    v.SetFlags(CD_FLAG);
    v.SetFsType(static_cast<int32_t>(UDF));
    v.SetFsUuid(fsUuid);
    v.SetPath("/mnt/data/external/" + fsUuid);
    return v;
}

void ClearDiskManagerState()
{
    g_mockFindParameterResult = 1;
    strcpy_s(g_mockParameterValue, sizeof(g_mockParameterValue), "false");
    g_mockGetParameterValueResult = VALUESIZE;
    auto &dm = DiskManager::GetInstance();
    dm.partitionTableMap_.clear();
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

class DiskManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        testing::Mock::AllowLeak(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::AllowLeak(&MockUsbFuseAdapter::GetInstance());
        GTEST_LOG_(INFO) << "DiskManagerTest SetUpTestCase";
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "DiskManagerTest TearDownTestCase";
    }

    void SetUp() override
    {
        ClearDiskManagerState();
        testing::Mock::VerifyAndClearExpectations(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::VerifyAndClearExpectations(&MockUsbFuseAdapter::GetInstance());
        GTEST_LOG_(INFO) << "DiskManagerTest SetUp";
    }

    void TearDown() override
    {
        ClearDiskManagerState();
        GTEST_LOG_(INFO) << "DiskManagerTest TearDown";
    }
};

/**
 * @tc.name: OnDiskCreated_TestCase_001
 * @tc.desc: Normal disk creation returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnDiskCreated_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnDiskCreated_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    Disk disk = MakeUsbDisk();
    EXPECT_EQ(dm.OnDiskCreated(disk), DiskManagerErrNo::E_OK);
    EXPECT_TRUE(dm.HasDisk(disk.GetDiskId()));
    GTEST_LOG_(INFO) << "OnDiskCreated_TestCase_001 End";
}

/**
 * @tc.name: OnDiskCreated_TestCase_002
 * @tc.desc: Duplicate disk creation returns E_DISK_HAS_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnDiskCreated_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnDiskCreated_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    Disk disk = MakeUsbDisk();
    EXPECT_EQ(dm.OnDiskCreated(disk), DiskManagerErrNo::E_OK);
    EXPECT_EQ(dm.OnDiskCreated(disk), DiskManagerErrNo::E_DISK_HAS_EXIST);
    GTEST_LOG_(INFO) << "OnDiskCreated_TestCase_002 End";
}

/**
 * @tc.name: OnDiskCreated_TestCase_003
 * @tc.desc: Create different disk types (SD, CD, SSD, HDD).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnDiskCreated_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnDiskCreated_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.OnDiskCreated(MakeSdDisk("disk-sd-a")), DiskManagerErrNo::E_OK);
    EXPECT_EQ(dm.OnDiskCreated(MakeCdDisk("disk-cd-a")), DiskManagerErrNo::E_OK);
    EXPECT_EQ(dm.OnDiskCreated(MakeSsdDisk("disk-ssd-a")), DiskManagerErrNo::E_OK);
    EXPECT_EQ(dm.OnDiskCreated(MakeHddDisk("disk-hdd-a")), DiskManagerErrNo::E_OK);
    EXPECT_TRUE(dm.HasDisk("disk-sd-a"));
    EXPECT_TRUE(dm.HasDisk("disk-cd-a"));
    EXPECT_TRUE(dm.HasDisk("disk-ssd-a"));
    EXPECT_TRUE(dm.HasDisk("disk-hdd-a"));
    GTEST_LOG_(INFO) << "OnDiskCreated_TestCase_003 End";
}

/**
 * @tc.name: HasDisk_TestCase_001
 * @tc.desc: HasDisk returns true for existing disk.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, HasDisk_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "HasDisk_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    Disk disk = MakeUsbDisk();
    dm.OnDiskCreated(disk);
    EXPECT_TRUE(dm.HasDisk(disk.GetDiskId()));
    GTEST_LOG_(INFO) << "HasDisk_TestCase_001 End";
}

/**
 * @tc.name: HasDisk_TestCase_002
 * @tc.desc: HasDisk returns false for non-existent disk.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, HasDisk_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "HasDisk_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_FALSE(dm.HasDisk("nonexistent-disk"));
    GTEST_LOG_(INFO) << "HasDisk_TestCase_002 End";
}

/**
 * @tc.name: OnDiskDestroyed_TestCase_001
 * @tc.desc: Destroy existing disk returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnDiskDestroyed_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnDiskDestroyed_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    Disk disk = MakeUsbDisk();
    dm.OnDiskCreated(disk);
    EXPECT_EQ(dm.OnDiskDestroyed(disk.GetDiskId()), DiskManagerErrNo::E_OK);
    EXPECT_FALSE(dm.HasDisk(disk.GetDiskId()));
    GTEST_LOG_(INFO) << "OnDiskDestroyed_TestCase_001 End";
}

/**
 * @tc.name: OnDiskDestroyed_TestCase_002
 * @tc.desc: Destroy non-existent disk returns E_DISK_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnDiskDestroyed_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnDiskDestroyed_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.OnDiskDestroyed("nonexistent-disk"), E_DISK_NOT_FOUND);
    GTEST_LOG_(INFO) << "OnDiskDestroyed_TestCase_002 End";
}

/**
 * @tc.name: GetDiskById_TestCase_001
 * @tc.desc: Get existing disk by id returns E_OK with volumeIds.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiskById_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    Disk disk = MakeUsbDisk("disk-gbi-1");
    dm.OnDiskCreated(disk);
    VolumeExternal vol = MakeUsbVolume("vol-gbi-1", "disk-gbi-1", "uuid-gbi-1");
    dm.OnVolumeCreated(vol);
    Disk out;
    EXPECT_EQ(dm.GetDiskById("disk-gbi-1", out), DiskManagerErrNo::E_OK);
    EXPECT_EQ(out.GetDiskId(), "disk-gbi-1");
    EXPECT_EQ(out.GetDiskType(), USB_FLAG);
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_001 End";
}

/**
 * @tc.name: GetDiskById_TestCase_002
 * @tc.desc: Get non-existent disk returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiskById_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    Disk out;
    EXPECT_EQ(dm.GetDiskById("nonexistent-disk", out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_002 End";
}

/**
 * @tc.name: GetAllDisks_TestCase_001
 * @tc.desc: GetAllDisks with empty map returns E_OK with empty vector.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetAllDisks_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    std::vector<Disk> out;
    EXPECT_EQ(dm.GetAllDisks(out), DiskManagerErrNo::E_OK);
    EXPECT_EQ(out.size(), static_cast<size_t>(0));
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_001 End";
}

/**
 * @tc.name: GetAllDisks_TestCase_002
 * @tc.desc: GetAllDisks with multiple disks returns all disks.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetAllDisks_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ad-1"));
    dm.OnDiskCreated(MakeSdDisk("disk-ad-2"));
    std::vector<Disk> out;
    EXPECT_EQ(dm.GetAllDisks(out), DiskManagerErrNo::E_OK);
    EXPECT_EQ(out.size(), static_cast<size_t>(2));
    GTEST_LOG_(INFO) << "GetAllDisks_TestCase_002 End";
}

/**
 * @tc.name: OnVolumeCreated_TestCase_001
 * @tc.desc: Normal volume creation returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnVolumeCreated_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnVolumeCreated_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-vc-1"));
    VolumeExternal vol = MakeUsbVolume("vol-vc-1", "disk-vc-1");
    EXPECT_EQ(dm.OnVolumeCreated(vol), DiskManagerErrNo::E_OK);
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeById("vol-vc-1", out), E_OK);
    EXPECT_EQ(out.GetId(), "vol-vc-1");
    EXPECT_EQ(out.GetFlags(), USB_FLAG);
    GTEST_LOG_(INFO) << "OnVolumeCreated_TestCase_001 End";
}

/**
 * @tc.name: OnVolumeCreated_TestCase_002
 * @tc.desc: Volume creation with various disk types sets correct flags.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnVolumeCreated_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnVolumeCreated_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSdDisk("disk-vc-sd"));
    dm.OnDiskCreated(MakeCdDisk("disk-vc-cd"));
    dm.OnDiskCreated(MakeSsdDisk("disk-vc-ssd"));
    VolumeExternal volSd = MakeUsbVolume("vol-vc-sd", "disk-vc-sd");
    VolumeExternal volCd = MakeUsbVolume("vol-vc-cd", "disk-vc-cd");
    VolumeExternal volSsd = MakeUsbVolume("vol-vc-ssd", "disk-vc-ssd");
    EXPECT_EQ(dm.OnVolumeCreated(volSd), E_OK);
    EXPECT_EQ(dm.OnVolumeCreated(volCd), E_OK);
    EXPECT_EQ(dm.OnVolumeCreated(volSsd), E_OK);
    VolumeExternal outSd, outCd, outSsd;
    dm.GetVolumeById("vol-vc-sd", outSd);
    dm.GetVolumeById("vol-vc-cd", outCd);
    dm.GetVolumeById("vol-vc-ssd", outSsd);
    EXPECT_EQ(outSd.GetFlags(), SD_FLAG);
    EXPECT_EQ(outCd.GetFlags(), CD_FLAG);
    EXPECT_EQ(outSsd.GetFlags(), DATA_DISK_SSD);
    GTEST_LOG_(INFO) << "OnVolumeCreated_TestCase_002 End";
}

/**
 * @tc.name: OnVolumeDestroyed_TestCase_001
 * @tc.desc: Destroy existing volume returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnVolumeDestroyed_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnVolumeDestroyed_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk());
    VolumeExternal vol = MakeUsbVolume();
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.OnVolumeDestroyed(vol.GetId()), E_OK);
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeById(vol.GetId(), out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "OnVolumeDestroyed_TestCase_001 End";
}

/**
 * @tc.name: OnVolumeDestroyed_TestCase_002
 * @tc.desc: Destroy non-existent volume returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, OnVolumeDestroyed_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "OnVolumeDestroyed_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.OnVolumeDestroyed("nonexistent-vol"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "OnVolumeDestroyed_TestCase_002 End";
}

/**
 * @tc.name: GetVolumeById_TestCase_001
 * @tc.desc: Get existing volume returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetVolumeById_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeById_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-vbi-1"));
    VolumeExternal vol = MakeUsbVolume("vol-vbi-1", "disk-vbi-1", "uuid-vbi-1");
    dm.OnVolumeCreated(vol);
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeById("vol-vbi-1", out), E_OK);
    EXPECT_EQ(out.GetId(), "vol-vbi-1");
    EXPECT_EQ(out.GetUuid(), "uuid-vbi-1");
    GTEST_LOG_(INFO) << "GetVolumeById_TestCase_001 End";
}

/**
 * @tc.name: GetVolumeById_TestCase_002
 * @tc.desc: Get non-existent volume returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetVolumeById_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeById_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeById("nonexistent-vol", out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetVolumeById_TestCase_002 End";
}

/**
 * @tc.name: GetVolumeByUuid_TestCase_001
 * @tc.desc: Get volume by existing UUID returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetVolumeByUuid_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeByUuid_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-vbu-1"));
    VolumeExternal vol = MakeUsbVolume("vol-vbu-1", "disk-vbu-1", "uuid-vbu-1");
    dm.OnVolumeCreated(vol);
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeByUuid("uuid-vbu-1", out), E_OK);
    EXPECT_EQ(out.GetId(), "vol-vbu-1");
    GTEST_LOG_(INFO) << "GetVolumeByUuid_TestCase_001 End";
}

/**
 * @tc.name: GetVolumeByUuid_TestCase_002
 * @tc.desc: Get volume by non-existent UUID returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetVolumeByUuid_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeByUuid_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeByUuid("nonexistent-uuid", out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetVolumeByUuid_TestCase_002 End";
}

/**
 * @tc.name: GetAllVolumes_TestCase_001
 * @tc.desc: GetAllVolumes with empty map returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetAllVolumes_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    std::vector<VolumeExternal> out;
    EXPECT_EQ(dm.GetAllVolumes(out), E_OK);
    EXPECT_EQ(out.size(), static_cast<size_t>(0));
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_001 End";
}

/**
 * @tc.name: GetAllVolumes_TestCase_002
 * @tc.desc: GetAllVolumes with normal volumes returns all volumes.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetAllVolumes_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-av-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-av-1", "disk-av-1", "uuid-av-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-av-2", "disk-av-1", "uuid-av-2"));
    std::vector<VolumeExternal> out;
    EXPECT_EQ(dm.GetAllVolumes(out), E_OK);
    EXPECT_EQ(out.size(), static_cast<size_t>(2));
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_002 End";
}

/**
 * @tc.name: GetAllVolumes_TestCase_003
 * @tc.desc: GetAllVolumes with CD disk without UDF volume creates virtual DVD.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetAllVolumes_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-cd-av"));
    std::vector<VolumeExternal> out;
    EXPECT_EQ(dm.GetAllVolumes(out), E_OK);
    EXPECT_GE(out.size(), static_cast<size_t>(1));
    bool foundDvd = false;
    for (auto &v : out) {
        if (v.GetDescription() == "DVD RW") {
            foundDvd = true;
        }
    }
    EXPECT_TRUE(foundDvd);
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_003 End";
}

/**
 * @tc.name: GetAllVolumes_TestCase_004
 * @tc.desc: GetAllVolumes with CD disk and existing UDF volume - no virtual DVD duplication.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetAllVolumes_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_004 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-cd-av2"));
    dm.OnVolumeCreated(MakeUdfVolume("vol-udf-av2", "disk-cd-av2", "uuid-udf-av2"));
    std::vector<VolumeExternal> out;
    EXPECT_EQ(dm.GetAllVolumes(out), E_OK);
    EXPECT_GE(out.size(), static_cast<size_t>(1));
    GTEST_LOG_(INFO) << "GetAllVolumes_TestCase_004 End";
}

/**
 * @tc.name: UpdateVolumeMetadata_TestCase_001
 * @tc.desc: Update existing volume metadata returns E_OK.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, UpdateVolumeMetadata_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "UpdateVolumeMetadata_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uv-1"));
    VolumeExternal vol = MakeUsbVolume("vol-uv-1", "disk-uv-1", "uuid-old");
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.UpdateVolumeMetadata("vol-uv-1", "uuid-new", "ntfs", "NewDesc"), E_OK);
    VolumeExternal out;
    dm.GetVolumeById("vol-uv-1", out);
    EXPECT_EQ(out.GetUuid(), "uuid-new");
    EXPECT_EQ(out.GetDescription(), "NewDesc");
    GTEST_LOG_(INFO) << "UpdateVolumeMetadata_TestCase_001 End";
}

/**
 * @tc.name: UpdateVolumeMetadata_TestCase_002
 * @tc.desc: Update non-existent volume metadata returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, UpdateVolumeMetadata_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "UpdateVolumeMetadata_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.UpdateVolumeMetadata("nonexistent-vol", "uuid", "vfat", "desc"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "UpdateVolumeMetadata_TestCase_002 End";
}

/**
 * @tc.name: Mount_TestCase_001
 * @tc.desc: Mount non-existent volume returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Mount_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.Mount("nonexistent-vol"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "Mount_TestCase_001 End";
}

/**
 * @tc.name: Mount_TestCase_002
 * @tc.desc: Mount volume in MOUNTED state returns E_VOL_MOUNT_ERR.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Mount_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-2"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-2", "disk-mt-2", "uuid-mt-2", MOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.Mount("vol-mt-2"), E_VOL_MOUNT_ERR);
    GTEST_LOG_(INFO) << "Mount_TestCase_002 End";
}

/**
 * @tc.name: Mount_TestCase_003
 * @tc.desc: Mount volume in UNMOUNTED state with Mount success.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Mount_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-3"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-3", "disk-mt-3", "uuid-mt-3", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Mount("vol-mt-3"), E_OK);
    GTEST_LOG_(INFO) << "Mount_TestCase_003 End";
}

/**
 * @tc.name: Mount_TestCase_004
 * @tc.desc: Mount volume in DECRYPTING state proceeds to mount.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Mount_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_004 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-4"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-4", "disk-mt-4", "uuid-mt-4", DECRYPTING);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Mount("vol-mt-4"), E_OK);
    GTEST_LOG_(INFO) << "Mount_TestCase_004 End";
}

/**
 * @tc.name: Mount_TestCase_005
 * @tc.desc: Mount fails when StorageDaemonAdapter::Mount returns error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Mount_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_005 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-5"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-5", "disk-mt-5", "uuid-mt-5", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.Mount("vol-mt-5"), E_OK);
    GTEST_LOG_(INFO) << "Mount_TestCase_005 End";
}

/**
 * @tc.name: Mount_TestCase_006
 * @tc.desc: Mount volume with unsafe fsUuid (contains ..) returns E_PARAMS_INVALID.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Mount_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Mount_TestCase_006 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-6"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-6", "disk-mt-6", "../unsafe", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.Mount("vol-mt-6"), E_PARAMS_INVALID);
    GTEST_LOG_(INFO) << "Mount_TestCase_006 End";
}

/**
 * @tc.name: Unmount_TestCase_001
 * @tc.desc: Unmount non-existent volume returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Unmount_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.Unmount("nonexistent-vol"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "Unmount_TestCase_001 End";
}

/**
 * @tc.name: Unmount_TestCase_002
 * @tc.desc: Unmount volume in UNMOUNTED state returns E_VOL_UMOUNT_ERR.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Unmount_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-um-2"));
    VolumeExternal vol = MakeUsbVolume("vol-um-2", "disk-um-2", "uuid-um-2", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.Unmount("vol-um-2"), E_VOL_UMOUNT_ERR);
    GTEST_LOG_(INFO) << "Unmount_TestCase_002 End";
}

/**
 * @tc.name: Unmount_TestCase_003
 * @tc.desc: Unmount volume in MOUNTED state with Unmount success.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Unmount_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-um-3"));
    VolumeExternal vol = MakeUsbVolume("vol-um-3", "disk-um-3", "uuid-um-3", MOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Unmount("vol-um-3"), E_OK);
    GTEST_LOG_(INFO) << "Unmount_TestCase_003 End";
}

/**
 * @tc.name: Unmount_TestCase_004
 * @tc.desc: Unmount fails when StorageDaemonAdapter::Unmount returns error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Unmount_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Unmount_TestCase_004 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-um-4"));
    VolumeExternal vol = MakeUsbVolume("vol-um-4", "disk-um-4", "uuid-um-4", MOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.Unmount("vol-um-4"), E_OK);
    GTEST_LOG_(INFO) << "Unmount_TestCase_004 End";
}

/**
 * @tc.name: Format_TestCase_001
 * @tc.desc: Format MTP volume returns E_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Format_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fmt-1"));
    VolumeExternal vol = MakeMtpVolume("vol-fmt-mtp", "disk-fmt-1");
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.Format("vol-fmt-mtp", "vfat"), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "Format_TestCase_001 End";
}

/**
 * @tc.name: Format_TestCase_002
 * @tc.desc: Format volume on CD disk returns E_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Format_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-fmt-cd"));
    VolumeExternal vol = MakeUsbVolume("vol-fmt-cd", "disk-fmt-cd", "uuid-fmt-cd", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.Format("vol-fmt-cd", "vfat"), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "Format_TestCase_002 End";
}

/**
 * @tc.name: Format_TestCase_003
 * @tc.desc: Format volume in MOUNTED state returns E_VOL_STATE.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Format_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fmt-3"));
    VolumeExternal vol = MakeUsbVolume("vol-fmt-3", "disk-fmt-3", "uuid-fmt-3", MOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.Format("vol-fmt-3", "vfat"), E_VOL_STATE);
    GTEST_LOG_(INFO) << "Format_TestCase_003 End";
}

/**
 * @tc.name: Format_TestCase_004
 * @tc.desc: Format non-existent volume returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Format_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_TestCase_004 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.Format("nonexistent-vol", "vfat"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "Format_TestCase_004 End";
}

/**
 * @tc.name: Format_TestCase_005
 * @tc.desc: Format volume in UNMOUNTED state with FormatVolume success.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Format_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_TestCase_005 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fmt-5"));
    VolumeExternal vol = MakeUsbVolume("vol-fmt-5", "disk-fmt-5", "uuid-fmt-5", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, FormatVolume(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, ReadMetadata(_, _, _, _)).WillOnce(DoAll(
        SetArgReferee<1>("new-uuid"), SetArgReferee<2>("vfat"), SetArgReferee<3>("label"), Return(ERR_OK)));
    EXPECT_EQ(dm.Format("vol-fmt-5", "vfat"), E_OK);
    VolumeExternal out;
    dm.GetVolumeById("vol-fmt-5", out);
    EXPECT_EQ(out.GetUuid(), "new-uuid");
    EXPECT_EQ(out.GetDescription(), "label");
    GTEST_LOG_(INFO) << "Format_TestCase_005 End";
}

/**
 * @tc.name: Format_TestCase_006
 * @tc.desc: Format volume with FormatVolume failure returns error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Format_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Format_TestCase_006 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fmt-6"));
    VolumeExternal vol = MakeUsbVolume("vol-fmt-6", "disk-fmt-6", "uuid-fmt-6", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, FormatVolume(_, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.Format("vol-fmt-6", "vfat"), E_OK);
    GTEST_LOG_(INFO) << "Format_TestCase_006 End";
}

/**
 * @tc.name: TryToFix_TestCase_001
 * @tc.desc: TryToFix non-existent volume returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, TryToFix_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TryToFix_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.TryToFix("nonexistent-vol"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "TryToFix_TestCase_001 End";
}

/**
 * @tc.name: TryToFix_TestCase_002
 * @tc.desc: TryToFix volume in UNMOUNTED state with Repair and Mount success.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, TryToFix_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TryToFix_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-tf-2"));
    VolumeExternal vol = MakeUsbVolume("vol-tf-2", "disk-tf-2", "uuid-tf-2", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Repair(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.TryToFix("vol-tf-2"), E_OK);
    GTEST_LOG_(INFO) << "TryToFix_TestCase_002 End";
}

/**
 * @tc.name: TryToFix_TestCase_003
 * @tc.desc: TryToFix volume in MOUNTED state - unmount then repair remount.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, TryToFix_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TryToFix_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-tf-3"));
    VolumeExternal vol = MakeUsbVolume("vol-tf-3", "disk-tf-3", "uuid-tf-3", MOUNTED);
    vol.SetPath("/mnt/data/external/uuid-tf-3");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Repair(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.TryToFix("vol-tf-3"), E_OK);
    GTEST_LOG_(INFO) << "TryToFix_TestCase_003 End";
}

/**
 * @tc.name: TryToFix_TestCase_004
 * @tc.desc: TryToFix with Repair failure returns error.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, TryToFix_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "TryToFix_TestCase_004 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-tf-4"));
    VolumeExternal vol = MakeUsbVolume("vol-tf-4", "disk-tf-4", "uuid-tf-4", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Repair(_, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.TryToFix("vol-tf-4"), E_OK);
    GTEST_LOG_(INFO) << "TryToFix_TestCase_004 End";
}

/**
 * @tc.name: SetVolumeDescription_TestCase_001
 * @tc.desc: SetVolumeDescription with non-existent UUID returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetVolumeDescription_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.SetVolumeDescription("nonexistent-uuid", "desc"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_001 End";
}

/**
 * @tc.name: SetVolumeDescription_TestCase_002
 * @tc.desc: SetVolumeDescription with volume in MOUNTED state returns E_VOL_STATE.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetVolumeDescription_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svd-2"));
    VolumeExternal vol = MakeUsbVolume("vol-svd-2", "disk-svd-2", "uuid-svd-2", MOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.SetVolumeDescription("uuid-svd-2", "desc"), E_VOL_STATE);
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_002 End";
}

/**
 * @tc.name: SetVolumeDescription_TestCase_003
 * @tc.desc: SetVolumeDescription on CD disk returns E_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetVolumeDescription_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-svd-3"));
    VolumeExternal vol = MakeUsbVolume("vol-svd-3", "disk-svd-3", "uuid-svd-3", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.SetVolumeDescription("uuid-svd-3", "desc"), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_003 End";
}

/**
 * @tc.name: SetVolumeDescription_TestCase_004
 * @tc.desc: SetVolumeDescription with unsupported fsType returns E_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetVolumeDescription_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_004 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svd-4"));
    VolumeExternal vol = MakeUsbVolume("vol-svd-4", "disk-svd-4", "uuid-svd-4", UNMOUNTED);
    vol.SetFsType(static_cast<int32_t>(VFAT));
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.SetVolumeDescription("uuid-svd-4", "desc"), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_004 End";
}

/**
 * @tc.name: SetVolumeDescription_TestCase_005
 * @tc.desc: SetVolumeDescription with supported fsType (ntfs) and SetLabel success.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetVolumeDescription_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_005 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svd-5"));
    VolumeExternal vol = MakeUsbVolume("vol-svd-5", "disk-svd-5", "uuid-svd-5", UNMOUNTED);
    vol.SetFsType(static_cast<int32_t>(NTFS));
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, SetLabel(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.SetVolumeDescription("uuid-svd-5", "NewDesc"), E_OK);
    VolumeExternal out;
    dm.GetVolumeById("vol-svd-5", out);
    EXPECT_EQ(out.GetDescription(), "NewDesc");
    GTEST_LOG_(INFO) << "SetVolumeDescription_TestCase_005 End";
}

/**
 * @tc.name: PurgeVolumesForDisk_TestCase_001
 * @tc.desc: PurgeVolumesForDisk non-existent disk returns E_DISK_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, PurgeVolumesForDisk_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PurgeVolumesForDisk_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.PurgeVolumesForDisk("nonexistent-disk"), E_DISK_NOT_FOUND);
    GTEST_LOG_(INFO) << "PurgeVolumesForDisk_TestCase_001 End";
}

/**
 * @tc.name: PurgeVolumesForDisk_TestCase_002
 * @tc.desc: PurgeVolumesForDisk removes all volumes for disk.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, PurgeVolumesForDisk_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PurgeVolumesForDisk_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    Disk disk = MakeUsbDisk("disk-pv-2");
    dm.OnDiskCreated(disk);
    dm.OnVolumeCreated(MakeUsbVolume("vol-pv-2a", "disk-pv-2", "uuid-pv-2a"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-pv-2b", "disk-pv-2", "uuid-pv-2b"));
    EXPECT_EQ(dm.PurgeVolumesForDisk("disk-pv-2"), E_OK);
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeById("vol-pv-2a", out), E_NON_EXIST);
    EXPECT_EQ(dm.GetVolumeById("vol-pv-2b", out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "PurgeVolumesForDisk_TestCase_002 End";
}

/**
 * @tc.name: Partition_TestCase_001
 * @tc.desc: Partition CD disk returns E_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Partition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Partition_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-pt-1"));
    EXPECT_EQ(dm.Partition("disk-pt-1", 0), E_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "Partition_TestCase_002 End";
}

/**
 * @tc.name: Partition_TestCase_002
 * @tc.desc: Partition disk with mounted volume returns E_VOL_STATE.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Partition_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Partition_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-pt-2"));
    VolumeExternal vol = MakeUsbVolume("vol-pt-2", "disk-pt-2", "uuid-pt-2", MOUNTED);
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.Partition("disk-pt-2", 0), E_VOL_STATE);
    GTEST_LOG_(INFO) << "Partition_TestCase_002 End";
}

/**
 * @tc.name: Partition_TestCase_003
 * @tc.desc: Partition disk with all volumes UNMOUNTED succeeds.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Partition_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Partition_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-pt-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-pt-3", "disk-pt-3", "uuid-pt-3", UNMOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Partition(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Partition("disk-pt-3", 0), E_OK);
    GTEST_LOG_(INFO) << "Partition_TestCase_003 End";
}

/**
 * @tc.name: CreateIsoImage_TestCase_001
* @tc.desc: CreateIsoImage with nonexistent volumeId returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, CreateIsoImage_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.CreateIsoImage("nonexistent-vol", "/path"), E_NON_EXIST);
    GTEST_LOG_(INFO) << "CreateIsoImage_TestCase_001 End";
}

/**
 * @tc.name: GetVolumeOpProcess_TestCase_001
 * @tc.desc: GetVolumeOpProcess returns E_OK with progressPct=0.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetVolumeOpProcess_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetVolumeOpProcess_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    int32_t progress = -1;
    EXPECT_EQ(dm.GetVolumeOpProcess("nonexistent-vol", progress), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetVolumeOpProcess_TestCase_001 End";
}

/**
 * @tc.name: VerifyBurnData_TestCase_001
 * @tc.desc: VerifyBurnData returns E_OK (stub).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, VerifyBurnData_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "VerifyBurnData_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.VerifyBurnData("nonexistent-vol", 0), E_NON_EXIST);
    GTEST_LOG_(INFO) << "VerifyBurnData_TestCase_001 End";
}

/**
 * @tc.name: ReplacePartitionsForDisk_TestCase_001
 * @tc.desc: ReplacePartitionsForDisk returns E_OK (stub).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, ReplacePartitionsForDisk_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReplacePartitionsForDisk_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    std::vector<PartitionRecord> records;
    EXPECT_EQ(dm.ReplacePartitionsForDisk("any-disk", records), E_OK);
    GTEST_LOG_(INFO) << "ReplacePartitionsForDisk_TestCase_001 End";
}

/**
 * @tc.name: NotifyMtpMounted_TestCase_001
 * @tc.desc: NotifyMtpMounted with mtpfs fsType sets MTP fs type.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyMtpMounted_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_NO_THROW(dm.NotifyMtpMounted("mtp-id-1", "/mnt/data/external/mtp-uuid", "MTP Device",
                                         "mtp-uuid", "mtpfs"));
    VolumeExternal out;
    std::vector<VolumeExternal> volumes;
    dm.GetAllVolumes(volumes);
    bool found = false;
    for (auto &v : volumes) {
        if (v.GetId() == "mtp-id-1") {
            found = true;
            EXPECT_EQ(v.GetFsType(), static_cast<int32_t>(MTP));
        }
    }
    EXPECT_TRUE(found);
    dm.OnVolumeDestroyed("mtp-id-1");
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_001 End";
}

/**
 * @tc.name: NotifyMtpMounted_TestCase_002
 * @tc.desc: NotifyMtpMounted with gphotofs fsType sets PTP fs type.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyMtpMounted_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_NO_THROW(dm.NotifyMtpMounted("ptp-id-1", "/mnt/data/external/ptp-uuid", "PTP Device",
                                         "ptp-uuid", "gphotofs"));
    std::vector<VolumeExternal> volumes;
    dm.GetAllVolumes(volumes);
    bool found = false;
    for (auto &v : volumes) {
        if (v.GetId() == "ptp-id-1") {
            found = true;
            EXPECT_EQ(v.GetFsType(), static_cast<int32_t>(PTP));
        }
    }
    EXPECT_TRUE(found);
    dm.OnVolumeDestroyed("ptp-id-1");
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_002 End";
}

/**
 * @tc.name: NotifyMtpMounted_TestCase_003
 * @tc.desc: NotifyMtpMounted with unknown fsType does not set fs type.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyMtpMounted_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_NO_THROW(dm.NotifyMtpMounted("unk-id-1", "/mnt/data/external/unk-uuid", "Unknown",
                                         "unk-uuid", "unknownfs"));
    dm.OnVolumeDestroyed("unk-id-1");
    GTEST_LOG_(INFO) << "NotifyMtpMounted_TestCase_003 End";
}

/**
 * @tc.name: NotifyMtpUnmounted_TestCase_001
 * @tc.desc: NotifyMtpUnmounted with isBadRemove=false publishes REMOVED event.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyMtpUnmounted_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    dm.NotifyMtpMounted("mtp-um-1", "/mnt/data/external/mtp-um-1", "MTP", "mtp-um-uuid", "mtpfs");
    EXPECT_NO_THROW(dm.NotifyMtpUnmounted("mtp-um-1", false));
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeById("mtp-um-1", out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_001 End";
}

/**
 * @tc.name: NotifyMtpUnmounted_TestCase_002
 * @tc.desc: NotifyMtpUnmounted with isBadRemove=true publishes BAD_REMOVAL event.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyMtpUnmounted_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.NotifyMtpMounted("mtp-um-2", "/mnt/data/external/mtp-um-2", "MTP", "mtp-um-uuid2", "mtpfs");
    EXPECT_NO_THROW(dm.NotifyMtpUnmounted("mtp-um-2", true));
    VolumeExternal out;
    EXPECT_EQ(dm.GetVolumeById("mtp-um-2", out), E_NON_EXIST);
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_002 End";
}

/**
 * @tc.name: NotifyMtpUnmounted_TestCase_003
 * @tc.desc: NotifyMtpUnmounted with non-existent volumeId logs error but no crash.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyMtpUnmounted_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_NO_THROW(dm.NotifyMtpUnmounted("nonexistent-mtp", false));
    GTEST_LOG_(INFO) << "NotifyMtpUnmounted_TestCase_003 End";
}

/**
 * @tc.name: GetPartitionTable_TestCase_001
 * @tc.desc: GetPartitionTable with non-existent disk returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetPartitionTable_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    EXPECT_EQ(dm.GetPartitionTable("nonexistent-disk", info), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_001 End";
}

/**
 * @tc.name: GetPartitionTable_TestCase_002
 * @tc.desc: GetPartitionTable with IPC failure returns E_GET_PARTITION_ERROR.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetPartitionTable_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gpt-2"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, GetPartitionTableInfo(_, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    PartitionTableInfo info;
    EXPECT_EQ(dm.GetPartitionTable("disk-gpt-2", info), E_GET_PARTITION_ERROR);
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_002 End";
}

/**
 * @tc.name: GetPartitionTable_TestCase_003
 * @tc.desc: GetPartitionTable with empty execRet returns E_GET_PARTITION_ERROR.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetPartitionTable_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_003 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gpt-3"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, GetPartitionTableInfo(_, _)).WillOnce(DoAll(
        SetArgReferee<1>(""), Return(ERR_OK)));
    PartitionTableInfo info;
    EXPECT_EQ(dm.GetPartitionTable("disk-gpt-3", info), E_GET_PARTITION_ERROR);
    GTEST_LOG_(INFO) << "GetPartitionTable_TestCase_003 End";
}

/**
 * @tc.name: CreatePartition_TestCase_001
 * @tc.desc: CreatePartition with non-existent disk returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, CreatePartition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    PartitionParams params(1, 2048, 500000, "vfat");
    EXPECT_EQ(dm.CreatePartition("nonexistent-disk", params), E_NON_EXIST);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_001 End";
}

/**
 * @tc.name: CreatePartition_TestCase_002
 * @tc.desc: CreatePartition with CD disk returns E_CREATE_PARTITION_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, CreatePartition_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-cp-2"));
    PartitionParams params(1, 2048, 500000, "vfat");
    EXPECT_EQ(dm.CreatePartition("disk-cp-2", params), E_CREATE_PARTITION_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "CreatePartition_TestCase_002 End";
}

/**
 * @tc.name: DeletePartition_TestCase_001
 * @tc.desc: DeletePartition with non-existent disk returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, DeletePartition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.DeletePartition("nonexistent-disk", 1), E_NON_EXIST);
    GTEST_LOG_(INFO) << "DeletePartition_TestCase_001 End";
}

/**
 * @tc.name: FormatPartition_TestCase_001
 * @tc.desc: FormatPartition with non-existent disk returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, FormatPartition_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(dm.FormatPartition("nonexistent-disk", 1, params), E_NON_EXIST);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_001 End";
}

/**
 * @tc.name: FormatPartition_TestCase_002
 * @tc.desc: FormatPartition with CD disk returns E_DELETE_PARTITION_NOT_SUPPORT.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, FormatPartition_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-fp-2"));
    FormatParams params("vfat", true, "volume");
    EXPECT_EQ(dm.FormatPartition("disk-fp-2", 1, params), E_FORMAT_PARTITION_NOT_SUPPORT);
    GTEST_LOG_(INFO) << "FormatPartition_TestCase_002 End";
}

/**
 * @tc.name: GetFreeSizeOfVolume_TestCase_001
 * @tc.desc: GetFreeSizeOfVolume with non-existent UUID returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetFreeSizeOfVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    int64_t freeSize = 0;
    EXPECT_EQ(dm.GetFreeSizeOfVolume("nonexistent-uuid", freeSize), E_NON_EXIST);
    EXPECT_EQ(freeSize, 0);
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_TestCase_001 End";
}

/**
 * @tc.name: GetFreeSizeOfVolume_TestCase_002
 * @tc.desc: GetFreeSizeOfVolume with volume having empty path returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetFreeSizeOfVolume_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fs-2"));
    VolumeExternal vol = MakeUsbVolume("vol-fs-2", "disk-fs-2", "uuid-fs-2", UNMOUNTED);
    vol.SetPath("");
    dm.OnVolumeCreated(vol);
    int64_t freeSize = 0;
    EXPECT_EQ(dm.GetFreeSizeOfVolume("uuid-fs-2", freeSize), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetFreeSizeOfVolume_TestCase_002 End";
}

/**
 * @tc.name: GetTotalSizeOfVolume_TestCase_001
 * @tc.desc: GetTotalSizeOfVolume with non-existent UUID returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetTotalSizeOfVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    int64_t totalSize = 0;
    EXPECT_EQ(dm.GetTotalSizeOfVolume("nonexistent-uuid", totalSize), E_NON_EXIST);
    EXPECT_EQ(totalSize, 0);
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_TestCase_001 End";
}

/**
 * @tc.name: GetTotalSizeOfVolume_TestCase_002
 * @tc.desc: GetTotalSizeOfVolume with volume having empty path returns E_NON_EXIST.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetTotalSizeOfVolume_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ts-2"));
    VolumeExternal vol = MakeUsbVolume("vol-ts-2", "disk-ts-2", "uuid-ts-2", UNMOUNTED);
    vol.SetPath("");
    dm.OnVolumeCreated(vol);
    int64_t totalSize = 0;
    EXPECT_EQ(dm.GetTotalSizeOfVolume("uuid-ts-2", totalSize), E_NON_EXIST);
    GTEST_LOG_(INFO) << "GetTotalSizeOfVolume_TestCase_002 End";
}

HWTEST_F(DiskManagerTest, IsSafeFsUuid_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_TRUE(dm.IsSafeFsUuid("abc-123"));
    EXPECT_FALSE(dm.IsSafeFsUuid("../abc"));
    EXPECT_FALSE(dm.IsSafeFsUuid("abc/def"));
    EXPECT_TRUE(dm.IsSafeFsUuid(""));
}

HWTEST_F(DiskManagerTest, IsOddFsType_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_TRUE(dm.IsOddFsType("udf"));
    EXPECT_TRUE(dm.IsOddFsType("iso9660"));
    EXPECT_FALSE(dm.IsOddFsType("vfat"));
    EXPECT_FALSE(dm.IsOddFsType("ext4"));
}

HWTEST_F(DiskManagerTest, IsPartitioning_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.AddPartitioningDisk("disk-pt-chk");
    EXPECT_TRUE(dm.IsPartitioning("disk-pt-chk"));
    EXPECT_FALSE(dm.IsPartitioning("disk-not-pt"));
    dm.RemovePartitioningDisk("disk-pt-chk");
    EXPECT_FALSE(dm.IsPartitioning("disk-pt-chk"));
}

HWTEST_F(DiskManagerTest, LookupVolumeByUuidUnlocked_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-lk-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-lk-1", "disk-lk-1", "uuid-lk-1"));
    VolumeExternal out;
    EXPECT_EQ(dm.LookupVolumeByUuidUnlocked("uuid-lk-1", out), DiskManagerErrNo::E_OK);
    EXPECT_EQ(dm.LookupVolumeByUuidUnlocked("nonexistent-uuid", out), E_NON_EXIST);
}

HWTEST_F(DiskManagerTest, GetVolumePath_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gvp-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-gvp-1", "disk-gvp-1", "uuid-gvp-1"));
    EXPECT_EQ(dm.GetVolumePath("uuid-gvp-1"), "/mnt/data/external/uuid-gvp-1");
    EXPECT_EQ(dm.GetVolumePath("nonexistent-uuid"), "");
}

HWTEST_F(DiskManagerTest, GetOddCapacity_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    int64_t total = 0, free = 0;
    EXPECT_NE(dm.GetOddCapacity("", total, free), DiskManagerErrNo::E_OK);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, GetCapacity(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.GetOddCapacity("/dev/block/sr0", total, free), ERR_OK);
}

HWTEST_F(DiskManagerTest, IsPathMounted_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_TRUE(dm.IsPathMounted(""));
}

HWTEST_F(DiskManagerTest, EnsureFsUuidReady_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fsr-1"));
    VolumeExternal vol = MakeUsbVolume("vol-fsr-1", "disk-fsr-1", "uuid-fsr-1", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-fsr-1", volOut);
    std::string outUuid;
    EXPECT_EQ(dm.EnsureFsUuidReady(volOut, outUuid), DiskManagerErrNo::E_OK);
    EXPECT_EQ(outUuid, "uuid-fsr-1");
}

HWTEST_F(DiskManagerTest, EnsureFsUuidReady_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fsr-2"));
    VolumeExternal vol = MakeUsbVolume("vol-fsr-2", "disk-fsr-2", "", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-fsr-2", volOut);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("new-uuid"), SetArgReferee<2>("vfat"),
                        SetArgReferee<3>("lbl"), Return(ERR_OK)));
    std::string outUuid;
    EXPECT_EQ(dm.EnsureFsUuidReady(volOut, outUuid), DiskManagerErrNo::E_OK);
    EXPECT_EQ(outUuid, "new-uuid");
}

HWTEST_F(DiskManagerTest, EnsureFsUuidReady_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fsr-3"));
    VolumeExternal vol = MakeUsbVolume("vol-fsr-3", "disk-fsr-3", "", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-fsr-3", volOut);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, ReadMetadata(_, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    std::string outUuid;
    EXPECT_NE(dm.EnsureFsUuidReady(volOut, outUuid), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, ShouldUseVoldataMountPathForDiskUnlocked_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-svdp-1"));
    EXPECT_TRUE(dm.ShouldUseVoldataMountPathForDiskUnlocked("disk-svdp-1", "f2fs"));
    EXPECT_FALSE(dm.ShouldUseVoldataMountPathForDiskUnlocked("disk-svdp-1", "vfat"));
    EXPECT_FALSE(dm.ShouldUseVoldataMountPathForDiskUnlocked("disk-not-exist", "f2fs"));
}

HWTEST_F(DiskManagerTest, ShouldUseVoldataMountPathForDiskUnlocked_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svdp-usb"));
    EXPECT_FALSE(dm.ShouldUseVoldataMountPathForDiskUnlocked("disk-svdp-usb", "f2fs"));
}

HWTEST_F(DiskManagerTest, ComputeVolumeMountPolicy_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-cvp-1"));
    auto policy = dm.ComputeVolumeMountPolicy("disk-cvp-1", "f2fs");
    EXPECT_TRUE(policy.useVoldataPath);
    EXPECT_FALSE(policy.useFuseData);
    EXPECT_FALSE(policy.useDvrPath);
}

HWTEST_F(DiskManagerTest, ComputeVolumeMountPolicy_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-cvp-2"));
    auto policy = dm.ComputeVolumeMountPolicy("disk-cvp-2", "vfat");
    EXPECT_FALSE(policy.useVoldataPath);
    EXPECT_FALSE(policy.useFuseData);
}

HWTEST_F(DiskManagerTest, ComputeVolumeMountPolicy_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-cvp-3"));
    auto policy = dm.ComputeVolumeMountPolicy("disk-cvp-3", "vfat");
    EXPECT_FALSE(policy.useVoldataPath);
    EXPECT_FALSE(policy.useDvrPath);
}

HWTEST_F(DiskManagerTest, ComputeVolumeMountPolicy_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-cvp-miss"));
    auto policy = dm.ComputeVolumeMountPolicy("disk-cvp-miss", "vfat");
    EXPECT_FALSE(policy.useVoldataPath);
}

/**
 * @tc.name: ComputeVolumeMountPolicy_TestCase_005
 * @tc.desc: ext4 文件系统挂载策略不使用 voldata 路径
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, ComputeVolumeMountPolicy_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ComputeVolumeMountPolicy_TestCase_005 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-cvp-5"));
    auto policy = dm.ComputeVolumeMountPolicy("disk-cvp-5", "ext4");
    EXPECT_FALSE(policy.useVoldataPath);
    EXPECT_FALSE(policy.useFuseData);
    GTEST_LOG_(INFO) << "ComputeVolumeMountPolicy_TestCase_005 End";
}

/**
 * @tc.name: ComputeVolumeMountPolicy_TestCase_006
 * @tc.desc: DVR USB 磁盘挂载策略使用 DVR 路径
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, ComputeVolumeMountPolicy_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ComputeVolumeMountPolicy_TestCase_006 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeDvrDisk("disk-cvp-dvr"));
    auto policy = dm.ComputeVolumeMountPolicy("disk-cvp-dvr", "vfat");
    EXPECT_TRUE(policy.useDvrPath);
    EXPECT_FALSE(policy.useFuseData);
    GTEST_LOG_(INFO) << "ComputeVolumeMountPolicy_TestCase_006 End";
}

HWTEST_F(DiskManagerTest, BuildMountDataPath_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-bmd-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-bmd-1", "disk-bmd-1", "uuid-bmd-1"));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-bmd-1", volOut);
    DiskManager::VolumeMountPolicy policy;
    policy.useVoldataPath = false;
    policy.useDvrPath = false;
    policy.useFuseData = false;
    DiskManager::MountDataPathParams params{volOut, "uuid-bmd-1", policy, nullptr};
    std::string path = dm.BuildMountDataPath(params);
    EXPECT_EQ(path, "/mnt/data/external/uuid-bmd-1");
}

HWTEST_F(DiskManagerTest, BuildMountDataPath_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-bmd-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-bmd-2", "disk-bmd-2", "uuid-bmd-2"));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-bmd-2", volOut);
    DiskManager::VolumeMountPolicy policy;
    policy.useVoldataPath = false;
    policy.useDvrPath = false;
    policy.useFuseData = true;
    DiskManager::MountDataPathParams params{volOut, "uuid-bmd-2", policy, nullptr};
    std::string path = dm.BuildMountDataPath(params);
    EXPECT_EQ(path, "/mnt/data/external_fuse/uuid-bmd-2");
}

HWTEST_F(DiskManagerTest, ResolveVolumeFlagsUnlocked_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-rvf-1"));
    EXPECT_EQ(dm.ResolveVolumeFlagsUnlocked("disk-rvf-1"), USB_FLAG);
    dm.OnDiskCreated(MakeSdDisk("disk-rvf-2"));
    EXPECT_EQ(dm.ResolveVolumeFlagsUnlocked("disk-rvf-2"), SD_FLAG);
    EXPECT_EQ(dm.ResolveVolumeFlagsUnlocked("nonexistent-disk"), USB_FLAG);
}

HWTEST_F(DiskManagerTest, SetVolumeStateLocked_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svs-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-svs-1", "disk-svs-1", "uuid-svs-1", UNMOUNTED));
    dm.SetVolumeStateLocked("vol-svs-1", CHECKING);
    VolumeExternal out;
    dm.GetVolumeById("vol-svs-1", out);
    EXPECT_EQ(out.GetState(), CHECKING);
}

HWTEST_F(DiskManagerTest, PublishFormatFailEvent_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-pff-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-pff-1", "disk-pff-1", "uuid-pff-1", UNMOUNTED));
    dm.PublishFormatFailEvent("vol-pff-1");
    VolumeExternal out;
    dm.GetVolumeById("vol-pff-1", out);
    EXPECT_EQ(out.GetState(), FORMAT_FINISH_FAIL);
}

HWTEST_F(DiskManagerTest, UpdateVolumeAfterFormat_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvf-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-uvf-1", "disk-uvf-1", "uuid-old", UNMOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>("uuid-new"), SetArgReferee<2>("ext4"),
                        SetArgReferee<3>("NewLabel"), Return(ERR_OK)));
    EXPECT_EQ(dm.UpdateVolumeAfterFormat("vol-uvf-1", "ext4", "disk-uvf-1", "uuid-old", "vol-uvf-1"), E_OK);
    VolumeExternal out;
    dm.GetVolumeById("vol-uvf-1", out);
    EXPECT_EQ(out.GetUuid(), "uuid-new");
    EXPECT_EQ(out.GetDescription(), "NewLabel");
}

HWTEST_F(DiskManagerTest, UpdateVolumeAfterFormat_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.UpdateVolumeAfterFormat("nonexistent-vol", "vfat", "disk-x", "old-uuid", "block-x"), E_NON_EXIST);
}

HWTEST_F(DiskManagerTest, FindVolumeForPartition_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fvp-1"));
    VolumeExternal vol = MakeUsbVolume("vol-fvp-1", "disk-fvp-1", "uuid-fvp-1", UNMOUNTED);
    vol.SetPartitionNum(1);
    dm.OnVolumeCreated(vol);
    Disk diskOut;
    dm.GetDiskById("disk-fvp-1", diskOut);
    VolumeExternal found = dm.FindVolumeForPartition(diskOut, 1);
    EXPECT_EQ(found.GetId(), "vol-fvp-1");
    VolumeExternal notFound = dm.FindVolumeForPartition(diskOut, 99);
    EXPECT_TRUE(notFound.GetId().empty());
}

HWTEST_F(DiskManagerTest, IsDiskNotReady_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dnr-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dnr-1", "disk-dnr-1", "uuid-dnr-1", UNMOUNTED));
    EXPECT_FALSE(dm.IsDiskNotReady("disk-dnr-1"));
    EXPECT_TRUE(dm.IsDiskNotReady("nonexistent-disk"));
}

HWTEST_F(DiskManagerTest, IsDiskNotReady_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dnr-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dnr-2", "disk-dnr-2", "uuid-dnr-2", MOUNTED));
    EXPECT_TRUE(dm.IsDiskNotReady("disk-dnr-2"));
}

HWTEST_F(DiskManagerTest, IsDiskNotReady_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dnr-3"));
    EXPECT_FALSE(dm.IsDiskNotReady("disk-dnr-3"));
}

HWTEST_F(DiskManagerTest, IsVolumeMounted_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_TRUE(dm.IsVolumeMounted("nonexistent-disk", 1));
    dm.OnDiskCreated(MakeUsbDisk("disk-ivm-1"));
    VolumeExternal vol = MakeUsbVolume("vol-ivm-1", "disk-ivm-1", "uuid-ivm-1", MOUNTED);
    vol.SetPartitionNum(1);
    dm.OnVolumeCreated(vol);
    EXPECT_TRUE(dm.IsVolumeMounted("disk-ivm-1", 1));
}

HWTEST_F(DiskManagerTest, MountUsbFuseIfNeeded_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.MountUsbFuseIfNeeded("vol-id", "vfat", "uuid-safe", false), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, MountUsbFuseIfNeeded_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(true));
    EXPECT_EQ(dm.MountUsbFuseIfNeeded("vol-id", "vfat", "../unsafe", true), E_PARAMS_INVALID);
}

HWTEST_F(DiskManagerTest, MountUsbFuseIfNeeded_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.MountUsbFuseIfNeeded("vol-id", "vfat", "uuid-safe", true), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, Erase_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-er-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-er-1", "disk-er-1", "uuid-er-1", MOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Erase(_)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Eject(_)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Erase("vol-er-1"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, Erase_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-er-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-er-2", "disk-er-2", "uuid-er-2"));
    EXPECT_EQ(dm.Erase("nonexistent-vol"), E_NON_EXIST);
}

HWTEST_F(DiskManagerTest, Eject_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ej-1"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Eject(_)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Eject("disk-ej-1"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, Eject_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ej-2"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Eject(_)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.Eject("disk-ej-2"), ERR_OK);
}

HWTEST_F(DiskManagerTest, Eject_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.Eject("nonexistent-disk"), E_NON_EXIST);
}

HWTEST_F(DiskManagerTest, Burn_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-bn-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-bn-1", "disk-bn-1", "uuid-bn-1"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Burn(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Burn("vol-bn-1", "dao"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, Burn_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-bn-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-bn-2", "disk-bn-2", "uuid-bn-2"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Burn(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.Burn("vol-bn-2", "dao"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, CreateIsoImage_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-cii-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-cii-2", "disk-cii-2", "uuid-cii-2"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, CreateIsoImage(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.CreateIsoImage("vol-cii-2", "/tmp/img.iso"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, CreateIsoImage_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-cii-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-cii-3", "disk-cii-3", "uuid-cii-3"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, CreateIsoImage(_, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.CreateIsoImage("vol-cii-3", "/tmp/img.iso"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, GetVolumeOpProcess_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gvo-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-gvo-2", "disk-gvo-2", "uuid-gvo-2"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, GetVolumeOpProcess(_, _)).WillOnce(DoAll(SetArgReferee<1>(50), Return(ERR_OK)));
    int32_t progress = 0;
    EXPECT_EQ(dm.GetVolumeOpProcess("vol-gvo-2", progress), DiskManagerErrNo::E_OK);
    EXPECT_EQ(progress, 50);
}

HWTEST_F(DiskManagerTest, VerifyBurnData_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-vbd-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-vbd-2", "disk-vbd-2", "uuid-vbd-2"));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, VerifyBurnData(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.VerifyBurnData("vol-vbd-2", 1), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, RepairAndCheckVolume_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-rcv-1"));
    VolumeExternal vol = MakeUsbVolume("vol-rcv-1", "disk-rcv-1", "uuid-rcv-1", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-rcv-1", volOut);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Repair(_, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.RepairAndCheckVolume(volOut, "vol-rcv-1"), E_OK);
}

HWTEST_F(DiskManagerTest, RepairAndCheckVolume_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-rcv-2"));
    VolumeExternal vol = MakeUsbVolume("vol-rcv-2", "disk-rcv-2", "uuid-rcv-2", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-rcv-2", volOut);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Repair(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.RepairAndCheckVolume(volOut, "vol-rcv-2"), E_OK);
}

HWTEST_F(DiskManagerTest, ParsePartitionInfo_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionInfo info;
    EXPECT_TRUE(dm.ParsePartitionInfo("1 2048 500000", info));
    EXPECT_EQ(info.GetPartitionNum(), 1);
    EXPECT_EQ(info.GetStartSector(), 2048);
    EXPECT_EQ(info.GetEndSector(), 500000);
}

HWTEST_F(DiskManagerTest, ParsePartitionInfo_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionInfo info;
    EXPECT_FALSE(dm.ParsePartitionInfo("", info));
    EXPECT_FALSE(dm.ParsePartitionInfo("abc 2048 500000", info));
    EXPECT_FALSE(dm.ParsePartitionInfo("1 abc 500000", info));
    EXPECT_FALSE(dm.ParsePartitionInfo("1 2048 abc", info));
}

HWTEST_F(DiskManagerTest, SetSectorSize_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"Sector size (logical/physical): 512 bytes"};
    EXPECT_TRUE(dm.SetSectorSize(content, info));
    EXPECT_EQ(info.GetSectorSize(), 512);
}

HWTEST_F(DiskManagerTest, SetSectorSize_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"no sector size here"};
    EXPECT_FALSE(dm.SetSectorSize(content, info));
}

HWTEST_F(DiskManagerTest, SetAlignSector_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"Partitions will be aligned on 2048-sector boundaries"};
    EXPECT_TRUE(dm.SetAlignSector(content, info));
    EXPECT_EQ(info.GetAlignSector(), 2048);
}

HWTEST_F(DiskManagerTest, SetAlignSector_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"no align info"};
    EXPECT_FALSE(dm.SetAlignSector(content, info));
}

HWTEST_F(DiskManagerTest, SetUsableSector_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"First usable sector is 34, last usable sector is 1000"};
    EXPECT_TRUE(dm.SetUsableSector(content, info));
    EXPECT_EQ(info.GetLastUsableSector(), 1000);
}

HWTEST_F(DiskManagerTest, SetUsableSector_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"no usable sector"};
    EXPECT_FALSE(dm.SetUsableSector(content, info));
}

HWTEST_F(DiskManagerTest, SetTableType_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"Found invalid GPT and valid MBR"};
    dm.SetTableType(content, info);
    EXPECT_EQ(info.GetTableType(), "MBR");
}

HWTEST_F(DiskManagerTest, SetTableType_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo info;
    std::vector<std::string> content = {"Disk /dev/sda: 100 GB"};
    dm.SetTableType(content, info);
    EXPECT_EQ(info.GetTableType(), "GPT");
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_001, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams validParams(1, 2048, 100000, "vfat");
    EXPECT_TRUE(DiskManager::GetInstance().IsParamsValid(validParams, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_002, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams startLow(1, 0, 100000, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(startLow, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_003, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams notAligned(1, 1, 100000, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(notAligned, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_004, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams endOutOfRange(1, 2048, 2000000, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(endOutOfRange, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_005, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams smallVfat(1, 2048, 2049, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(smallVfat, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_006, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams smallF2fs(1, 2048, 4096, "f2fs");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(smallF2fs, info));
}

HWTEST_F(DiskManagerTest, GetFsTypeByDiskIdAndPartNum_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gft-1"));
    VolumeExternal vol = MakeUsbVolume("vol-gft-1", "disk-gft-1", "uuid-gft-1", UNMOUNTED);
    vol.SetPartitionNum(1);
    vol.SetFsType(static_cast<int32_t>(VFAT));
    dm.OnVolumeCreated(vol);
    EXPECT_EQ(dm.GetFsTypeByDiskIdAndPartNum("disk-gft-1", 1), "vfat");
    EXPECT_EQ(dm.GetFsTypeByDiskIdAndPartNum("nonexistent-disk", 1), "");
}

HWTEST_F(DiskManagerTest, DeletePartition_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-dp-cd"));
    EXPECT_EQ(dm.DeletePartition("disk-dp-cd", 1), E_DELETE_PARTITION_NOT_SUPPORT);
}

HWTEST_F(DiskManagerTest, FormatPartition_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fp-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-fp-3", "disk-fp-3", "uuid-fp-3", MOUNTED));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-fp-3", volOut);
    volOut.SetPartitionNum(1);
    FormatParams params("vfat", true, "volume");
    EXPECT_NE(dm.FormatPartition("disk-fp-3", 1, params), E_OK);
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_007, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams params(1, 2048, 4096, "exfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(params, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_008, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams params(1, 2048, 4096, "ntfs");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(params, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_009, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams params(1, 2048, 500, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(params, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_010, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    PartitionParams params(1, 1000001, 2000000, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(params, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_011, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(100000);
    PartitionParams params(1, 100001, 200000, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(params, info));
}

HWTEST_F(DiskManagerTest, IsParamsValid_TestCase_012, TestSize.Level0)
{
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    PartitionParams params(1, 100, 200, "vfat");
    EXPECT_FALSE(DiskManager::GetInstance().IsParamsValid(params, info));
}

HWTEST_F(DiskManagerTest, ParsePartitionInfo_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionInfo info;
    std::string item;
    EXPECT_FALSE(dm.ParsePartitionInfo("", info));
    std::stringstream ss(" ");
    ss >> item;
    EXPECT_TRUE(item.empty());
}

HWTEST_F(DiskManagerTest, SetUsableSector_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo tableInfo;
    std::vector<std::string> content = {"First usable sector is  2048invalid"};
    EXPECT_FALSE(dm.SetUsableSector(content, tableInfo));
}

HWTEST_F(DiskManagerTest, SetUsableSector_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo tableInfo;
    std::vector<std::string> content = {"First usable sector is  not_a_number"};
    EXPECT_FALSE(dm.SetUsableSector(content, tableInfo));
}

HWTEST_F(DiskManagerTest, SetSectorSize_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo tableInfo;
    std::vector<std::string> content = {"No sector size information available"};
    EXPECT_FALSE(dm.SetSectorSize(content, tableInfo));
}

HWTEST_F(DiskManagerTest, SetSectorSize_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo tableInfo;
    std::vector<std::string> content = {"Sector size (logical/physical):  abc"};
    EXPECT_FALSE(dm.SetSectorSize(content, tableInfo));
}

HWTEST_F(DiskManagerTest, SetAlignSector_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo tableInfo;
    std::vector<std::string> content = {"Alignment:  2048invalid"};
    EXPECT_FALSE(dm.SetAlignSector(content, tableInfo));
}

HWTEST_F(DiskManagerTest, SetAlignSector_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    PartitionTableInfo tableInfo;
    std::vector<std::string> content = {"Alignment:  xyz"};
    EXPECT_FALSE(dm.SetAlignSector(content, tableInfo));
}

HWTEST_F(DiskManagerTest, Burn_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.Burn("nonexistent-vol", "options"), E_NON_EXIST);
}

/**
 * @tc.name: Burn_TestCase_004
 * @tc.desc: 刻录成功后正常弹出光盘
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Burn_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Burn_TestCase_004 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-bn-4"));
    VolumeExternal vol = MakeUsbVolume("vol-bn-4", "disk-bn-4", "uuid-bn-4");
    vol.SetExtraInfo(R"({"ODD_INFO":{"DISC_TYPE":"DVD+RW"}})");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Burn(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Eject(_)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Burn("vol-bn-4", "dao"), DiskManagerErrNo::E_OK);
    GTEST_LOG_(INFO) << "Burn_TestCase_004 End";
}

/**
 * @tc.name: Burn_TestCase_005
 * @tc.desc: 刻录成功但 Eject 失败仍返回 E_OK
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, Burn_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Burn_TestCase_005 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-bn-5"));
    VolumeExternal vol = MakeUsbVolume("vol-bn-5", "disk-bn-5", "uuid-bn-5");
    vol.SetExtraInfo(R"({"ODD_INFO":{"DISC_TYPE":"BD-R"}})");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Burn(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Eject(_)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_EQ(dm.Burn("vol-bn-5", "dao"), DiskManagerErrNo::E_OK);
    GTEST_LOG_(INFO) << "Burn_TestCase_005 End";
}

HWTEST_F(DiskManagerTest, GetVolumeOpProcess_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gvo-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-gvo-3", "disk-gvo-3", "uuid-gvo-3", UNMOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, GetVolumeOpProcess(_, _)).WillOnce(Return(ERR_OK));
    int32_t progress = 0;
    EXPECT_EQ(dm.GetVolumeOpProcess("vol-gvo-3", progress), ERR_OK);
}

HWTEST_F(DiskManagerTest, VerifyBurnData_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-vbd-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-vbd-3", "disk-vbd-3", "uuid-vbd-3", UNMOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, VerifyBurnData(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.VerifyBurnData("vol-vbd-3", 0), ERR_OK);
}

HWTEST_F(DiskManagerTest, Erase_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-er-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-er-3", "disk-er-3", "uuid-er-3", MOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Erase(_)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Erase("vol-er-3"), ERR_OK);
}

HWTEST_F(DiskManagerTest, Erase_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-er-4"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-er-4", "disk-er-4", "uuid-er-4", MOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Erase(_)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.Erase("vol-er-4"), E_OK);
}

HWTEST_F(DiskManagerTest, SetVolumeDescription_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svd-6"));
    VolumeExternal vol = MakeUsbVolume("vol-svd-6", "disk-svd-6", "uuid-svd-6", UNMOUNTED, F2FS);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, SetLabel(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.SetVolumeDescription("uuid-svd-6", "myUSB"), E_OK);
}

HWTEST_F(DiskManagerTest, GetAllVolumes_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-gav-5"));
    dm.OnVolumeCreated(MakeUdfVolume("vol-iso-gav5", "disk-gav-5", "uuid-iso-gav5"));
    std::vector<VolumeExternal> volumes;
    EXPECT_EQ(dm.GetAllVolumes(volumes), E_OK);
    EXPECT_GE(volumes.size(), static_cast<size_t>(1));
}

HWTEST_F(DiskManagerTest, GetFsTypeByDiskIdAndPartNum_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gft-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-gft-2", "disk-gft-2", "uuid-gft-2", UNMOUNTED));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-gft-2", volOut);
    volOut.SetPartitionNum(5);
    dm.volumeMap_["vol-gft-2"] = volOut;
    EXPECT_EQ(dm.GetFsTypeByDiskIdAndPartNum("disk-gft-2", 5), volOut.GetFsTypeString());
}

HWTEST_F(DiskManagerTest, IsVolumeMounted_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ivm-2"));
    EXPECT_TRUE(dm.IsVolumeMounted("disk-ivm-2", 1));
}

HWTEST_F(DiskManagerTest, IsVolumeMounted_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ivm-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-ivm-3", "disk-ivm-3", "uuid-ivm-3", UNMOUNTED));
    EXPECT_FALSE(dm.IsVolumeMounted("disk-ivm-3", 999));
}

HWTEST_F(DiskManagerTest, PurgeVolumesForDisk_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-pvd-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-pvd-3a", "disk-pvd-3", "../unsafe", UNMOUNTED));
    dm.OnVolumeCreated(MakeUsbVolume("vol-pvd-3b", "disk-pvd-3", "uuid-safe", UNMOUNTED));
    EXPECT_EQ(dm.PurgeVolumesForDisk("disk-pvd-3"), E_OK);
}

HWTEST_F(DiskManagerTest, TryToFix_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ttf-5"));
    VolumeExternal vol = MakeUsbVolume("vol-ttf-5", "disk-ttf-5", "uuid-ttf-5", DAMAGED_MOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Repair(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.TryToFix("vol-ttf-5"), E_OK);
}

HWTEST_F(DiskManagerTest, TryToFix_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ttf-6"));
    VolumeExternal vol = MakeUsbVolume("vol-ttf-6", "disk-ttf-6", "uuid-ttf-6", MOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Repair(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.TryToFix("vol-ttf-6"), E_OK);
}

HWTEST_F(DiskManagerTest, TryToFix_TestCase_007, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ttf-7"));
    VolumeExternal vol = MakeUsbVolume("vol-ttf-7", "disk-ttf-7", "uuid-ttf-7", DAMAGED_MOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.TryToFix("vol-ttf-7"), E_OK);
}

HWTEST_F(DiskManagerTest, Unmount_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-um-5"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-um-5", "disk-um-5", "uuid-um-5", DAMAGED_MOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Unmount("vol-um-5"), ERR_OK);
}

HWTEST_F(DiskManagerTest, Unmount_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-um-6"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-um-6", "disk-um-6", "uuid-um-6", ENCRYPTED_AND_LOCKED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Unmount("vol-um-6"), ERR_OK);
}

HWTEST_F(DiskManagerTest, Unmount_TestCase_007, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-um-7"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-um-7", "disk-um-7", "uuid-um-7", ENCRYPTED_AND_UNLOCKED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Unmount("vol-um-7"), ERR_OK);
}

HWTEST_F(DiskManagerTest, IsPathMounted_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_FALSE(dm.IsPathMounted("/mnt/nonexistent_path_12345"));
}

HWTEST_F(DiskManagerTest, IsPathMounted_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_TRUE(dm.IsPathMounted("/mnt/data/"));
}

HWTEST_F(DiskManagerTest, MountUsbFuseIfNeeded_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(true));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, MountFuseDevice(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(ufAdapter, NotifyUsbFuseMount(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(dm.MountUsbFuseIfNeeded("vol-muf-4", "vfat", "uuid-safe", true), E_OK);
}

HWTEST_F(DiskManagerTest, MountUsbFuseIfNeeded_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(true));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, MountFuseDevice(_, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.MountUsbFuseIfNeeded("vol-muf-5", "vfat", "uuid-safe", true), E_OK);
}

HWTEST_F(DiskManagerTest, MountUsbFuseIfNeeded_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(true));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, MountFuseDevice(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(ufAdapter, NotifyUsbFuseMount(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.MountUsbFuseIfNeeded("vol-muf-6", "vfat", "uuid-safe", true), E_OK);
}

HWTEST_F(DiskManagerTest, Mount_TestCase_007, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-7"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-7", "disk-mt-7", "uuid-mt-7", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(sdAdapter, ReadMetadata(_, _, _, _)).WillRepeatedly(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Mount("vol-mt-7"), E_OK);
}

HWTEST_F(DiskManagerTest, Mount_TestCase_008, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mt-8"));
    VolumeExternal vol = MakeUsbVolume("vol-mt-8", "disk-mt-8", "", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(sdAdapter, ReadMetadata(_, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.Mount("vol-mt-8"), E_OK);
}

HWTEST_F(DiskManagerTest, Partition_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-pt-4"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-pt-4", "disk-pt-4", "uuid-pt-4", UNMOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Partition(_, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Partition("disk-pt-4", 1), E_OK);
}

HWTEST_F(DiskManagerTest, Partition_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-pt-5"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-pt-5", "disk-pt-5", "uuid-pt-5", MOUNTED));
    EXPECT_EQ(dm.Partition("disk-pt-5", 1), E_VOL_STATE);
}

HWTEST_F(DiskManagerTest, CreatePartition_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-cp-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-cp-3", "disk-cp-3", "uuid-cp-3", UNMOUNTED));
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    dm.partitionTableMap_["disk-cp-3"] = info;
    PartitionParams params(1, 2048, 100000, "unknown_code");
    EXPECT_EQ(dm.CreatePartition("disk-cp-3", params), E_CREATE_PARTITION_NOT_SUPPORT);
}

HWTEST_F(DiskManagerTest, CreatePartition_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-cp-4"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-cp-4", "disk-cp-4", "uuid-cp-4", UNMOUNTED));
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    dm.partitionTableMap_["disk-cp-4"] = info;
    PartitionParams params(1, 2048, 100000, "vfat");
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, CreatePartition(_, _, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(dm.CreatePartition("disk-cp-4", params), E_OK);
}

HWTEST_F(DiskManagerTest, CreatePartition_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-cp-5"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-cp-5", "disk-cp-5", "uuid-cp-5", UNMOUNTED));
    PartitionTableInfo info;
    info.SetSectorSize(512);
    info.SetAlignSector(2048);
    info.SetLastUsableSector(1000000);
    dm.partitionTableMap_["disk-cp-5"] = info;
    PartitionParams params(1, 2048, 100000, "vfat");
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, CreatePartition(_, _, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_EQ(dm.CreatePartition("disk-cp-5", params), E_CREATE_PARTITION_ERROR);
}

HWTEST_F(DiskManagerTest, DeletePartition_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dp-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dp-3", "disk-dp-3", "uuid-dp-3", UNMOUNTED));
    PartitionTableInfo info;
    PartitionInfo pinfo;
    pinfo.SetPartitionNum(1);
    std::vector<PartitionInfo> partitions = {pinfo};
    info.SetPartitions(partitions);
    dm.partitionTableMap_["disk-dp-3"] = info;
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, DeletePartition(_, _, _)).WillOnce(Return(E_OK));
    EXPECT_EQ(dm.DeletePartition("disk-dp-3", 1), E_OK);
}

HWTEST_F(DiskManagerTest, DeletePartition_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dp-4"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dp-4", "disk-dp-4", "uuid-dp-4", UNMOUNTED));
    PartitionTableInfo info;
    dm.partitionTableMap_["disk-dp-4"] = info;
    EXPECT_EQ(dm.DeletePartition("disk-dp-4", 99), E_NON_EXIST);
}

HWTEST_F(DiskManagerTest, DeletePartition_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dp-5"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dp-5", "disk-dp-5", "uuid-dp-5", UNMOUNTED));
    PartitionTableInfo info;
    PartitionInfo pinfo;
    pinfo.SetPartitionNum(1);
    info.SetPartitions({pinfo});
    dm.partitionTableMap_["disk-dp-5"] = info;
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, DeletePartition(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_EQ(dm.DeletePartition("disk-dp-5", 1), E_DELETE_PARTITION_ERROR);
}

HWTEST_F(DiskManagerTest, DeletePartition_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dp-6"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dp-6", "disk-dp-6", "uuid-dp-6", MOUNTED));
    EXPECT_EQ(dm.DeletePartition("disk-dp-6", 1), E_VOL_STATE);
}

HWTEST_F(DiskManagerTest, DeletePartition_TestCase_007, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dp-7"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dp-7", "disk-dp-7", "uuid-dp-7", UNMOUNTED));
    EXPECT_EQ(dm.DeletePartition("disk-dp-7", 1), E_NON_EXIST);
}

HWTEST_F(DiskManagerTest, FormatPartition_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fp-4"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-fp-4", "disk-fp-4", "uuid-fp-4", MOUNTED));
    PartitionTableInfo info;
    PartitionInfo pinfo;
    pinfo.SetPartitionNum(1);
    info.SetPartitions({pinfo});
    dm.partitionTableMap_["disk-fp-4"] = info;
    FormatParams params("vfat", true, "volume");
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, FormatPartition(_, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.FormatPartition("disk-fp-4", 1, params), E_OK);
}

HWTEST_F(DiskManagerTest, FormatPartition_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fp-5"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-fp-5", "disk-fp-5", "uuid-fp-5", UNMOUNTED));
    PartitionTableInfo info;
    PartitionInfo pinfo;
    pinfo.SetPartitionNum(1);
    info.SetPartitions({pinfo});
    dm.partitionTableMap_["disk-fp-5"] = info;
    FormatParams params("vfat", true, "volume");
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, FormatPartition(_, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_EQ(dm.FormatPartition("disk-fp-5", 1, params), E_FORMAT_PARTITION_ERROR);
}

HWTEST_F(DiskManagerTest, FormatPartition_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fp-6"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-fp-6", "disk-fp-6", "uuid-fp-6", UNMOUNTED));
    PartitionTableInfo info;
    PartitionInfo pinfo;
    pinfo.SetPartitionNum(2);
    info.SetPartitions({pinfo});
    dm.partitionTableMap_["disk-fp-6"] = info;
    FormatParams params("vfat", true, "volume");
    EXPECT_NE(dm.FormatPartition("disk-fp-6", 1, params), E_OK);
}

HWTEST_F(DiskManagerTest, FormatPartition_TestCase_007, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-fp-7"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-fp-7", "disk-fp-7", "uuid-fp-7", UNMOUNTED));
    PartitionTableInfo info;
    dm.partitionTableMap_["disk-fp-7"] = info;
    FormatParams params("vfat", true, "volume");
    EXPECT_NE(dm.FormatPartition("disk-fp-7", 1, params), E_OK);
}

HWTEST_F(DiskManagerTest, DestroyVolumeByDiskIdAndPartNum_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    EXPECT_FALSE(dm.DestroyVolumeByDiskIdAndPartNum("nonexistent-disk", 1));
}

HWTEST_F(DiskManagerTest, DestroyVolumeByDiskIdAndPartNum_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dv-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-dv-2", "disk-dv-2", "uuid-dv-2", UNMOUNTED));
    EXPECT_FALSE(dm.DestroyVolumeByDiskIdAndPartNum("disk-dv-2", 999));
}

HWTEST_F(DiskManagerTest, DestroyVolumeByDiskIdAndPartNum_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dv-3"));
    VolumeExternal vol = MakeUsbVolume("vol-dv-3", "disk-dv-3", "uuid-dv-3", UNMOUNTED);
    vol.SetPartitionNum(1);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, DestroyBlockDeviceNode(_)).WillOnce(Return(E_OK));
    EXPECT_TRUE(dm.DestroyVolumeByDiskIdAndPartNum("disk-dv-3", 1));
}

HWTEST_F(DiskManagerTest, DestroyVolumeByDiskIdAndPartNum_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dv-4"));
    VolumeExternal vol = MakeUsbVolume("vol-dv-4", "disk-dv-4", "uuid-dv-4", MOUNTED);
    vol.SetPartitionNum(1);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, DestroyBlockDeviceNode(_)).WillOnce(Return(E_OK));
    EXPECT_TRUE(dm.DestroyVolumeByDiskIdAndPartNum("disk-dv-4", 1));
}

HWTEST_F(DiskManagerTest, DestroyVolumeByDiskIdAndPartNum_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dv-5"));
    VolumeExternal vol = MakeUsbVolume("vol-dv-5", "disk-dv-5", "uuid-dv-5", UNMOUNTED);
    vol.SetPartitionNum(1);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, DestroyBlockDeviceNode(_)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_FALSE(dm.DestroyVolumeByDiskIdAndPartNum("disk-dv-5", 1));
}

HWTEST_F(DiskManagerTest, MountVolumeFilesystem_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mvf-1"));
    VolumeExternal vol = MakeUsbVolume("vol-mvf-1", "disk-mvf-1", "uuid-mvf-1", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(false));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mvf-1", volOut);
    EXPECT_EQ(dm.MountVolumeFilesystem(volOut, "vfat", "uuid-mvf-1"), E_OK);
}

HWTEST_F(DiskManagerTest, MountVolumeFilesystem_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mvf-2"));
    VolumeExternal vol = MakeUsbVolume("vol-mvf-2", "disk-mvf-2", "uuid-mvf-2", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(false));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mvf-2", volOut);
    EXPECT_NE(dm.MountVolumeFilesystem(volOut, "vfat", "uuid-mvf-2"), E_OK);
}

HWTEST_F(DiskManagerTest, MountVolumeFilesystem_TestCase_004, TestSize.Level0)
{
    g_mockFindParameterResult = 1;
    g_mockGetParameterValueResult = 4;
    strcpy_s(g_mockParameterValue, sizeof(g_mockParameterValue), "true");
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-mvfs-hmfs"));
    VolumeExternal vol = MakeUsbVolume("vol-mvfs-hmfs", "disk-mvfs-hmfs", "uuid-mvfs-hmfs", UNMOUNTED);
    vol.SetFsType(static_cast<int32_t>(HMFS));
    dm.OnVolumeCreated(vol);

    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).Times(0);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mvfs-hmfs", volOut);
    EXPECT_EQ(dm.MountVolumeFilesystem(volOut, "hmfs", "uuid-mvfs-hmfs"), E_OK);
}

HWTEST_F(DiskManagerTest, MountVolumeFilesystem_TestCase_005, TestSize.Level0)
{
    g_mockFindParameterResult = 1;
    g_mockGetParameterValueResult = 4;
    strcpy_s(g_mockParameterValue, sizeof(g_mockParameterValue), "true");
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeHddDisk("disk-ad-hdd"));
    VolumeExternal vol = MakeUsbVolume("vol-ad-hdd", "disk-ad-hdd", "uuid-ad-hdd", UNMOUNTED);
    vol.SetFsType(static_cast<int32_t>(HMFS));
    dm.OnVolumeCreated(vol);

    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).Times(0);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-ad-hdd", volOut);
    EXPECT_EQ(dm.MountVolumeFilesystem(volOut, "hmfs", "uuid-ad-hdd"), E_OK);
}

HWTEST_F(DiskManagerTest, ReadPersistUsbReadonlyMount_TestCase_001, TestSize.Level0)
{
    g_mockFindParameterResult = 1;
    g_mockGetParameterValueResult = 5;
    strcpy_s(g_mockParameterValue, sizeof(g_mockParameterValue), "false");
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-rprm-1"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-rprm-1", "disk-rprm-1", "uuid-rprm-1", UNMOUNTED));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Mount("vol-rprm-1"), E_OK);
}

HWTEST_F(DiskManagerTest, ReadPersistUsbReadonlyMount_TestCase_002, TestSize.Level0)
{
    g_mockFindParameterResult = 1;
    g_mockGetParameterValueResult = 4;
    strcpy_s(g_mockParameterValue, sizeof(g_mockParameterValue), "true");
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-rprm-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-rprm-2", "disk-rprm-2", "uuid-rprm-2", UNMOUNTED));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Mount("vol-rprm-2"), E_OK);
}

HWTEST_F(DiskManagerTest, ReadPersistUsbReadonlyMount_TestCase_003, TestSize.Level0)
{
    g_mockFindParameterResult = -1;
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-rprm-3"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-rprm-3", "disk-rprm-3", "uuid-rprm-3", UNMOUNTED));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.Mount("vol-rprm-3"), E_OK);
}

HWTEST_F(DiskManagerTest, NotifyMtpMounted_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.NotifyMtpMounted("vol-nm-4", "/mnt/data/external/uuid-nm-4", "MtpDevice", "uuid-nm-4", "mtpfs");
    VolumeExternal volOut;
    dm.GetVolumeById("vol-nm-4", volOut);
    EXPECT_EQ(volOut.GetState(), MOUNTED);
}

HWTEST_F(DiskManagerTest, ApplyDefaultDesc_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSdDisk("disk-ad-2"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-ad-2", "disk-ad-2", "uuid-ad-2", UNMOUNTED));
    dm.OnVolumeCreated(MakeUsbVolume("vol-ad-3", "disk-ad-3", "uuid-ad-3", UNMOUNTED));
    dm.OnDiskCreated(MakeCdDisk("disk-ad-4"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-ad-4", "disk-ad-4", "uuid-ad-4", UNMOUNTED));
}

HWTEST_F(DiskManagerTest, IsDiskSupported_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    Disk diskOut;
    EXPECT_EQ(dm.GetDiskById("nonexistent", diskOut), E_NON_EXIST);
}

HWTEST_F(DiskManagerTest, GetPartitionTable_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-gpt-4"));
    dm.OnVolumeCreated(MakeUsbVolume("vol-gpt-4", "disk-gpt-4", "uuid-gpt-4", UNMOUNTED));
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    std::string validDump =
        "BYT;\ndisk-pt-test 4096 2048\n"
        "Sector size (logical/physical): 512 bytes\n"
        "Partitions will be aligned on 2048-sector boundaries\n"
        "First usable sector is 2048, last usable sector is 1000000\n"
        "Partition table type: gpt\n"
        "Number  Start   End     File system  Name\n"
        "1 2048 1000000 vfat test-part";
    EXPECT_CALL(sdAdapter, GetPartitionTableInfo(_, _)).WillOnce(DoAll(SetArgReferee<1>(validDump), Return(E_OK)));
    PartitionTableInfo info;
    EXPECT_EQ(dm.GetPartitionTable("disk-gpt-4", info), E_OK);
    EXPECT_EQ(info.GetSectorSize(), 512);
    EXPECT_EQ(info.GetAlignSector(), 2048);
    EXPECT_EQ(info.GetLastUsableSector(), 1000000);
}

HWTEST_F(DiskManagerTest, UnmountVolumeMountPoints_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmp-1"));
    VolumeExternal vol = MakeUsbVolume("vol-uvmp-1", "disk-uvmp-1", "uuid-uvmp-1", MOUNTED);
    vol.SetPath("/mnt/data/external_fuse/uuid-uvmp-1");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK)).WillOnce(Return(ERR_OK));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, NotifyUsbFuseUmount(_)).WillOnce(Return(E_OK));
    EXPECT_EQ(dm.UnmountVolumeMountPoints(vol, true), E_OK);
}

HWTEST_F(DiskManagerTest, UnmountVolumeMountPoints_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmp-2"));
    VolumeExternal vol = MakeUsbVolume("vol-uvmp-2", "disk-uvmp-2", "uuid-uvmp-2", MOUNTED);
    vol.SetPath("/mnt/data/external_fuse/uuid-uvmp-2");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.UnmountVolumeMountPoints(vol, true), E_OK);
}

HWTEST_F(DiskManagerTest, UnmountVolumeMountPoints_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmp-3"));
    VolumeExternal vol = MakeUsbVolume("vol-uvmp-3", "disk-uvmp-3", "uuid-uvmp-3", MOUNTED);
    vol.SetPath("/mnt/data/external_fuse/uuid-uvmp-3");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_NE(dm.UnmountVolumeMountPoints(vol, false), E_OK);
}

HWTEST_F(DiskManagerTest, UnmountVolumeMountPoints_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmp-4"));
    VolumeExternal vol = MakeUsbVolume("vol-uvmp-4", "disk-uvmp-4", "uuid-uvmp-4", MOUNTED);
    vol.SetPath("/mnt/data/external/uuid-uvmp-4");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_EQ(dm.UnmountVolumeMountPoints(vol, true), ERR_OK);
}

HWTEST_F(DiskManagerTest, UnmountVolumeMountPoints_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmp-5"));
    VolumeExternal vol = MakeMtpVolume("vol-uvmp-5", "disk-uvmp-5", "uuid-uvmp-5");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-uvmp-5", volOut);
    EXPECT_EQ(dm.UnmountVolumeMountPoints(volOut, true), ERR_OK);
}

HWTEST_F(DiskManagerTest, UnmountVolumeMountPoints_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmp-6"));
    VolumeExternal vol = MakeUsbVolume("vol-uvmp-6", "disk-uvmp-6", "uuid-uvmp-6", UNMOUNTED);
    vol.SetPath("");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Unmount(_, _, _)).WillOnce(Return(ERR_OK));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-uvmp-6", volOut);
    EXPECT_EQ(dm.UnmountVolumeMountPoints(volOut, true), ERR_OK);
}

HWTEST_F(DiskManagerTest, ResolveUnmountForceFlag_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-ruf-1"));
    VolumeExternal vol = MakeUsbVolume("vol-ruf-1", "disk-ruf-1", "uuid-ruf-1", MOUNTED);
    dm.OnVolumeCreated(vol);
    bool forceUnmount = false;
    EXPECT_EQ(dm.ResolveUnmountForceFlag(vol, forceUnmount), DiskManagerErrNo::E_OK);
    EXPECT_TRUE(forceUnmount);
}

HWTEST_F(DiskManagerTest, ResolveUnmountForceFlag_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-ruf-2"));
    VolumeExternal vol = MakeUsbVolume("vol-ruf-2", "disk-ruf-2", "uuid-ruf-2", MOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    dm.OnVolumeCreated(vol);
    bool forceUnmount = true;
    EXPECT_EQ(dm.ResolveUnmountForceFlag(vol, forceUnmount), DiskManagerErrNo::E_OK);
    EXPECT_FALSE(forceUnmount);
}

HWTEST_F(DiskManagerTest, ResolveUnmountForceFlag_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-ruf-3"));
    VolumeExternal vol = MakeUsbVolume("vol-ruf-3", "disk-ruf-3", "../unsafe", MOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    vol.SetPath("");
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, QueryUsbIsInUse(_, _)).WillRepeatedly(DoAll(SetArgReferee<1>(false), Return(ERR_OK)));
    bool forceUnmount = true;
    EXPECT_EQ(dm.ResolveUnmountForceFlag(vol, forceUnmount), E_PARAMS_INVALID);
}

HWTEST_F(DiskManagerTest, ResolveUnmountForceFlag_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-ruf-4"));
    VolumeExternal vol = MakeUsbVolume("vol-ruf-4", "disk-ruf-4", "uuid-ruf-4", MOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, QueryUsbIsInUse(_, _)).WillOnce(DoAll(SetArgReferee<1>(false), Return(ERR_OK)));
    bool forceUnmount = true;
    EXPECT_EQ(dm.ResolveUnmountForceFlag(vol, forceUnmount), DiskManagerErrNo::E_OK);
    EXPECT_FALSE(forceUnmount);
}

HWTEST_F(DiskManagerTest, ResolveUnmountForceFlag_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-ruf-5"));
    VolumeExternal vol = MakeUsbVolume("vol-ruf-5", "disk-ruf-5", "uuid-ruf-5", MOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, QueryUsbIsInUse(_, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    bool forceUnmount = true;
    EXPECT_NE(dm.ResolveUnmountForceFlag(vol, forceUnmount), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, ResolveUnmountForceFlag_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-ruf-6"));
    VolumeExternal vol = MakeUsbVolume("vol-ruf-6", "disk-ruf-6", "uuid-ruf-6", MOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, QueryUsbIsInUse(_, _)).WillOnce(DoAll(SetArgReferee<1>(true), Return(ERR_OK)));
    bool forceUnmount = true;
    EXPECT_EQ(dm.ResolveUnmountForceFlag(vol, forceUnmount), E_UMOUNT_BUSY);
}

HWTEST_F(DiskManagerTest, MountVolumeEntry_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mve-1"));
    VolumeExternal vol = MakeUsbVolume("vol-mve-1", "disk-mve-1", "", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, ReadMetadata(_, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mve-1", volOut);
    EXPECT_NE(dm.MountVolumeEntry(volOut, "vol-mve-1"), DiskManagerErrNo::E_OK);
    EXPECT_EQ(volOut.GetState(), UNMOUNTED);
}

HWTEST_F(DiskManagerTest, MountVolumeEntry_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mve-2"));
    VolumeExternal vol = MakeUsbVolume("vol-mve-2", "disk-mve-2", "../unsafe", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mve-2", volOut);
    EXPECT_EQ(dm.MountVolumeEntry(volOut, "vol-mve-2"), E_PARAMS_INVALID);
    EXPECT_EQ(volOut.GetState(), UNMOUNTED);
}

HWTEST_F(DiskManagerTest, MountVolumeEntry_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-mve-3"));
    VolumeExternal vol = MakeUsbVolume("vol-mve-3", "disk-mve-3", "uuid-mve-3", UNMOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Check(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mve-3", volOut);
    EXPECT_NE(dm.MountVolumeEntry(volOut, "vol-mve-3"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, MountVolumeEntry_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-mve-4"));
    VolumeExternal vol = MakeUsbVolume("vol-mve-4", "disk-mve-4", "uuid-mve-4", UNMOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Check(_, _, _)).WillOnce(Return(ERR_OK));
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mve-4", volOut);
    EXPECT_EQ(dm.MountVolumeEntry(volOut, "vol-mve-4"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, MountVolumeEntry_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mve-5"));
    VolumeExternal vol = MakeUsbVolume("vol-mve-5", "disk-mve-5", "uuid-mve-5", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mve-5", volOut);
    EXPECT_NE(dm.MountVolumeEntry(volOut, "vol-mve-5"), DiskManagerErrNo::E_OK);
    EXPECT_EQ(volOut.GetState(), UNMOUNTED);
}

HWTEST_F(DiskManagerTest, MountVolumeEntry_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mve-6"));
    VolumeExternal vol = MakeUsbVolume("vol-mve-6", "disk-mve-6", "uuid-mve-6", UNMOUNTED);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mve-6", volOut);
    EXPECT_EQ(dm.MountVolumeEntry(volOut, "vol-mve-6"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, SaveVolumeFreeSize_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svs-1"));
    VolumeExternal vol = MakeUsbVolume("vol-svs-1", "disk-svs-1", "uuid-svs-1", MOUNTED);
    dm.OnVolumeCreated(vol);
    dm.SaveVolumeFreeSize(vol);
    EXPECT_EQ(vol.GetFreeSize(), 0);
}

HWTEST_F(DiskManagerTest, SaveVolumeFreeSize_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-svs-2"));
    VolumeExternal vol = MakeUsbVolume("vol-svs-2", "disk-svs-2", "", MOUNTED);
    dm.OnVolumeCreated(vol);
    dm.SaveVolumeFreeSize(vol);
}

HWTEST_F(DiskManagerTest, SaveVolumeFreeSize_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    VolumeExternal vol;
    dm.SaveVolumeFreeSize(vol);
}

HWTEST_F(DiskManagerTest, DestroyVolumeByDiskIdAndPartNum_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-dv-6"));
    VolumeExternal vol = MakeUsbVolume("vol-dv-6", "disk-dv-6", "uuid-dv-6", DAMAGED_MOUNTED);
    vol.SetPartitionNum(1);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, DestroyBlockDeviceNode(_)).WillOnce(Return(ERR_OK));
    EXPECT_TRUE(dm.DestroyVolumeByDiskIdAndPartNum("disk-dv-6", 1));
}

HWTEST_F(DiskManagerTest, UpdateVoldataMappingAfterFormat_TestCase_001, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmf-1"));
    dm.UpdateVoldataMappingAfterFormat("disk-uvmf-1", "old-uuid-uvmf-1", "new-uuid-uvmf-1", "vfat");
}

HWTEST_F(DiskManagerTest, UpdateVoldataMappingAfterFormat_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmf-2"));
    dm.UpdateVoldataMappingAfterFormat("disk-uvmf-2", "", "new-uuid-uvmf-2", "vfat");
}

HWTEST_F(DiskManagerTest, UpdateVoldataMappingAfterFormat_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-uvmf-3"));
    dm.UpdateVoldataMappingAfterFormat("disk-uvmf-3", "old-uuid-uvmf-3", "new-uuid-uvmf-3", "f2fs");
}

HWTEST_F(DiskManagerTest, UpdateVoldataMappingAfterFormat_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-uvmf-4"));
    dm.UpdateVoldataMappingAfterFormat("disk-uvmf-4", "", "new-uuid-uvmf-4", "f2fs");
}

HWTEST_F(DiskManagerTest, UpdateVoldataMappingAfterFormat_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-uvmf-5"));
    dm.UpdateVoldataMappingAfterFormat("disk-uvmf-5", "../unsafe", "new-uuid-uvmf-5", "f2fs");
}

HWTEST_F(DiskManagerTest, UpdateVoldataMappingAfterFormat_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-uvmf-6"));
    dm.UpdateVoldataMappingAfterFormat("disk-uvmf-6", "../unsafe", "new-uuid-uvmf-6", "vfat");
}

HWTEST_F(DiskManagerTest, ApplyDefaultDesc_TestCase_002, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSdDisk("disk-ad-sd"));
    VolumeExternal vol = MakeUsbVolume("vol-ad-sd", "disk-ad-sd", "uuid-ad-sd", UNMOUNTED);
    dm.OnVolumeCreated(vol);
}

HWTEST_F(DiskManagerTest, ApplyDefaultDesc_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-ad-cd"));
    VolumeExternal vol = MakeUsbVolume("vol-ad-cd", "disk-ad-cd", "uuid-ad-cd", UNMOUNTED);
    dm.OnVolumeCreated(vol);
}

HWTEST_F(DiskManagerTest, ApplyDefaultDesc_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-ad-ssd"));
    VolumeExternal vol = MakeUsbVolume("vol-ad-ssd", "disk-ad-ssd", "uuid-ad-ssd", UNMOUNTED);
    dm.OnVolumeCreated(vol);
}

HWTEST_F(DiskManagerTest, ApplyDefaultDesc_TestCase_005, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeHddDisk("disk-ad-hdd"));
    VolumeExternal vol = MakeUsbVolume("vol-ad-hdd", "disk-ad-hdd", "uuid-ad-hdd", UNMOUNTED);
    dm.OnVolumeCreated(vol);
}

HWTEST_F(DiskManagerTest, MountVolumeFilesystem_TestCase_003, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-mvfs-hmfs"));
    VolumeExternal vol = MakeUsbVolume("vol-mvfs-hmfs", "disk-mvfs-hmfs", "uuid-mvfs-hmfs", UNMOUNTED);
    vol.SetFsType(static_cast<int32_t>(HMFS));
    vol.SetUserData(true);
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, Mount(_, _, _, _, _)).WillOnce(Return(ERR_OK));
    auto &ufAdapter = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(ufAdapter, IsUsbFuseEnabledForFsType(_)).WillRepeatedly(Return(false));
    VolumeExternal volOut;
    dm.GetVolumeById("vol-mvfs-hmfs", volOut);
    EXPECT_EQ(dm.MountVolumeFilesystem(volOut, "hmfs", "uuid-mvfs-hmfs"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, PurgeVolumesForDisk_TestCase_004, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-pvd-4"));
    VolumeExternal vol1 = MakeUsbVolume("vol-pvd-4a", "disk-pvd-4", "../unsafe", UNMOUNTED);
    dm.OnVolumeCreated(vol1);
    VolumeExternal vol2 = MakeUsbVolume("vol-pvd-4b", "disk-pvd-4", "uuid/slash", UNMOUNTED);
    dm.OnVolumeCreated(vol2);
    EXPECT_EQ(dm.PurgeVolumesForDisk("disk-pvd-4"), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, GetAllVolumes_TestCase_006, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeCdDisk("disk-gav-5"));
    VolumeExternal vol = MakeUdfVolume("vol-gav-5", "disk-gav-5", "uuid-gav-5");
    dm.OnVolumeCreated(vol);
    std::vector<VolumeExternal> volumes;
    EXPECT_EQ(dm.GetAllVolumes(volumes), DiskManagerErrNo::E_OK);
}

HWTEST_F(DiskManagerTest, Unmount_TestCase_008, TestSize.Level0)
{
    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeSsdDisk("disk-um-8"));
    VolumeExternal vol = MakeUsbVolume("vol-um-8", "disk-um-8", "uuid-um-8", MOUNTED);
    vol.SetFsType(static_cast<int32_t>(F2FS));
    dm.OnVolumeCreated(vol);
    auto &sdAdapter = MockStorageDaemonAdapter::GetInstance();
    EXPECT_CALL(sdAdapter, QueryUsbIsInUse(_, _)).WillOnce(DoAll(SetArgReferee<1>(true), Return(ERR_OK)));
    EXPECT_EQ(dm.Unmount("vol-um-8"), E_UMOUNT_BUSY);
}

/**
 * @tc.name: GetDiscType_Empty_TestCase_001
 * @tc.desc: 空 extraInfo 返回空 disc 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiscType_Empty_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiscType_Empty_TestCase_001 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDiscType(""), "");
    GTEST_LOG_(INFO) << "GetDiscType_Empty_TestCase_001 End";
}

/**
 * @tc.name: GetDiscType_InvalidJson_TestCase_002
 * @tc.desc: 非法 JSON extraInfo 返回空 disc 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiscType_InvalidJson_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiscType_InvalidJson_TestCase_002 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDiscType("not-json"), "");
    GTEST_LOG_(INFO) << "GetDiscType_InvalidJson_TestCase_002 End";
}

/**
 * @tc.name: GetDiscType_MissingOddInfo_TestCase_003
 * @tc.desc: 缺少 ODD_INFO 返回空 disc 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiscType_MissingOddInfo_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiscType_MissingOddInfo_TestCase_003 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDiscType(R"({"OTHER":"value"})"), "");
    GTEST_LOG_(INFO) << "GetDiscType_MissingOddInfo_TestCase_003 End";
}

/**
 * @tc.name: GetDiscType_MissingDiscType_TestCase_004
 * @tc.desc: 缺少 DISC_TYPE 返回空 disc 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiscType_MissingDiscType_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiscType_MissingDiscType_TestCase_004 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDiscType(R"({"ODD_INFO":{"DRIVE_TYPE":"DVD"}})"), "");
    GTEST_LOG_(INFO) << "GetDiscType_MissingDiscType_TestCase_004 End";
}

/**
 * @tc.name: GetDiscType_Success_TestCase_005
 * @tc.desc: 合法 ODD_INFO 解析 disc 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiscType_Success_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiscType_Success_TestCase_005 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDiscType(R"({"ODD_INFO":{"DISC_TYPE":"BD-R"}})"), "BD-R");
    GTEST_LOG_(INFO) << "GetDiscType_Success_TestCase_005 End";
}

/**
 * @tc.name: GetDriverType_Empty_TestCase_001
 * @tc.desc: 空 extraInfo 返回空 drive 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDriverType_Empty_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDriverType_Empty_TestCase_001 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDriverType(""), "");
    GTEST_LOG_(INFO) << "GetDriverType_Empty_TestCase_001 End";
}

/**
 * @tc.name: GetDriverType_InvalidJson_TestCase_002
 * @tc.desc: 非法 JSON extraInfo 返回空 drive 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDriverType_InvalidJson_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDriverType_InvalidJson_TestCase_002 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDriverType("{bad json"), "");

    GTEST_LOG_(INFO) << "GetDriverType_InvalidJson_TestCase_002 End";
}

/**
 * @tc.name: GetDriverType_MissingOddInfo_TestCase_003
 * @tc.desc: 缺少 ODD_INFO 返回空 drive 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDriverType_MissingOddInfo_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDriverType_MissingOddInfo_TestCase_003 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDriverType(R"({"OTHER":"value"})"), "");

    GTEST_LOG_(INFO) << "GetDriverType_MissingOddInfo_TestCase_003 End";
}

/**
 * @tc.name: GetDriverType_MissingDriveType_TestCase_004
 * @tc.desc: 缺少 DRIVE_TYPE 返回空 drive 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDriverType_MissingDriveType_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDriverType_MissingDriveType_TestCase_004 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDriverType(R"({"ODD_INFO":{"DISC_TYPE":"CD-R"}})"), "");

    GTEST_LOG_(INFO) << "GetDriverType_MissingDriveType_TestCase_004 End";
}

/**
 * @tc.name: GetDriverType_Success_TestCase_005
 * @tc.desc: 合法 ODD_INFO 解析 drive 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDriverType_Success_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDriverType_Success_TestCase_005 Start";

    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.GetDriverType(R"({"ODD_INFO":{"DRIVE_TYPE":"DVD-RW"}})"), "DVD-RW");

    GTEST_LOG_(INFO) << "GetDriverType_Success_TestCase_005 End";
}

/**
 * @tc.name: SetPartitions_NoHeader_TestCase_001
 * @tc.desc: 分区表内容缺少表头时不解析分区
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetPartitions_NoHeader_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetPartitions_NoHeader_TestCase_001 Start";

    auto &dm = DiskManager::GetInstance();
    std::vector<std::string> content = {"Disk /dev/block/sda: 8 GiB", "1 2048 500000"};
    PartitionTableInfo info;
    info.SetDiskId("disk-sp-1");
    info.SetSectorSize(512);
    dm.SetPartitions(content, info);
    EXPECT_TRUE(info.GetPartitions().empty());

    GTEST_LOG_(INFO) << "SetPartitions_NoHeader_TestCase_001 End";
}

/**
 * @tc.name: SetPartitions_Success_TestCase_002
 * @tc.desc: 合法分区表解析单分区并匹配卷 fs 类型
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetPartitions_Success_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetPartitions_Success_TestCase_002 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-sp-2"));
    VolumeExternal vol = MakeUsbVolume("vol-sp-2", "disk-sp-2", "uuid-sp-2", UNMOUNTED);
    vol.SetPartitionNum(1);
    dm.OnVolumeCreated(vol);
    std::vector<std::string> content = {
        "Disk /dev/block/disk-sp-2: 8 GiB",
        "Number  Start   End",
        "1 2048 500000",
        "invalid line",
    };
    PartitionTableInfo info;
    info.SetDiskId("disk-sp-2");
    info.SetSectorSize(512);
    dm.SetPartitions(content, info);
    ASSERT_EQ(info.GetPartitions().size(), 1U);
    EXPECT_EQ(info.GetPartitions()[0].GetPartitionNum(), 1);
    EXPECT_EQ(info.GetPartitions()[0].GetFsType(), "vfat");

    GTEST_LOG_(INFO) << "SetPartitions_Success_TestCase_002 End";
}

/**
 * @tc.name: SetPartitions_MultiPartition_TestCase_003
 * @tc.desc: 多分区解析及 size 计算
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, SetPartitions_MultiPartition_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetPartitions_MultiPartition_TestCase_003 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-sp-3"));
    VolumeExternal vol1 = MakeUsbVolume("vol-sp-3a", "disk-sp-3", "uuid-sp-3a", UNMOUNTED);
    vol1.SetPartitionNum(1);
    dm.OnVolumeCreated(vol1);
    VolumeExternal vol2 = MakeUsbVolume("vol-sp-3b", "disk-sp-3", "uuid-sp-3b", UNMOUNTED);
    vol2.SetPartitionNum(2);
    vol2.SetFsType(static_cast<int32_t>(EXT4));
    dm.OnVolumeCreated(vol2);
    std::vector<std::string> content = {
        "Disk /dev/block/disk-sp-3: 16 GiB",
        "Number  Start   End",
        "1 2048 500000",
        "2 500001 1000000",
    };
    PartitionTableInfo info;
    info.SetDiskId("disk-sp-3");
    info.SetSectorSize(512);
    dm.SetPartitions(content, info);
    ASSERT_EQ(info.GetPartitions().size(), 2U);
    EXPECT_EQ(info.GetPartitions()[0].GetPartitionNum(), 1);
    EXPECT_EQ(info.GetPartitions()[0].GetFsType(), "vfat");
    EXPECT_EQ(info.GetPartitions()[1].GetPartitionNum(), 2);
    EXPECT_EQ(info.GetPartitions()[1].GetFsType(), "ext4");
    EXPECT_EQ(info.GetPartitions()[0].GetSizeBytes(), (500000LL - 2048LL + 1) * 512);

    GTEST_LOG_(INFO) << "SetPartitions_MultiPartition_TestCase_003 End";
}

/**
 * @tc.name: NotifyPartitionDone_NoWaiter_TestCase_001
 * @tc.desc: 无等待者时 NotifyPartitionDone 清除分区状态
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyPartitionDone_NoWaiter_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyPartitionDone_NoWaiter_TestCase_001 Start";

    auto &dm = DiskManager::GetInstance();
    dm.NotifyPartitionDone("disk-npd-1");
    EXPECT_FALSE(dm.IsPartitioning("disk-npd-1"));

    GTEST_LOG_(INFO) << "NotifyPartitionDone_NoWaiter_TestCase_001 End";
}

/**
 * @tc.name: WaitForPartitionDone_Timeout_TestCase_001
 * @tc.desc: WaitForPartitionDone 超时后清除分区状态
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, WaitForPartitionDone_Timeout_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "WaitForPartitionDone_Timeout_TestCase_001 Start";

    auto &dm = DiskManager::GetInstance();
    dm.WaitForPartitionDone("disk-wfpd-1", 1);
    EXPECT_FALSE(dm.IsPartitioning("disk-wfpd-1"));

    GTEST_LOG_(INFO) << "WaitForPartitionDone_Timeout_TestCase_001 End";
}

/**
 * @tc.name: WaitForPartitionDone_Notify_TestCase_002
 * @tc.desc: NotifyPartitionDone 唤醒 WaitForPartitionDone
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, WaitForPartitionDone_Notify_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "WaitForPartitionDone_Notify_TestCase_002 Start";

    auto &dm = DiskManager::GetInstance();
    std::thread waiter([&dm]() { dm.WaitForPartitionDone("disk-wfpd-2", 3000); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    dm.NotifyPartitionDone("disk-wfpd-2");
    waiter.join();

    GTEST_LOG_(INFO) << "WaitForPartitionDone_Notify_TestCase_002 End";
}

/**
 * @tc.name: NotifyVolumeEjecting_TestCase_001
 * @tc.desc: NotifyVolumeEjecting 将卷状态置为 EJECTING
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, NotifyVolumeEjecting_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "NotifyVolumeEjecting_TestCase_001 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-nve-1"));
    VolumeExternal vol = MakeUsbVolume("vol-nve-1", "disk-nve-1", "uuid-nve-1", MOUNTED);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-nve-1", volOut);
    EXPECT_EQ(dm.NotifyVolumeEjecting("vol-nve-1", volOut), MOUNTED);
    EXPECT_EQ(volOut.GetState(), EJECTING);

    GTEST_LOG_(INFO) << "NotifyVolumeEjecting_TestCase_001 End";
}

/**
 * @tc.name: RestoreVolumeState_TestCase_001
 * @tc.desc: RestoreVolumeState 恢复卷先前状态
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, RestoreVolumeState_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RestoreVolumeState_TestCase_001 Start";

    auto &dm = DiskManager::GetInstance();
    dm.OnDiskCreated(MakeUsbDisk("disk-rvs-1"));
    VolumeExternal vol = MakeUsbVolume("vol-rvs-1", "disk-rvs-1", "uuid-rvs-1", EJECTING);
    dm.OnVolumeCreated(vol);
    VolumeExternal volOut;
    dm.GetVolumeById("vol-rvs-1", volOut);
    dm.RestoreVolumeState("vol-rvs-1", volOut, MOUNTED);
    EXPECT_EQ(volOut.GetState(), MOUNTED);
    VolumeExternal volCheck;
    dm.GetVolumeById("vol-rvs-1", volCheck);
    EXPECT_EQ(volCheck.GetState(), MOUNTED);

    GTEST_LOG_(INFO) << "RestoreVolumeState_TestCase_001 End";
}

} // namespace DiskManager
} // namespace OHOS