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
 
#include "disk/sysfs_reader.h"
 
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <string>
#include <unistd.h>
 
namespace OHOS {
namespace DiskManager {
 
using namespace testing::ext;
 
namespace {
 
// 创建临时目录模拟 sysfs 设备树
class SysfsTree {
public:
    std::string root_;
 
    explicit SysfsTree(const std::string &prefix)
    {
        char tmpl[] = "/tmp/sysfs_test_XXXXXX";
        root_ = mkdtemp(tmpl);
    }
 
    ~SysfsTree()
    {
        if (!root_.empty()) {
            std::string cmd = "rm -rf " + root_;
            (void)system(cmd.c_str());
        }
    }
 
    // 创建目录
    void Mkdir(const std::string &relPath)
    {
        std::string full = root_ + relPath;
        std::string cmd = "mkdir -p " + full;
        (void)system(cmd.c_str());
    }
 
    // 写入属性文件
    void WriteFile(const std::string &relPath, const std::string &content)
    {
        std::string full = root_ + relPath;
        std::ofstream ofs(full);
        if (ofs.is_open()) {
            ofs << content;
        }
    }
 
    // 创建 uevent 文件
    void WriteUevent(const std::string &relDir, const std::string &content)
    {
        WriteFile(relDir + "/uevent", content);
    }
 
    // 创建 subsystem 符号链接（目标为相对路径）
    void SymlinkSubsys(const std::string &relDir, const std::string &subsysName)
    {
        // 先确保 bus 目录存在
        Mkdir("/bus/" + subsysName);
        std::string linkPath = root_ + relDir + "/subsystem";
        std::string target = root_ + "/bus/" + subsysName;
        // 使用相对路径创建符号链接
        std::string cmd = "ln -sfn " + target + " " + linkPath;
        (void)system(cmd.c_str());
    }
 
    // 写入 sysfs 属性（自动加换行）
    void WriteAttr(const std::string &relDir, const std::string &attr, const std::string &value)
    {
        WriteFile(relDir + "/" + attr, value + "\n");
    }
 
    // 构建标准 USB 存储设备树（SCSI -> usb_interface -> usb_device）
    void BuildUsbStorageTree(const std::string &vid = "0781", const std::string &pid = "5581",
                             const std::string &serial = "ABC123", const std::string &busnum = "2",
                             const std::string &devnum = "3")
    {
        // usb2 (host controller)
        Mkdir("/usb2");
        WriteUevent("/usb2", "MAJOR=189\nMINOR=0\nDEVTYPE=usb_device\n");
        SymlinkSubsys("/usb2", "usb");
        WriteAttr("/usb2", "busnum", "2");
        WriteAttr("/usb2", "devnum", "1");
        WriteAttr("/usb2", "idVendor", "1d6b");
        WriteAttr("/usb2", "idProduct", "0002");
 
        // 2-1 (USB storage device)
        Mkdir("/usb2/2-1");
        WriteUevent("/usb2/2-1",
                    "MAJOR=189\nMINOR=1\nDEVTYPE=usb_device\nDRIVER=usb\n"
                    "PRODUCT=" + vid + "/" + pid + "/100\nBUSNUM=002\nDEVNUM=" + devnum + "\n");
        SymlinkSubsys("/usb2/2-1", "usb");
        WriteAttr("/usb2/2-1", "idVendor", vid);
        WriteAttr("/usb2/2-1", "idProduct", pid);
        WriteAttr("/usb2/2-1", "serial", serial);
        WriteAttr("/usb2/2-1", "busnum", busnum);
        WriteAttr("/usb2/2-1", "devnum", devnum);
 
        // 2-1:1.0 (USB interface)
        Mkdir("/usb2/2-1/2-1:1.0");
        WriteUevent("/usb2/2-1/2-1:1.0", "DEVTYPE=usb_interface\nDRIVER=usb-storage\n");
        SymlinkSubsys("/usb2/2-1/2-1:1.0", "usb");
 
        // host0/target0:0:0/0:0:0:0 (SCSI)
        Mkdir("/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0");
        WriteUevent("/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0",
                    "SUBSYSTEM=scsi\nDEVTYPE=scsi_device\n");
        SymlinkSubsys("/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0", "scsi");
 
        // block/sda
        Mkdir("/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0/block/sda");
        WriteUevent("/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0/block/sda",
                    "MAJOR=8\nMINOR=0\nDEVTYPE=disk\n");
        SymlinkSubsys("/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0/block/sda", "block");
    }
 
    // 构建非 USB 设备树（SATA/virtio）
    void BuildNonUsbTree()
    {
        Mkdir("/pci0000:00/0000:00:1f.2/ata1/host1/target1:0:0/1:0:0:0");
        WriteUevent("/pci0000:00/0000:00:1f.2/ata1/host1/target1:0:0/1:0:0:0",
                    "SUBSYSTEM=scsi\nDEVTYPE=scsi_device\n");
        SymlinkSubsys("/pci0000:00/0000:00:1f.2/ata1/host1/target1:0:0/1:0:0:0", "scsi");
 
        Mkdir("/pci0000:00/0000:00:1f.2/ata1/host1/target1:0:0/1:0:0:0/block/sdb");
        WriteUevent("/pci0000:00/0000:00:1f.2/ata1/host1/target1:0:0/1:0:0:0/block/sdb",
                    "MAJOR=8\nMINOR=16\nDEVTYPE=disk\n");
        SymlinkSubsys("/pci0000:00/0000:00:1f.2/ata1/host1/target1:0:0/1:0:0:0/block/sdb", "block");
    }
 
    // 构建多级 Hub 场景：Hub(2-1) + Storage(2-1.2)
    void BuildHubTree()
    {
        // usb2 (host controller)
        Mkdir("/usb2");
        WriteUevent("/usb2", "DEVTYPE=usb_device\n");
        SymlinkSubsys("/usb2", "usb");
        WriteAttr("/usb2", "idVendor", "1d6b");
        WriteAttr("/usb2", "idProduct", "0002");
 
        // 2-1 (Hub)
        Mkdir("/usb2/2-1");
        WriteUevent("/usb2/2-1", "DEVTYPE=usb_device\n");
        SymlinkSubsys("/usb2/2-1", "usb");
        WriteAttr("/usb2/2-1", "idVendor", "1a2b");
        WriteAttr("/usb2/2-1", "idProduct", "3c4d");
        WriteAttr("/usb2/2-1", "busnum", "2");
        WriteAttr("/usb2/2-1", "devnum", "2");
 
        // 2-1.2 (USB storage behind hub)
        Mkdir("/usb2/2-1/2-1.2");
        WriteUevent("/usb2/2-1/2-1.2", "DEVTYPE=usb_device\n");
        SymlinkSubsys("/usb2/2-1/2-1.2", "usb");
        WriteAttr("/usb2/2-1/2-1.2", "idVendor", "0123");
        WriteAttr("/usb2/2-1/2-1.2", "idProduct", "4567");
        WriteAttr("/usb2/2-1/2-1.2", "serial", "HUB-SN-001");
        WriteAttr("/usb2/2-1/2-1.2", "busnum", "2");
        WriteAttr("/usb2/2-1/2-1.2", "devnum", "5");
 
        // 2-1.2:1.0 (USB interface)
        Mkdir("/usb2/2-1/2-1.2/2-1.2:1.0");
        WriteUevent("/usb2/2-1/2-1.2/2-1.2:1.0", "DEVTYPE=usb_interface\n");
        SymlinkSubsys("/usb2/2-1/2-1.2/2-1.2:1.0", "usb");
 
        // SCSI + block
        Mkdir("/usb2/2-1/2-1.2/2-1.2:1.0/host2/target2:0:0/2:0:0:0");
        WriteUevent("/usb2/2-1/2-1.2/2-1.2:1.0/host2/target2:0:0/2:0:0:0",
                    "DEVTYPE=scsi_device\n");
        SymlinkSubsys("/usb2/2-1/2-1.2/2-1.2:1.0/host2/target2:0:0/2:0:0:0", "scsi");
 
        Mkdir("/usb2/2-1/2-1.2/2-1.2:1.0/host2/target2:0:0/2:0:0:0/block/sdc");
        WriteUevent("/usb2/2-1/2-1.2/2-1.2:1.0/host2/target2:0:0/2:0:0:0/block/sdc",
                    "DEVTYPE=disk\n");
        SymlinkSubsys("/usb2/2-1/2-1.2/2-1.2:1.0/host2/target2:0:0/2:0:0:0/block/sdc", "block");
    }
 
    std::string BlockPath(const std::string &relBlockPath)
    {
        return root_ + relBlockPath;
    }
};
 
} // namespace
 
class SysfsReaderTest : public testing::Test {
public:
    void TearDown() override {}
};
 
// ============ StripBlockSuffix ============
 
/**
 * @tc.name: StripBlockSuffix_Standard_TestCase_001
 * @tc.desc: sysPath含/block/sda后缀时正确剥离
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, StripBlockSuffix_Standard_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "StripBlockSuffix_Standard_TestCase_001 Start";
    auto result = SysfsReader::StripBlockSuffix(
        "/sys/devices/pci/usb2/2-1/2-1:1.0/host0/0:0:0:0/block/sda", "sda");
    EXPECT_EQ(result, "/sys/devices/pci/usb2/2-1/2-1:1.0/host0/0:0:0:0");
    GTEST_LOG_(INFO) << "StripBlockSuffix_Standard_TestCase_001 End";
}
 
/**
 * @tc.name: StripBlockSuffix_NoBlockSuffix_TestCase_002
 * @tc.desc: sysPath无/block/后缀时fallback取dirname
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, StripBlockSuffix_NoBlockSuffix_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "StripBlockSuffix_NoBlockSuffix_TestCase_002 Start";
    auto result = SysfsReader::StripBlockSuffix("/sys/devices/pci/0:0:0:0", "sda");
    EXPECT_EQ(result, "/sys/devices/pci");
    GTEST_LOG_(INFO) << "StripBlockSuffix_NoBlockSuffix_TestCase_002 End";
}
 
// ============ ReadSysfsFile ============
 
/**
 * @tc.name: ReadSysfsFile_Normal_TestCase_003
 * @tc.desc: 正常属性文件读取
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadSysfsFile_Normal_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadSysfsFile_Normal_TestCase_003 Start";
    SysfsTree tree("normal");
    tree.Mkdir("/dev");
    tree.WriteAttr("/dev", "idVendor", "0781\n");
    auto result = SysfsReader::ReadSysfsFile(tree.root_ + "/dev/idVendor");
    EXPECT_EQ(result, "0781");
    GTEST_LOG_(INFO) << "ReadSysfsFile_Normal_TestCase_003 End";
}
 
/**
 * @tc.name: ReadSysfsFile_NotExist_TestCase_004
 * @tc.desc: 文件不存在返回空串
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadSysfsFile_NotExist_TestCase_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadSysfsFile_NotExist_TestCase_004 Start";
    auto result = SysfsReader::ReadSysfsFile("/nonexistent/path/idVendor");
    EXPECT_EQ(result, "");
    GTEST_LOG_(INFO) << "ReadSysfsFile_NotExist_TestCase_004 End";
}
 
/**
 * @tc.name: ReadSysfsFile_EmptyFile_TestCase_005
 * @tc.desc: 空文件返回空串
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadSysfsFile_EmptyFile_TestCase_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadSysfsFile_EmptyFile_TestCase_005 Start";
    SysfsTree tree("empty");
    tree.Mkdir("/dev");
    tree.WriteFile("/dev/attr", "");
    auto result = SysfsReader::ReadSysfsFile(tree.root_ + "/dev/attr");
    EXPECT_EQ(result, "");
    GTEST_LOG_(INFO) << "ReadSysfsFile_EmptyFile_TestCase_005 End";
}
 
// ============ ReadSubsystemSymlink ============
 
/**
 * @tc.name: ReadSubsystemSymlink_Usb_TestCase_006
 * @tc.desc: subsystem符号链接指向usb总线
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadSubsystemSymlink_Usb_TestCase_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadSubsystemSymlink_Usb_TestCase_006 Start";
    SysfsTree tree("subsys_usb");
    tree.Mkdir("/dev");
    tree.SymlinkSubsys("/dev", "usb");
    auto result = SysfsReader::ReadSubsystemSymlink(tree.root_ + "/dev");
    EXPECT_EQ(result, "usb");
    GTEST_LOG_(INFO) << "ReadSubsystemSymlink_Usb_TestCase_006 End";
}
 
/**
 * @tc.name: ReadSubsystemSymlink_Scsi_TestCase_007
 * @tc.desc: subsystem符号链接指向scsi总线
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadSubsystemSymlink_Scsi_TestCase_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadSubsystemSymlink_Scsi_TestCase_007 Start";
    SysfsTree tree("subsys_scsi");
    tree.Mkdir("/dev");
    tree.SymlinkSubsys("/dev", "scsi");
    auto result = SysfsReader::ReadSubsystemSymlink(tree.root_ + "/dev");
    EXPECT_EQ(result, "scsi");
    GTEST_LOG_(INFO) << "ReadSubsystemSymlink_Scsi_TestCase_007 End";
}
 
/**
 * @tc.name: ReadSubsystemSymlink_NoLink_TestCase_008
 * @tc.desc: 无subsystem符号链接返回空串
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadSubsystemSymlink_NoLink_TestCase_008, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadSubsystemSymlink_NoLink_TestCase_008 Start";
    SysfsTree tree("subsys_none");
    tree.Mkdir("/dev");
    auto result = SysfsReader::ReadSubsystemSymlink(tree.root_ + "/dev");
    EXPECT_EQ(result, "");
    GTEST_LOG_(INFO) << "ReadSubsystemSymlink_NoLink_TestCase_008 End";
}
 
// ============ ReadUeventField ============
 
/**
 * @tc.name: ReadUeventField_DevType_TestCase_009
 * @tc.desc: uevent文件含DEVTYPE=usb_device
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUeventField_DevType_TestCase_009, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUeventField_DevType_TestCase_009 Start";
    SysfsTree tree("uevent_devtype");
    tree.Mkdir("/dev");
    tree.WriteUevent("/dev", "MAJOR=189\nDEVTYPE=usb_device\nDRIVER=usb\n");
    auto result = SysfsReader::ReadUeventField(tree.root_ + "/dev", "DEVTYPE");
    EXPECT_EQ(result, "usb_device");
    GTEST_LOG_(INFO) << "ReadUeventField_DevType_TestCase_009 End";
}
 
/**
 * @tc.name: ReadUeventField_FieldNotFound_TestCase_010
 * @tc.desc: uevent文件不包含目标字段
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUeventField_FieldNotFound_TestCase_010, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUeventField_FieldNotFound_TestCase_010 Start";
    SysfsTree tree("uevent_nofield");
    tree.Mkdir("/dev");
    tree.WriteUevent("/dev", "MAJOR=8\nMINOR=0\nDEVTYPE=disk\n");
    auto result = SysfsReader::ReadUeventField(tree.root_ + "/dev", "SERIAL");
    EXPECT_EQ(result, "");
    GTEST_LOG_(INFO) << "ReadUeventField_FieldNotFound_TestCase_010 End";
}
 
/**
 * @tc.name: ReadUeventField_NoUeventFile_TestCase_011
 * @tc.desc: 无uevent文件返回空串
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUeventField_NoUeventFile_TestCase_011, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUeventField_NoUeventFile_TestCase_011 Start";
    SysfsTree tree("uevent_nofile");
    tree.Mkdir("/dev");
    auto result = SysfsReader::ReadUeventField(tree.root_ + "/dev", "DEVTYPE");
    EXPECT_EQ(result, "");
    GTEST_LOG_(INFO) << "ReadUeventField_NoUeventFile_TestCase_011 End";
}
 
// ============ HasUeventFile ============
 
/**
 * @tc.name: HasUeventFile_Exists_TestCase_012
 * @tc.desc: 目录有uevent文件返回true
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, HasUeventFile_Exists_TestCase_012, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "HasUeventFile_Exists_TestCase_012 Start";
    SysfsTree tree("has_uevent");
    tree.Mkdir("/dev");
    tree.WriteUevent("/dev", "MAJOR=8\n");
    EXPECT_TRUE(SysfsReader::HasUeventFile(tree.root_ + "/dev"));
    GTEST_LOG_(INFO) << "HasUeventFile_Exists_TestCase_012 End";
}
 
/**
 * @tc.name: HasUeventFile_NotExists_TestCase_013
 * @tc.desc: 目录无uevent文件返回false
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, HasUeventFile_NotExists_TestCase_013, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "HasUeventFile_NotExists_TestCase_013 Start";
    SysfsTree tree("no_uevent");
    tree.Mkdir("/dev");
    EXPECT_FALSE(SysfsReader::HasUeventFile(tree.root_ + "/dev"));
    GTEST_LOG_(INFO) << "HasUeventFile_NotExists_TestCase_013 End";
}
 
// ============ ReadUsbInfo 集成测试 ============
 
/**
 * @tc.name: ReadUsbInfo_UsbStorageDevice_TestCase_014
 * @tc.desc: 完整USB存储设备树能正确读取vid/pid/serial/busnum/devnum
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_UsbStorageDevice_TestCase_014, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_UsbStorageDevice_TestCase_014 Start";
    SysfsTree tree("usb_storage");
    tree.BuildUsbStorageTree("0781", "5581", "ABC123", "2", "3");
 
    std::string sysPath = tree.BlockPath(
        "/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0/block/sda");
    auto info = SysfsReader::ReadUsbInfo(sysPath, "sda");
 
    EXPECT_EQ(info.vid, "0781");
    EXPECT_EQ(info.pid, "5581");
    EXPECT_EQ(info.serialNumber, "ABC123");
    EXPECT_EQ(info.busnum, "2");
    EXPECT_EQ(info.devnum, "3");
    GTEST_LOG_(INFO) << "ReadUsbInfo_UsbStorageDevice_TestCase_014 End";
}
 
/**
 * @tc.name: ReadUsbInfo_NonUsbDevice_TestCase_015
 * @tc.desc: 非USB设备返回全空UsbSysfsInfo
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_NonUsbDevice_TestCase_015, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_NonUsbDevice_TestCase_015 Start";
    SysfsTree tree("non_usb");
    tree.BuildNonUsbTree();
 
    std::string sysPath = tree.BlockPath(
        "/pci0000:00/0000:00:1f.2/ata1/host1/target1:0:0/1:0:0:0/block/sdb");
    auto info = SysfsReader::ReadUsbInfo(sysPath, "sdb");
 
    EXPECT_TRUE(info.vid.empty());
    EXPECT_TRUE(info.pid.empty());
    EXPECT_TRUE(info.serialNumber.empty());
    EXPECT_TRUE(info.busnum.empty());
    EXPECT_TRUE(info.devnum.empty());
    GTEST_LOG_(INFO) << "ReadUsbInfo_NonUsbDevice_TestCase_015 End";
}
 
/**
 * @tc.name: ReadUsbInfo_UsbInterfaceSkipped_TestCase_016
 * @tc.desc: usb_interface节点被跳过，命中上层usb_device
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_UsbInterfaceSkipped_TestCase_016, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_UsbInterfaceSkipped_TestCase_016 Start";
    SysfsTree tree("skip_iface");
    tree.BuildUsbStorageTree("1234", "5678", "IFACE-TEST", "1", "2");
 
    // 从usb_interface层级开始walk up（跳过block和scsi层）
    std::string ifacePath = tree.root_ + "/usb2/2-1/2-1:1.0";
    auto info = SysfsReader::WalkUpAndReadUsbAttrs(ifacePath);
 
    EXPECT_EQ(info.vid, "1234");
    EXPECT_EQ(info.pid, "5678");
    GTEST_LOG_(INFO) << "ReadUsbInfo_UsbInterfaceSkipped_TestCase_016 End";
}
 
/**
 * @tc.name: ReadUsbInfo_MultiLevelHub_TestCase_017
 * @tc.desc: 多级Hub场景下匹配存储设备而非Hub
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_MultiLevelHub_TestCase_017, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_MultiLevelHub_TestCase_017 Start";
    SysfsTree tree("hub");
    tree.BuildHubTree();
 
    std::string sysPath = tree.BlockPath(
        "/usb2/2-1/2-1.2/2-1.2:1.0/host2/target2:0:0/2:0:0:0/block/sdc");
    auto info = SysfsReader::ReadUsbInfo(sysPath, "sdc");
 
    EXPECT_EQ(info.vid, "0123");
    EXPECT_EQ(info.pid, "4567");
    EXPECT_EQ(info.serialNumber, "HUB-SN-001");
    GTEST_LOG_(INFO) << "ReadUsbInfo_MultiLevelHub_TestCase_017 End";
}
 
/**
 * @tc.name: ReadUsbInfo_EmptySysPath_TestCase_018
 * @tc.desc: sysPath为空返回空UsbSysfsInfo
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_EmptySysPath_TestCase_018, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_EmptySysPath_TestCase_018 Start";
    auto info = SysfsReader::ReadUsbInfo("", "sda");
    EXPECT_TRUE(info.vid.empty());
    GTEST_LOG_(INFO) << "ReadUsbInfo_EmptySysPath_TestCase_018 End";
}
 
/**
 * @tc.name: ReadUsbInfo_EmptyDevName_TestCase_019
 * @tc.desc: devName为空返回空UsbSysfsInfo
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_EmptyDevName_TestCase_019, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_EmptyDevName_TestCase_019 Start";
    auto info = SysfsReader::ReadUsbInfo("/sys/block/sda", "");
    EXPECT_TRUE(info.vid.empty());
    GTEST_LOG_(INFO) << "ReadUsbInfo_EmptyDevName_TestCase_019 End";
}
 
/**
 * @tc.name: ReadUsbInfo_NoSerialAttr_TestCase_020
 * @tc.desc: USB设备无serial属性文件时serialNumber为空，其余正常
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_NoSerialAttr_TestCase_020, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_NoSerialAttr_TestCase_020 Start";
    SysfsTree tree("no_serial");
    tree.BuildUsbStorageTree("0781", "5581", "HAS-SERIAL", "2", "3");
    // 删除 serial 文件
    std::string serialPath = tree.root_ + "/usb2/2-1/serial";
    (void)unlink(serialPath.c_str());
 
    std::string sysPath = tree.BlockPath(
        "/usb2/2-1/2-1:1.0/host0/target0:0:0/0:0:0:0/block/sda");
    auto info = SysfsReader::ReadUsbInfo(sysPath, "sda");
 
    EXPECT_EQ(info.vid, "0781");
    EXPECT_EQ(info.pid, "5581");
    EXPECT_TRUE(info.serialNumber.empty());
    EXPECT_EQ(info.busnum, "2");
    EXPECT_EQ(info.devnum, "3");
    GTEST_LOG_(INFO) << "ReadUsbInfo_NoSerialAttr_TestCase_020 End";
}
 
/**
 * @tc.name: ReadUsbInfo_SubsystemFromSymlink_TestCase_021
 * @tc.desc: uevent文件无SUBSYSTEM行但subsystem符号链接指向usb，仍能正确匹配
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(SysfsReaderTest, ReadUsbInfo_SubsystemFromSymlink_TestCase_021, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ReadUsbInfo_SubsystemFromSymlink_TestCase_021 Start";
    SysfsTree tree("subsys_link");
    // 手动构建：uevent中不含SUBSYSTEM=行（与真实内核行为一致）
    tree.Mkdir("/usb2");
    tree.WriteUevent("/usb2", "MAJOR=189\nMINOR=0\nDEVTYPE=usb_device\n");
    tree.SymlinkSubsys("/usb2", "usb");
    tree.WriteAttr("/usb2", "busnum", "2");
    tree.WriteAttr("/usb2", "devnum", "1");
    tree.WriteAttr("/usb2", "idVendor", "1d6b");
    tree.WriteAttr("/usb2", "idProduct", "0002");
 
    tree.Mkdir("/usb2/2-1");
    // 注意：uevent中无SUBSYSTEM=行
    tree.WriteUevent("/usb2/2-1",
                     "MAJOR=189\nMINOR=1\nDEVTYPE=usb_device\nDRIVER=usb\nBUSNUM=002\nDEVNUM=003\n");
    tree.SymlinkSubsys("/usb2/2-1", "usb");
    tree.WriteAttr("/usb2/2-1", "idVendor", "0627");
    tree.WriteAttr("/usb2/2-1", "idProduct", "0001");
    tree.WriteAttr("/usb2/2-1", "busnum", "2");
    tree.WriteAttr("/usb2/2-1", "devnum", "3");
 
    // 从2-1直接walk
    auto info = SysfsReader::WalkUpAndReadUsbAttrs(tree.root_ + "/usb2/2-1");
 
    EXPECT_EQ(info.vid, "0627");
    EXPECT_EQ(info.pid, "0001");
    EXPECT_EQ(info.busnum, "2");
    EXPECT_EQ(info.devnum, "3");
    GTEST_LOG_(INFO) << "ReadUsbInfo_SubsystemFromSymlink_TestCase_021 End";
}
 
} // namespace DiskManager
} // namespace OHOS