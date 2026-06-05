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

#include "disk.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class DiskTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(DiskTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    Disk disk;
    EXPECT_EQ(disk.GetDiskId(), "");
    EXPECT_EQ(disk.GetSizeBytes(), 0);
    EXPECT_EQ(disk.GetDiskType(), DISK_TYPE_UNKNOWN);
    EXPECT_TRUE(disk.GetRemovable());
    EXPECT_TRUE(disk.IsRemovable());
    EXPECT_EQ(disk.GetSysPath(), "");
    EXPECT_FALSE(disk.IsInternalDataDisk());
}

HWTEST_F(DiskTest, ParameterizedConstructor_TestCase_001, TestSize.Level0)
{
    Disk disk("disk-1", 4096, "sda1", USB_FLAG);
    EXPECT_EQ(disk.GetDiskId(), "disk-1");
    EXPECT_EQ(disk.GetSizeBytes(), 4096);
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
    EXPECT_TRUE(disk.GetRemovable());
    EXPECT_EQ(disk.GetSysPath(), "/dev/block/sda1");
}

HWTEST_F(DiskTest, ParameterizedConstructor_TestCase_002, TestSize.Level0)
{
    Disk disk("disk-ssd", 1024, "nvme0n1", DATA_DISK_SSD);
    EXPECT_EQ(disk.GetDiskType(), DATA_DISK_SSD);
    EXPECT_FALSE(disk.GetRemovable());
    EXPECT_TRUE(disk.IsInternalDataDisk());
}

HWTEST_F(DiskTest, ParameterizedConstructor_TestCase_003, TestSize.Level0)
{
    Disk disk("disk-hdd", 2048, "sda", DATA_DISK_HDD);
    EXPECT_EQ(disk.GetDiskType(), DATA_DISK_HDD);
    EXPECT_FALSE(disk.GetRemovable());
    EXPECT_TRUE(disk.IsInternalDataDisk());
}

HWTEST_F(DiskTest, GetDiskId_TestCase_001, TestSize.Level0)
{
    Disk disk("test-id", 0, "", DISK_TYPE_UNKNOWN);
    EXPECT_EQ(disk.GetDiskId(), "test-id");
}

HWTEST_F(DiskTest, GetSizeBytes_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 999, "", DISK_TYPE_UNKNOWN);
    EXPECT_EQ(disk.GetSizeBytes(), 999);
}

HWTEST_F(DiskTest, SetSizeBytes_TestCase_001, TestSize.Level0)
{
    Disk disk;
    disk.SetSizeBytes(12345);
    EXPECT_EQ(disk.GetSizeBytes(), 12345);
}

HWTEST_F(DiskTest, GetDiskType_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", CD_FLAG);
    EXPECT_EQ(disk.GetDiskType(), CD_FLAG);
}

HWTEST_F(DiskTest, SetDiskType_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", USB_FLAG);
    EXPECT_TRUE(disk.GetRemovable());
    disk.SetDiskType(DATA_DISK_SSD);
    EXPECT_EQ(disk.GetDiskType(), DATA_DISK_SSD);
    EXPECT_FALSE(disk.GetRemovable());
}

HWTEST_F(DiskTest, SetDiskType_TestCase_002, TestSize.Level0)
{
    Disk disk("d", 0, "", DATA_DISK_SSD);
    EXPECT_FALSE(disk.GetRemovable());
    disk.SetDiskType(USB_FLAG);
    EXPECT_TRUE(disk.GetRemovable());
}

HWTEST_F(DiskTest, GetRemovable_TestCase_001, TestSize.Level0)
{
    Disk diskUsb("d", 0, "", USB_FLAG);
    EXPECT_TRUE(diskUsb.GetRemovable());
    Disk diskSsd("d", 0, "", DATA_DISK_SSD);
    EXPECT_FALSE(diskSsd.GetRemovable());
}

HWTEST_F(DiskTest, IsRemovable_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", SD_FLAG);
    EXPECT_TRUE(disk.IsRemovable());
    EXPECT_EQ(disk.IsRemovable(), disk.GetRemovable());
}

HWTEST_F(DiskTest, SetVolumeIds_TestCase_001, TestSize.Level0)
{
    Disk disk;
    std::vector<std::string> ids = {"vol-1", "vol-2"};
    disk.SetVolumeIds(ids);
    EXPECT_EQ(disk.GetVolumeIds().size(), 2u);
    EXPECT_EQ(disk.GetVolumeIds()[0], "vol-1");
}

HWTEST_F(DiskTest, SetVolumeIds_Move_TestCase_001, TestSize.Level0)
{
    Disk disk;
    std::vector<std::string> ids = {"vol-a", "vol-b"};
    disk.SetVolumeIds(std::move(ids));
    EXPECT_EQ(disk.GetVolumeIds().size(), 2u);
    EXPECT_EQ(disk.GetVolumeIds()[1], "vol-b");
}

HWTEST_F(DiskTest, GetVolumeIds_Default_TestCase_001, TestSize.Level0)
{
    Disk disk;
    EXPECT_EQ(disk.GetVolumeIds().size(), 0u);
}

HWTEST_F(DiskTest, SetExtraInfo_TestCase_001, TestSize.Level0)
{
    Disk disk;
    disk.SetExtraInfo("extra-data");
    EXPECT_EQ(disk.GetExtraInfo(), "extra-data");
}

HWTEST_F(DiskTest, GetExtraInfo_Default_TestCase_001, TestSize.Level0)
{
    Disk disk;
    EXPECT_EQ(disk.GetExtraInfo(), "");
}

HWTEST_F(DiskTest, GetSysPath_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "block1", USB_FLAG);
    EXPECT_EQ(disk.GetSysPath(), "/dev/block/block1");
}

HWTEST_F(DiskTest, IsInternalDataDisk_TestCase_001, TestSize.Level0)
{
    Disk diskSsd("d", 0, "", DATA_DISK_SSD);
    EXPECT_TRUE(diskSsd.IsInternalDataDisk());
    Disk diskHdd("d", 0, "", DATA_DISK_HDD);
    EXPECT_TRUE(diskHdd.IsInternalDataDisk());
    Disk diskUsb("d", 0, "", USB_FLAG);
    EXPECT_FALSE(diskUsb.IsInternalDataDisk());
    Disk diskSd("d", 0, "", SD_FLAG);
    EXPECT_FALSE(diskSd.IsInternalDataDisk());
    Disk diskCd("d", 0, "", CD_FLAG);
    EXPECT_FALSE(diskCd.IsInternalDataDisk());
}

HWTEST_F(DiskTest, UpdateRemovableFromDiskType_TestCase_001, TestSize.Level0)
{
    Disk disk;
    EXPECT_TRUE(disk.GetRemovable());
    disk.SetDiskType(DATA_DISK_SSD);
    EXPECT_FALSE(disk.GetRemovable());
    disk.SetDiskType(DATA_DISK_HDD);
    EXPECT_FALSE(disk.GetRemovable());
    disk.SetDiskType(USB_FLAG);
    EXPECT_TRUE(disk.GetRemovable());
    disk.SetDiskType(DISK_TYPE_UNKNOWN);
    EXPECT_TRUE(disk.GetRemovable());
}

HWTEST_F(DiskTest, RefreshClassification_DvrUsb_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DVR_USB);
    disk.RefreshClassificationFromSysfs("/sys/devices/usb/test");
    EXPECT_EQ(disk.GetDiskType(), DVR_USB);
    EXPECT_TRUE(disk.GetRemovable());
}

HWTEST_F(DiskTest, RefreshClassification_CdFlag_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", CD_FLAG);
    disk.RefreshClassificationFromSysfs("/sys/devices/block/sr0");
    EXPECT_EQ(disk.GetDiskType(), CD_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_OpticalBlock_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", USB_FLAG);
    disk.RefreshClassificationFromSysfs("/sys/devices/block/sr0/device");
    EXPECT_EQ(disk.GetDiskType(), CD_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_Nvme_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", USB_FLAG);
    disk.RefreshClassificationFromSysfs("/sys/devices/nvme/nvme0");
    EXPECT_EQ(disk.GetDiskType(), DATA_DISK_SSD);
    EXPECT_FALSE(disk.GetRemovable());
}

HWTEST_F(DiskTest, RefreshClassification_DirectSata_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", USB_FLAG);
    disk.RefreshClassificationFromSysfs("/sys/devices/ata/ahci0");
    EXPECT_EQ(disk.GetDiskType(), DATA_DISK_HDD);
    EXPECT_FALSE(disk.GetRemovable());
}

HWTEST_F(DiskTest, RefreshClassification_DirectSata2_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", USB_FLAG);
    disk.RefreshClassificationFromSysfs("/sys/devices/.sata/host0");
    EXPECT_EQ(disk.GetDiskType(), DATA_DISK_HDD);
}

HWTEST_F(DiskTest, RefreshClassification_Usb_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/usb/usb1");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
    EXPECT_TRUE(disk.GetRemovable());
}

HWTEST_F(DiskTest, RefreshClassification_UsbHost_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/usbhost/1-1");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_HiUsb_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/hiusb/hiusb-otg");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_PlatformHiUsb_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/platform/hiusb/hiusb-otg");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_Ehci_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/.ehci/ehci0");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_Ehci2_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/ehci/ehci-hcd");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_Ehci3_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/ehci/ehci_hcd.0");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_Xhci_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/xhci/xhci-hcd");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_Dwc3_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/.dwc3/dwc3.0");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_Dwc3Path_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/dwc3/dwc3.0");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_MmcHost_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/mmc_host/mmc0");
    EXPECT_EQ(disk.GetDiskType(), SD_FLAG);
    EXPECT_TRUE(disk.GetRemovable());
}

HWTEST_F(DiskTest, RefreshClassification_Dwmmc_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/dwmmc0");
    EXPECT_EQ(disk.GetDiskType(), SD_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_HiMci_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/hi_mci.0");
    EXPECT_EQ(disk.GetDiskType(), SD_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_DwMmc_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/dw_mmc/host0");
    EXPECT_EQ(disk.GetDiskType(), SD_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_EmptySysPath_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", USB_FLAG);
    disk.RefreshClassificationFromSysfs("");
    EXPECT_EQ(disk.GetDiskType(), USB_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_EmptySysPath_Unknown_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("");
    EXPECT_EQ(disk.GetDiskType(), DISK_TYPE_UNKNOWN);
}

HWTEST_F(DiskTest, RefreshClassification_EmptySysPath_Sd_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", SD_FLAG);
    disk.RefreshClassificationFromSysfs("");
    EXPECT_EQ(disk.GetDiskType(), SD_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_EmptySysPath_Cd_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", CD_FLAG);
    disk.RefreshClassificationFromSysfs("");
    EXPECT_EQ(disk.GetDiskType(), CD_FLAG);
}

HWTEST_F(DiskTest, RefreshClassification_NoMatch_TestCase_001, TestSize.Level0)
{
    Disk disk("d", 0, "", DISK_TYPE_UNKNOWN);
    disk.RefreshClassificationFromSysfs("/sys/devices/virtual/block/loop0");
    EXPECT_EQ(disk.GetDiskType(), DISK_TYPE_UNKNOWN);
}

HWTEST_F(DiskTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    Disk disk("disk-1", 4096, "sda1", USB_FLAG);
    disk.SetVolumeIds({"vol-1", "vol-2"});
    disk.SetExtraInfo("extra");
    Parcel parcel;
    EXPECT_TRUE(disk.Marshalling(parcel));
}

HWTEST_F(DiskTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    Disk disk("disk-1", 4096, "sda1", USB_FLAG);
    disk.SetVolumeIds({"vol-1"});
    disk.SetExtraInfo("extra-info");
    Parcel parcel;
    EXPECT_TRUE(disk.Marshalling(parcel));
    Disk *result = Disk::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetDiskId(), "disk-1");
    EXPECT_EQ(result->GetSizeBytes(), 4096);
    EXPECT_EQ(result->GetDiskType(), USB_FLAG);
    EXPECT_TRUE(result->GetRemovable());
    EXPECT_EQ(result->GetVolumeIds().size(), 1u);
    EXPECT_EQ(result->GetVolumeIds()[0], "vol-1");
    EXPECT_EQ(result->GetExtraInfo(), "extra-info");
    delete result;
}

HWTEST_F(DiskTest, Unmarshalling_EmptyVolumeIds_TestCase_001, TestSize.Level0)
{
    Disk disk("disk-2", 0, "", SD_FLAG);
    Parcel parcel;
    EXPECT_TRUE(disk.Marshalling(parcel));
    Disk *result = Disk::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetVolumeIds().size(), 0u);
    delete result;
}

HWTEST_F(DiskTest, Unmarshalling_OverMaxVolumeIds_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("disk-id");
    parcel.WriteInt64(0);
    parcel.WriteInt32(USB_FLAG);
    parcel.WriteBool(true);
    parcel.WriteUint32(257u);
    Disk *result = Disk::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

} // namespace DiskManager
} // namespace OHOS