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
#include <sys/mount.h>
#include <sys/statvfs.h>

#include "disk_manager.h"
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
#include "notification/common_event_publisher.h"
#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

namespace {
const int64_t SIZE = 4096;

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

class DiskManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        testing::Mock::AllowLeak(&MockStorageDaemonAdapter::GetInstance());
        testing::Mock::AllowLeak(&MockUsbFuseAdapter::GetInstance());
        testing::Mock::AllowLeak(&MockUeventBootstrap::GetInstance());
        GTEST_LOG_(INFO) << "DiskManagerTest SetUpTestCase";
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "DiskManagerTest TearDownTestCase";
    }

    void SetUp() override
    {
        ClearDiskManagerState();
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
 * @tc.desc: Get non-existent disk returns E_DISK_NOT_FOUND.
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, GetDiskById_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDiskById_TestCase_002 Start";
    auto &dm = DiskManager::GetInstance();
    Disk out;
    EXPECT_EQ(dm.GetDiskById("nonexistent-disk", out), E_DISK_NOT_FOUND);
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
 * @tc.name: EraseVolume_TestCase_001
 * @tc.desc: EraseVolume returns E_OK (stub).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, EraseVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "EraseVolume_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.EraseVolume("any-vol"), E_OK);
    GTEST_LOG_(INFO) << "EraseVolume_TestCase_001 End";
}

/**
 * @tc.name: EjectVolume_TestCase_001
 * @tc.desc: EjectVolume returns E_OK (stub).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, EjectVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "EjectVolume_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.EjectVolume("any-vol"), E_OK);
    GTEST_LOG_(INFO) << "EjectVolume_TestCase_001 End";
}

/**
 * @tc.name: CreateIsoImage_TestCase_001
 * @tc.desc: CreateIsoImage returns E_OK (stub).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, CreateIsoImage_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "CreateIsoImage_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.CreateIsoImage("any-vol", "/path"), E_OK);
    GTEST_LOG_(INFO) << "CreateIsoImage_TestCase_001 End";
}

/**
 * @tc.name: BurnVolume_TestCase_001
 * @tc.desc: BurnVolume returns E_OK (stub).
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(DiskManagerTest, BurnVolume_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "BurnVolume_TestCase_001 Start";
    auto &dm = DiskManager::GetInstance();
    EXPECT_EQ(dm.BurnVolume("any-vol", "options"), E_OK);
    GTEST_LOG_(INFO) << "BurnVolume_TestCase_001 End";
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
    EXPECT_EQ(dm.GetVolumeOpProcess("any-vol", progress), E_OK);
    EXPECT_EQ(progress, 0);
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
    EXPECT_EQ(dm.VerifyBurnData("any-vol", 0), E_OK);
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
    EXPECT_EQ(dm.FormatPartition("disk-fp-2", 1, params), E_DELETE_PARTITION_NOT_SUPPORT);
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

} // namespace DiskManager
} // namespace OHOS