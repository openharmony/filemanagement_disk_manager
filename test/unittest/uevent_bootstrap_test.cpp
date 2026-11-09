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
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cstdarg>
#include <sys/sysmacros.h>

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
#include "disk.h"
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
constexpr uint64_t SECTOR_SIZE = 512;

static bool g_interceptSysfs = false;
static uint64_t g_mockDevSectorSize = 0;
static int g_mockSysfsFd = -1;
static constexpr int MOCK_SYSFS_FD = 10000;
static constexpr size_t SYSFS_BLOCK_PREFIX_LEN = sizeof("/sys/class/block/") - 1;

extern "C" int __real_open(const char *pathname, int flags, ...);
extern "C" int __real___open_chk(const char *pathname, int flags);
extern "C" ssize_t __real_read(int fd, void *buf, size_t count);
extern "C" int __real_close(int fd);

extern "C" int __wrap_open(const char *pathname, int flags, ...)
{
    if (g_interceptSysfs && pathname != nullptr &&
        strncmp(pathname, "/sys/class/block/", SYSFS_BLOCK_PREFIX_LEN) == 0 && strstr(pathname, "/size") != nullptr) {
        g_mockSysfsFd = MOCK_SYSFS_FD;
        return g_mockSysfsFd;
    }
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        int mode = va_arg(args, int);
        va_end(args);
        return __real_open(pathname, flags, mode);
    }
    return __real_open(pathname, flags);
}

extern "C" int __wrap___open_chk(const char *pathname, int flags)
{
    if (g_interceptSysfs && pathname != nullptr &&
        strncmp(pathname, "/sys/class/block/", SYSFS_BLOCK_PREFIX_LEN) == 0 && strstr(pathname, "/size") != nullptr) {
        g_mockSysfsFd = MOCK_SYSFS_FD;
        return g_mockSysfsFd;
    }
    return __real___open_chk(pathname, flags);
}

extern "C" ssize_t __wrap_read(int fd, void *buf, size_t count)
{
    if (g_interceptSysfs && fd == g_mockSysfsFd) {
        std::string val = std::to_string(g_mockDevSectorSize);
        size_t n = val.size() < count ? val.size() : count;
        memcpy(buf, val.c_str(), n);
        return static_cast<ssize_t>(n);
    }
    return __real_read(fd, buf, count);
}

extern "C" int __wrap_close(int fd)
{
    if (g_interceptSysfs && fd == g_mockSysfsFd) {
        g_mockSysfsFd = -1;
        return 0;
    }
    return __real_close(fd);
}

struct SysfsInterceptGuard {
    explicit SysfsInterceptGuard(uint64_t sectors)
    {
        g_interceptSysfs = true;
        g_mockDevSectorSize = sectors;
    }
    ~SysfsInterceptGuard()
    {
        g_interceptSysfs = false;
        g_mockDevSectorSize = 0;
    }
};

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
        UeventBootstrap::ResetPartitionSnapshotForTest();
        ON_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillByDefault(Return(E_OK));
        ON_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillByDefault(Return(E_OK));
        ON_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillByDefault(Return(false));
        ON_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillByDefault(Return(E_OK));
        ON_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).WillByDefault(Return(E_OK));
        ON_CALL(DiskManager::GetInstance(), SetVolumeDiscState(_, _)).WillByDefault(Return(E_OK));
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

static void FillUsbDisk(const std::string &diskId, Disk &out)
{
    out = Disk(diskId, 1024, "/dev/block/" + diskId, USB_FLAG);
    out.SetDiskType(USB_FLAG);
}

static void FillInternalDataDisk(const std::string &diskId, Disk &out)
{
    out = Disk(diskId, 1024, "/dev/block/" + diskId, DATA_DISK_HDD);
    out.SetDiskType(DATA_DISK_HDD);
}

static void ExpectIsPartitioningFalse(int times = 2)
{
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .Times(times)
        .WillRepeatedly(Return(false));
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

HWTEST_F(UeventBootstrapTest, HandleDiskChange_InternalDataDiskPartitioning_Skip_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("change", 8, 0);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), NotifyPartitionDone(_)).Times(0);
    int32_t ret = UeventBootstrap::HandleDiskChange(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskChange_RemovablePartitioning_NotSkip_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillUsbDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(true));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK gpt\nPART 1\n")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), NotifyPartitionDone(_))
        .Times(1);
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK 8 1 gpt\n")), SetArgReferee<2>(0), Return(E_OK)));
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
    ExpectIsPartitioningFalse();
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK gpt\nPART 1\n")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, HandleDiskChange_HasDiskExist_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(true));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK gpt\nPART 1\n")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(DiskManager::GetInstance(), NotifyPartitionDone(_))
        .Times(1);
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK 8 1 gpt\n")), SetArgReferee<2>(0), Return(E_OK)));
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
    ExpectIsPartitioningFalse();
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK 8 1 gpt\n")), SetArgReferee<2>(0), Return(E_OK)));
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
    ExpectIsPartitioningFalse();
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK 8 1 gpt\n")), SetArgReferee<2>(0), Return(E_OK)));
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
    ExpectIsPartitioningFalse();
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_QueryCDStatusFail_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
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
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
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
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
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
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .WillOnce(Return(-1));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverCD_EjectFail_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
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
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(-1)));
    // GetDiskSize returns 2MB (<=4MB), disk should be abandoned
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(2 * 1024 * 1024)), Return(E_OK)));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_NE(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitions_NonEmptyShortDump_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string shortDump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(shortDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
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
    ExpectIsPartitioningFalse();
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
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
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
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            FillUsbDisk(diskId, out);
            return E_OK;
        }))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillRepeatedly(Return(true));
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
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            FillUsbDisk(diskId, out);
            return E_OK;
        }))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillRepeatedly(Return(true));
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
    ExpectIsPartitioningFalse();
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
    ExpectIsPartitioningFalse();
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
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
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
    ExpectIsPartitioningFalse();
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
    // disk node only, no fallback volume node (legacy behavior kept)
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
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
    ExpectIsPartitioningFalse();
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // legacy behavior kept: no volume created, no mount attempted, only disk-level abnormal event
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(),
                PublishDiskAbnormalImpl(Eq("disk-8-1"), Eq(PARTITION_TABLE_READ_FAILED)))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
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
    ExpectIsPartitioningFalse();
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
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", MOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), ForceUnmount(_))
        .WillOnce(Return(-1));
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

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_DestroyVolumeNodeFail_TestCase_008, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", UNMOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), ForceUnmount(_)).Times(0);
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
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), HasDisk(_))
        .WillOnce(Return(false));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK 8 1 gpt\n")), SetArgReferee<2>(0), Return(E_OK)));
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK 8 1 gpt\n")), SetArgReferee<2>(0), Return(E_OK)));
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
    ExpectIsPartitioningFalse();
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(msg), DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, OnBlockDiskUevent_Change_TestCase_006, TestSize.Level0)
{
    std::string msg = "ACTION=change\nSUBSYSTEM=block\nDEVPATH=/devices/sda\nDEVTYPE=disk\nMAJOR=8\nMINOR=1\n";
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillOnce(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), NotifyPartitionDone(_)).Times(0);
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(msg), DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_CD_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
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
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
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
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_WholeDiskNoFs_TestCase_005, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK 8 1 gpt\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_WholeDiskPartitioning_TestCase_006, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _)).Times(0);
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _)).WillRepeatedly(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            FillUsbDisk(diskId, out);
            return E_OK;
        }))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).Times(0);
    // 分区中走建卷+Format 流程，绝不允许标记异常
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_InternalDataDiskPartitioning_TestCase_007, TestSize.Level0)
{
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-8-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), DestroyVolumeByDiskIdAndPartNum(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, DiscoverPartitionsAndVolumes_EmptyDumpNoDiskLine_TestCase_004, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_)).Times(0);
    // dump 无 DISK 行：弃盘静默，不创建兜底卷也不标记
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_NE(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, RediscoverDiskVolumes_TestCase_001, TestSize.Level0)
{
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("DISK gpt\nPART 1\n")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
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
    // Safely ejected volume: skip ForceUnmount, only publish REMOVED.
    EXPECT_CALL(DiskManager::GetInstance(), ForceUnmount(_)).Times(0);
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

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_FuseVolume_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", UNMOUNTED);
            VolumeExternal vol(vc);
            vol.SetPath("/mnt/data/external_fuse/test-uuid");
            out.push_back(vol);
            return E_OK;
        }));
    // Fuse same as non-Fuse: safely ejected -> VOLUME_REMOVED only.
    EXPECT_CALL(DiskManager::GetInstance(), ForceUnmount(_)).Times(0);
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

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_FuseBadRemoval_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", MOUNTED);
            VolumeExternal vol(vc);
            vol.SetPath("/mnt/data/external_fuse/test-uuid");
            out.push_back(vol);
            return E_OK;
        }));
    // Fuse same as non-Fuse: mounted pull -> ForceUnmount then VOLUME_BAD_REMOVAL.
    EXPECT_CALL(DiskManager::GetInstance(), ForceUnmount(_))
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

HWTEST_F(UeventBootstrapTest, HandleDiskRemove_BadRemoval_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("remove", 8, 1);
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-8-2", EXTERNAL, "disk-8-1", MOUNTED);
            out.push_back(VolumeExternal(vc));
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), ForceUnmount(_))
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

HWTEST_F(UeventBootstrapTest, PartitionDiff_FirstAddSinglePart_TestCase_001, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK 8 1 gpt\n8-1 0 1024 0700 userdata\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), DestroyVolumeByDiskIdAndPartNum(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, PartitionDiff_ChangeAddPartition_TestCase_002, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string firstDump = "DISK gpt\nPART 1\n";
    std::string secondDump = "DISK gpt\nPART 1\nPART 2\n";

    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(firstDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    ASSERT_EQ(UeventBootstrap::DiscoverPartitionsAndVolumes(env, false), DiskManagerErrNo::E_OK);
    testing::Mock::VerifyAndClearExpectations(&DiskManager::GetInstance());
    testing::Mock::VerifyAndClearExpectations(&MockStorageDaemonAdapter::GetInstance());

    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(secondDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(),
                OnVolumeCreated(Property(&VolumeExternal::GetId, Eq("vol-8-3"))))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), DestroyVolumeByDiskIdAndPartNum(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, PartitionDiff_ChangeRemovePartition_TestCase_003, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string firstDump = "DISK gpt\nPART 1\n";
    std::string secondDump = "DISK gpt\n";

    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(firstDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    ASSERT_EQ(UeventBootstrap::DiscoverPartitionsAndVolumes(env, false), DiskManagerErrNo::E_OK);
    testing::Mock::VerifyAndClearExpectations(&DiskManager::GetInstance());
    testing::Mock::VerifyAndClearExpectations(&MockStorageDaemonAdapter::GetInstance());

    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(secondDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), DestroyVolumeByDiskIdAndPartNum(Eq("disk-8-1"), Eq(1)))
        .WillOnce(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(),
                OnVolumeCreated(Property(&VolumeExternal::GetId, Eq("vol-8-1"))))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    ExpectIsPartitioningFalse();
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

HWTEST_F(UeventBootstrapTest, PartitionDiff_ChangeTypeCode_TestCase_004, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string gptDump = "DISK gpt\nPART 1\n";
    std::string mbrDump = "DISK mbr\nPART 1 0x0c\n";

    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(gptDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    ASSERT_EQ(UeventBootstrap::DiscoverPartitionsAndVolumes(env, false), DiskManagerErrNo::E_OK);
    testing::Mock::VerifyAndClearExpectations(&DiskManager::GetInstance());
    testing::Mock::VerifyAndClearExpectations(&MockStorageDaemonAdapter::GetInstance());

    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(mbrDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), DestroyVolumeByDiskIdAndPartNum(Eq("disk-8-1"), Eq(1)))
        .WillOnce(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(),
                OnVolumeCreated(Property(&VolumeExternal::GetId, Eq("vol-8-2"))))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: PartitionDiff_MbrFsTypeChange_TestCase_005
 * @tc.desc: MBR same partition formatted to another fs (type code 0x0c -> 0x83, e.g. exFAT->ext4)
 *           triggers re-enumeration via kernel change uevent: partition identity is keyed by
 *           partitionNumber + partitionType only (fsTypeRaw excluded), so no volume is
 *           destroyed/created and no auto-mount happens after Format.
 */
HWTEST_F(UeventBootstrapTest, PartitionDiff_MbrFsTypeChange_TestCase_005, TestSize.Level0)
{
    UeventBootstrap::ResetPartitionSnapshotForTest();
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string firstDump = "DISK mbr\nPART 1 0x0c\n";
    std::string secondDump = "DISK mbr\nPART 1 0x83\n";

    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(firstDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _)).WillOnce(Return(E_OK)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    ASSERT_EQ(UeventBootstrap::DiscoverPartitionsAndVolumes(env, false), DiskManagerErrNo::E_OK);
    testing::Mock::VerifyAndClearExpectations(&DiskManager::GetInstance());
    testing::Mock::VerifyAndClearExpectations(&MockStorageDaemonAdapter::GetInstance());

    // 格式化后重枚举：同分区号仅 MBR 类型字节变化，不允许判为分区增删
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(secondDump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), DestroyVolumeByDiskIdAndPartNum(_, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).Times(0);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _)).Times(0);
    ExpectIsPartitioningFalse(1);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeLarge_TestCase_001
 * @tc.desc: ReadPartitionTable fails, GetDiskSize returns size > 4MB: keep legacy behavior that only
 *           disk object is created (no volume, no mount, no ReadMetadata), and publish disk-level
 *           abnormal event with reason PARTITION_TABLE_READ_FAILED.
 */
HWTEST_F(UeventBootstrapTest, DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeLarge_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    // CreateBlockDeviceNode x1: disk node only, no fallback volume node (legacy behavior kept)
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(-1)));
    // GetDiskSize returns 10MB (10 * 1024 * 1024 = 10485760 bytes), > 4MB threshold
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(10 * 1024 * 1024)), Return(E_OK)));
    // E_STORAGE_VALID_NODE path skips ReplacePartitionsForDisk
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _))
        .Times(0);
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    // E_STORAGE_VALID_NODE abnormal-notify gating queries IsPartitioning once
    ExpectIsPartitioningFalse(1);
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // legacy behavior kept: no volume, no mount, no ReadMetadata on table read failure
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    // disk-level abnormal event published with reason PARTITION_TABLE_READ_FAILED
    EXPECT_CALL(CommonEventPublisher::GetInstance(),
                PublishDiskAbnormalImpl(Eq("disk-8-1"), Eq(PARTITION_TABLE_READ_FAILED)))
        .Times(1);
    // Should NOT call DestroyBlockDeviceNode since device is valid
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeLarge_NoPublish_TestCase_001
 * @tc.desc: ReadPartitionTable fails, GetDiskSize returns size > 4MB with publishNewDiskEvent=false:
 *           no disk event published (legacy behavior), but disk-level abnormal event is still published
 *           with reason PARTITION_TABLE_READ_FAILED.
 */
HWTEST_F(UeventBootstrapTest, DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeLarge_NoPublish_TestCase_001,
         TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(-1)));
    // GetDiskSize returns 10MB, > 4MB threshold
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(10 * 1024 * 1024)), Return(E_OK)));
    // publishNewDiskEvent=false, UpsertDiskAndPublishEvent returns early, no disk event published
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_)).Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _)).Times(0);
    // E_STORAGE_VALID_NODE abnormal-notify gating queries IsPartitioning once
    ExpectIsPartitioningFalse(1);
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // legacy behavior kept: no volume, no mount, no ReadMetadata on table read failure
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).Times(0);
    // abnormal notify is independent of disk event publishing
    EXPECT_CALL(CommonEventPublisher::GetInstance(),
                PublishDiskAbnormalImpl(Eq("disk-8-1"), Eq(PARTITION_TABLE_READ_FAILED)))
        .Times(1);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeSmall_TestCase_002
 * @tc.desc: ReadPartitionTable fails, GetDiskSize returns size <= 4MB, disk should be abandoned
 */
HWTEST_F(UeventBootstrapTest, DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeSmall_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(-1)));
    // GetDiskSize returns 2MB (2 * 1024 * 1024 = 2097152 bytes), <= 4MB threshold
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(2 * 1024 * 1024)), Return(E_OK)));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_)).Times(0);
    // disk abandoned: no fallback volume detection, no abnormal marking, no abnormal event
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_NE(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeFail_TestCase_003
 * @tc.desc: ReadPartitionTable fails, GetDiskSize also fails, disk should be abandoned
 */
HWTEST_F(UeventBootstrapTest, DiscoverPartitions_ReadPartitionTableFail_GetDiskSizeFail_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(-1)));
    // GetDiskSize fails
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(0)), Return(-1)));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), DestroyBlockDeviceNode(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_)).Times(0);
    // disk abandoned: no fallback volume detection, no abnormal marking, no abnormal event
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_NE(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DiscoverPartitions_ReadPartitionTableFail_DirectFsNotMarked_TestCase_004
 * @tc.desc: ReadPartitionTable fails and whole-disk has a direct filesystem: keep legacy behavior
 *           that no volume is created and no mount is attempted (no ReadMetadata on this path),
 *           only disk-level abnormal event with reason PARTITION_TABLE_READ_FAILED is published.
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, DiscoverPartitions_ReadPartitionTableFail_DirectFsNotMarked_TestCase_004, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(-1)));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(10 * 1024 * 1024)), Return(E_OK)));
    // E_STORAGE_VALID_NODE abnormal-notify gating queries IsPartitioning once
    ExpectIsPartitioningFalse(1);
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // legacy behavior kept: table read failure never triggers whole-disk discovery,
    // direct filesystem is not probed, no volume, no mount
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _)).Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(),
                PublishDiskAbnormalImpl(Eq("disk-8-1"), Eq(PARTITION_TABLE_READ_FAILED)))
        .Times(1);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DiscoverPartitions_ReadPartitionTableFail_PartitioningSilent_TestCase_005
 * @tc.desc: ReadPartitionTable fails while partitioning is in progress: user-initiated flow keeps silent,
 *           no volume created, no abnormal marking and no abnormal event
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, DiscoverPartitions_ReadPartitionTableFail_PartitioningSilent_TestCase_005,
         TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")), SetArgReferee<2>(0), Return(-1)));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(10 * 1024 * 1024)), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _)).Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _)).Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: TryEjectCdrom_NonEmptyDiscNoMountedVol_TestCase_009
 * @tc.desc: NON_EMPTY_DISC + 无 MOUNTED 卷 → FindMountedVolumePath 返回空 → 直接弹出
 *           覆盖 TryEjectCdrom 的 NON_EMPTY_DISC 分支、FindMountedVolumePath 循环不匹配返回空、
 *           EjectAndDestroyCdrom 成功路径。
 */
HWTEST_F(UeventBootstrapTest, TryEjectCdrom_NonEmptyDiscNoMountedVol_TestCase_009, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            out = Disk(diskId, 1024, "/dev/block/" + diskId, CD_FLAG);
            out.SetCdromState(CdromState::NON_EMPTY_DISC);
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Eject(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: TryEjectCdrom_QueryUsbIsInUseFails_TestCase_010
 * @tc.desc: NON_EMPTY_DISC + 有 MOUNTED 卷 + QueryUsbIsInUse 返回失败 → CanEjectMountedCdrom 返回 false → 不弹出
 *           覆盖 FindMountedVolumePath 找到路径返回、CanEjectMountedCdrom 查询失败返回 false。
 */
HWTEST_F(UeventBootstrapTest, TryEjectCdrom_QueryUsbIsInUseFails_TestCase_010, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            out = Disk(diskId, 1024, "/dev/block/" + diskId, CD_FLAG);
            out.SetCdromState(CdromState::NON_EMPTY_DISC);
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-11-1", EXTERNAL, "disk-11-0", MOUNTED, "udf");
            VolumeExternal vol(vc);
            vol.SetPath("/mnt/data/external");
            out.push_back(vol);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(Return(E_DAEMON_IPC_FAILED));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateDisk(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Eject(_))
        .Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: TryEjectCdrom_UsbInUse_TestCase_011
 * @tc.desc: NON_EMPTY_DISC + 有 MOUNTED 卷 + QueryUsbIsInUse 成功但 isInUse=true → 不弹出
 *           覆盖 CanEjectMountedCdrom isInUse=true 返回 false。
 */
HWTEST_F(UeventBootstrapTest, TryEjectCdrom_UsbInUse_TestCase_011, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            out = Disk(diskId, 1024, "/dev/block/" + diskId, CD_FLAG);
            out.SetCdromState(CdromState::NON_EMPTY_DISC);
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-11-1", EXTERNAL, "disk-11-0", MOUNTED, "udf");
            VolumeExternal vol(vc);
            vol.SetPath("/mnt/data/external");
            out.push_back(vol);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(true), Return(E_OK)));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryCDStatus(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(1), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateDisk(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Eject(_))
        .Times(0);
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: TryEjectCdrom_UsbNotInUse_TestCase_012
 * @tc.desc: NON_EMPTY_DISC + 有 MOUNTED 卷 + QueryUsbIsInUse 成功且 isInUse=false → 弹出
 *           覆盖 CanEjectMountedCdrom 返回 true、EjectAndDestroyCdrom 成功。
 */
HWTEST_F(UeventBootstrapTest, TryEjectCdrom_UsbNotInUse_TestCase_012, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            out = Disk(diskId, 1024, "/dev/block/" + diskId, CD_FLAG);
            out.SetCdromState(CdromState::NON_EMPTY_DISC);
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            VolumeCore vc("vol-11-1", EXTERNAL, "disk-11-0", MOUNTED, "udf");
            VolumeExternal vol(vc);
            vol.SetPath("/mnt/data/external");
            out.push_back(vol);
            return E_OK;
        }))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(false), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), Eject(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: TryEjectCdrom_MultiVolumeMatch_TestCase_013
 * @tc.desc: NON_EMPTY_DISC + 多种不匹配卷(diskId不匹配/id=="0"/状态非MOUNTED/path为空) + 最终匹配 → 弹出
 *           覆盖 FindMountedVolumePath 所有 continue 分支 + 成功返回路径。
 */
HWTEST_F(UeventBootstrapTest, TryEjectCdrom_MultiVolumeMatch_TestCase_013, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 11, 0, "/devices/sr0", "disk", "block", "sr0", true);
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Return(false));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ReadExtDiskInfoFromDaemon(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), OnDiskCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskChangeImpl(_, _))
        .Times(1);
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            out = Disk(diskId, 1024, "/dev/block/" + diskId, CD_FLAG);
            out.SetCdromState(CdromState::NON_EMPTY_DISC);
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), GetAllVolumes(_))
        .WillOnce(Invoke([](std::vector<VolumeExternal> &out) {
            // 1. diskId 不匹配 → continue (vol.GetDiskId() != diskId)
            VolumeCore vc1("vol-99-1", EXTERNAL, "disk-99-0", MOUNTED, "udf");
            VolumeExternal v1(vc1);
            v1.SetPath("/mnt/data/other");
            out.push_back(v1);
            // 2. id == "0" → continue (vol.GetId() == "0")
            VolumeCore vc2("0", EXTERNAL, "disk-11-0", MOUNTED, "udf");
            VolumeExternal v2(vc2);
            v2.SetPath("/mnt/data/zero");
            out.push_back(v2);
            // 3. 状态非 MOUNTED → continue (vol.GetState() != MOUNTED)
            VolumeCore vc3("vol-11-2", EXTERNAL, "disk-11-0", UNMOUNTED, "udf");
            VolumeExternal v3(vc3);
            v3.SetPath("/mnt/data/unmounted");
            out.push_back(v3);
            // 4. MOUNTED 但 path 为空 → continue (vol.GetPath().empty())
            VolumeCore vc4("vol-11-3", EXTERNAL, "disk-11-0", MOUNTED, "udf");
            VolumeExternal v4(vc4);
            out.push_back(v4);
            // 5. MOUNTED 且 path 非空、diskId 匹配、id!="0" → return path
            VolumeCore vc5("vol-11-1", EXTERNAL, "disk-11-0", MOUNTED, "udf");
            VolumeExternal v5(vc5);
            v5.SetPath("/mnt/data/external");
            out.push_back(v5);
            return E_OK;
        }))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), QueryUsbIsInUse(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(false), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), Eject(_))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, true);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

// ===== ResolvePartitionDev / CreateDmLinearForPartition 测试 =====
// 从 DiscoverPartitionsAndVolumes 入口，通过 mock 控制进入 DiscoverSinglePartitionVolume，
// 通过 mock GetDiskSize IPC 控制 CreateDmLinearForPartition 内的分区大小。

/**
 * @tc.name: DmLinear_InternalDataDisk_Success_001
 * @tc.desc: 内置数据盘，分区字节数 > 20MB(40960 扇区)，CreateDmLinear 成功
 */
HWTEST_F(UeventBootstrapTest, DmLinear_InternalDataDisk_Success_001, TestSize.Level0)
{
    // 204800 扇区 * SECTOR_SIZE = 104857600 字节 > 20MB，走 CreateDmLinear 成功路径
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(204800 * SECTOR_SIZE)), Return(E_OK)));
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<3>(static_cast<uint64_t>(makedev(253, 0))), Return(E_OK)));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-8-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DmLinear_InternalDataDisk_SectorTooSmall_002
 * @tc.desc: 内置数据盘，分区字节数 <= 20MB(40960 扇区)，CreateDmLinearForPartition 跳过
 */
HWTEST_F(UeventBootstrapTest, DmLinear_InternalDataDisk_SectorTooSmall_002, TestSize.Level0)
{
    // 20480 扇区 * SECTOR_SIZE = 10485760 字节 <= 20MB，跳过 dm-linear
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(20480 * SECTOR_SIZE)), Return(E_OK)));
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(_, _, _, _)).Times(0);
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-8-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DmLinear_InternalDataDisk_SysfsZero_003
 * @tc.desc: 内置数据盘，GetDiskSize 返回 0，CreateDmLinearForPartition 跳过
 */
HWTEST_F(UeventBootstrapTest, DmLinear_InternalDataDisk_SysfsZero_003, TestSize.Level0)
{
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(0)), Return(E_OK)));
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(_, _, _, _)).Times(0);
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-8-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DmLinear_InternalDataDisk_CreateFail_004
 * @tc.desc: 内置数据盘，字节数足够但 CreateDmLinear 失败(err≠0)，走 fallback
 */
HWTEST_F(UeventBootstrapTest, DmLinear_InternalDataDisk_CreateFail_004, TestSize.Level0)
{
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(204800 * SECTOR_SIZE)), Return(E_OK)));
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<3>(static_cast<uint64_t>(0)), Return(-1)));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-8-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DmLinear_InternalDataDisk_DmDevZero_005
 * @tc.desc: 内置数据盘，CreateDmLinear 返回 E_OK 但 dmDev=0，走 fallback
 */
HWTEST_F(UeventBootstrapTest, DmLinear_InternalDataDisk_DmDevZero_005, TestSize.Level0)
{
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(204800 * SECTOR_SIZE)), Return(E_OK)));
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<3>(static_cast<uint64_t>(0)), Return(E_OK)));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-8-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DmLinear_InternalDataDisk_FallbackIsUserData_006
 * @tc.desc: 内置数据盘，DmLinear 跳过 + isUserData=true(>32分区含"userdata")，
 *           覆盖 ResolvePartitionDev isUserData=true 分支
 */
HWTEST_F(UeventBootstrapTest, DmLinear_InternalDataDisk_FallbackIsUserData_006, TestSize.Level0)
{
    // 33 分区每个都会走一次 ResolvePartitionDev，内置盘每次先调 GetDiskSize
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillRepeatedly(DoAll(SetArgReferee<1>(static_cast<uint64_t>(20480 * SECTOR_SIZE)), Return(E_OK)));
    std::string dump = "DISK gpt\n";
    for (int i = 1; i <= 33; ++i) {
        if (i == 1) {
            dump += "PART " + std::to_string(i) + " userdata\n";
        } else {
            dump += "PART " + std::to_string(i) + "\n";
        }
    }
    UeventEnv env = MakeUenv("change", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(_, _, _, _)).Times(0);
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-8-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DmLinear_Nvme_PartitionName_007
 * @tc.desc: NVMe 盘 nvme1n1 分区 1，验证 BuildPartitionDevName 拼出 nvme1n1p1（加 'p'），
 *           GetDiskSize 入参为层级路径 "nvme1n1/nvme1n1p1"，CreateDmLinear 成功
 */
HWTEST_F(UeventBootstrapTest, DmLinear_Nvme_PartitionName_007, TestSize.Level0)
{
    // 校验 GetDiskSize 入参为层级路径 "nvme1n1/nvme1n1p1"
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(StrEq("nvme1n1/nvme1n1p1"), _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(204800 * SECTOR_SIZE)), Return(E_OK)));
    UeventEnv env = MakeUenv("change", 259, 1, "/devices/nvme1n1", "disk", "block", "nvme1n1");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    // CreateDmLinear 入参 sourceDevPath 应为 /dev/block/nvme1n1p1
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(StrEq("/dev/block/nvme1n1p1"), _, _, _))
        .WillOnce(DoAll(SetArgReferee<3>(static_cast<uint64_t>(makedev(253, 0))), Return(E_OK)));
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-259-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: DmLinear_Nvme_GetDiskSizeFail_008
 * @tc.desc: NVMe 盘 nvme1n1 分区 1，GetDiskSize 失败，CreateDmLinearForPartition 跳过走 fallback
 */
HWTEST_F(UeventBootstrapTest, DmLinear_Nvme_GetDiskSizeFail_008, TestSize.Level0)
{
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), GetDiskSize(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(static_cast<uint64_t>(0)), Return(-1)));
    UeventEnv env = MakeUenv("change", 259, 1, "/devices/nvme1n1", "disk", "block", "nvme1n1");
    std::string dump = "DISK gpt\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), ReplacePartitionsForDisk(_, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), GetVolumeById(_, _)).WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillRepeatedly(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateDmLinear(_, _, _, _)).Times(0);
    EXPECT_CALL(BlockInfoTable::GetInstance(), TryCopyByDiskId(_, _))
        .WillOnce(Invoke([](const std::string &, BlockInfo &info) {
            info.diskId = "disk-259-1";
            info.vendor = "vendor";
            return true;
        }));
    EXPECT_CALL(BlockInfoTable::GetInstance(), ToJsonStringWithExtrasImpl(_, _))
        .WillOnce(Return(std::string("{}")));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_)).WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("test-uuid")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _)).WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _)).WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::DiscoverPartitionsAndVolumes(env, false);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_AbnormalMetadataErr_TestCase_001
 * @tc.desc: 单分区卷元数据读取 IPC 失败，标记 METADATA_READ_FAILED且不尝试挂载。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_AbnormalMetadataErr_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK gpt\nPART 1\n";
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_DAEMON_IPC_FAILED)));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, METADATA_READ_FAILED))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_AbnormalEmptyMetadata_TestCase_002
 * @tc.desc: 单分区卷元数据读取成功但 uuid/fsType 为空，标记 METADATA_READ_FAILED（空值分支）。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_AbnormalEmptyMetadata_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK gpt\nPART 1\n";
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    // 元数据读取成功但 uuid 为空：ReadAndUpdateMetadata 内 IsUuidValid 校验不通过提前返回，
    // 不落盘元数据（保持既有行为），空值由异常检测路径标记 METADATA_READ_FAILED
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, METADATA_READ_FAILED))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_AbnormalUnsupportedFs_TestCase_003
 * @tc.desc: 单分区卷 fsType 不在支持范围（zfs），标记 FSTYPE_UNSUPPORTED且不尝试挂载。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_AbnormalUnsupportedFs_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK gpt\nPART 1\n";
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("uuid-zfs-1")),
                        SetArgReferee<2>(std::string("zfs")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, FSTYPE_UNSUPPORTED))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_AbnormalMountFailed_TestCase_004
 * @tc.desc: 单分区卷元数据正常但自动挂载失败，标记 MOUNT_FAILED，详情携带底层错误码。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_AbnormalMountFailed_TestCase_004, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK gpt\nPART 1\n";
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("uuid-ok-1")),
                        SetArgReferee<2>(std::string("vfat")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .WillOnce(Return(E_OTHER_MOUNT));
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, MOUNT_FAILED))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_NormalNoAbnormal_TestCase_005
 * @tc.desc: 正常识别流程（元数据正常+挂载成功）零异常标记。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_NormalNoAbnormal_TestCase_005, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK gpt\nPART 1\n";
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("uuid-ok-1")),
                        SetArgReferee<2>(std::string("vfat")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_AbnormalUserdataSilent_TestCase_006
 * @tc.desc: 内置 userdata 分区不参与异常检测（元数据为空不标记原因），空值门禁跳过自动挂载。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_AbnormalUserdataSilent_TestCase_006, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    // 分区表超过 MIN_LINES(32) 行且含 userdata 行，触发内置盘 userdata 识别并压缩为 DISK+单分区行
    std::string partitionDump = "DISK gpt\nPART 1\nPART 2\nPART 3\nPART 4 userdata\n";
    for (int i = 5; i <= 33; i++) {
        partitionDump += "PART " + std::to_string(i) + "\n";
    }
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_AbnormalUndetectableFsSilent_TestCase_007
 * @tc.desc: UDF 等不可检测文件系统卷不参与异常检测（保持既有静默行为），挂载失败也不标记。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_AbnormalUndetectableFsSilent_TestCase_007, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK gpt\nPART 1\n";
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("uuid-ok-1")),
                        SetArgReferee<2>(std::string("udf")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .WillOnce(Return(E_OTHER_MOUNT));
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: HandleDiskAdd_PartitioningFormatFailSilent_TestCase_008
 * @tc.desc: 分区中用户主动 Format 失败仅记录日志，不参与异常检测（用户流程不标记原因）。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, HandleDiskAdd_PartitioningFormatFailSilent_TestCase_008, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string partitionDump = "DISK gpt\nPART 1\n";
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
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("uuid-ok-1")),
                        SetArgReferee<2>(std::string("vfat")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), IsPartitioning(_))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(DiskManager::GetInstance(), Format(_, _))
        .WillOnce(Return(-1));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: UeventInvalid_Silent_TestCase_001
 * @tc.desc: uevent 非法（原因 1）静默——解析失败零异常标记。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, UeventInvalid_Silent_TestCase_001, TestSize.Level0)
{
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_EQ(UeventBootstrap::OnBlockDiskUevent(""), DiskManagerErrNo::E_UEVENT_PARSE_FAILED);
}

/**
 * @tc.name: PartitionTableParseFailure_Silent_TestCase_001
 * @tc.desc: 分区表内容非空但解析不出分区（原因 3）：放弃该盘（返回错误），零异常标记。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, PartitionTableParseFailure_Silent_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("garbage-dump")), SetArgReferee<2>(0), Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::DISK_MGR_ERR);
}

/**
 * @tc.name: WholeDiskNoFsAbnormal_TestCase_001
 * @tc.desc: 表类型非 GPT/MBR（如 APM，含合法 PART 行但表类型不可识别）且无直挂文件系统（盘不可用）：
 *           保持既有不建卷不挂载行为，仅广播分区表不可用（无卷载体）。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, WholeDiskNoFsAbnormal_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK apm\nPART 1\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
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
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(-1)));
    ExpectIsPartitioningFalse();
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // legacy behavior kept: no volume created, no mount attempted, only disk-level abnormal event
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(),
                PublishDiskAbnormalImpl(Eq("disk-8-1"), Eq(PARTITION_TABLE_READ_FAILED)))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: WholeDiskNoFsAbnormal_TestCase_002
 * @tc.desc: GPT 表但 PART 行字符排列无法解析（分区号非法/单 token）且无直挂文件系统：
 *           与 GPT 空分区一致按表分流保持既有静默（未分区状态非异常），不建卷不广播。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, WholeDiskNoFsAbnormal_TestCase_002, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\nPART abc\nPART\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
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
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(-1)));
    ExpectIsPartitioningFalse();
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // GPT 表按表分流：保持既有静默，不建卷不挂载不广播
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: WholeDiskNoFsInternalDiskSilent_TestCase_003
 * @tc.desc: 盘分类为内置数据盘且无可用分区、无直挂文件系统：不走异常兜底，保持静默零标记。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, WholeDiskNoFsInternalDiskSilent_TestCase_003, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 179, 0, "/devices/mmcblk0", "disk", "block", "mmcblk0");
    std::string dump = "DISK apm\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
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
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(-1)));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Invoke([](const std::string &diskId, Disk &out) {
            FillInternalDataDisk(diskId, out);
            return E_OK;
        }));
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: WholeDiskNoFsLegacyUserdataSilent_TestCase_004
 * @tc.desc: 旧格式 dump 含 userdata 分区特征（触发 userdata 过滤，视为内部用户数据盘）且无直挂文件系统：
 *           不走异常兜底，保持静默零标记。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, WholeDiskNoFsLegacyUserdataSilent_TestCase_004, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK 8 1 gpt\n";
    for (int i = 0; i < 33; ++i) {
        dump += "8-" + std::to_string(i) + " 0 1024 0700 " +
                (i == 1 ? "userdata" : "part" + std::to_string(i)) + "\n";
    }
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
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
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(Return(-1));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: WholeDiskMbrNoPartDirectFsMount_TestCase_001
 * @tc.desc: MBR 表无分区条目且整盘直挂文件系统可读（exfat，实测盘形态）：走既有 whole-disk
 *           建卷并挂载，零异常标记（既有整盘挂载规格不变，防止回归）。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, WholeDiskMbrNoPartDirectFsMount_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK mbr\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("mbr-direct-uuid")),
                        SetArgReferee<2>(std::string("exfat")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .WillOnce(Return(E_OK));
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: WholeDiskMbrNoFsAbnormalPublish_TestCase_001
 * @tc.desc: MBR 表无分区条目且整盘无直挂文件系统（盘不可用）：保持既有不建卷不挂载行为，
 *           仅广播分区表不可用（无卷载体）。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, WholeDiskMbrNoFsAbnormalPublish_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK mbr\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
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
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(-1)));
    ExpectIsPartitioningFalse();
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // legacy behavior kept: no volume created, no mount attempted, only disk-level abnormal event
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(),
                PublishDiskAbnormalImpl(Eq("disk-8-1"), Eq(PARTITION_TABLE_READ_FAILED)))
        .Times(1);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: WholeDiskGptNoFsSilent_TestCase_001
 * @tc.desc: GPT 表完好但无分区条目（未分区状态，非异常）且整盘无直挂文件系统：
 *           保持既有静默，不建卷不挂载不广播。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, WholeDiskGptNoFsSilent_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string dump = "DISK gpt\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(dump), SetArgReferee<2>(0), Return(E_OK)));
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
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadMetadata(_, _, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(std::string("")),
                        SetArgReferee<2>(std::string("")),
                        SetArgReferee<3>(std::string("")),
                        Return(-1)));
    ExpectIsPartitioningFalse();
    // IsInternalDataDiskById queries disk once (external disk: not internal)
    EXPECT_CALL(DiskManager::GetInstance(), GetDiskById(_, _))
        .WillOnce(Return(E_OK));
    // GPT 表按表分流：保持既有静默，不建卷不挂载不广播
    EXPECT_CALL(DiskManager::GetInstance(), OnVolumeCreated(_))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, _))
        .Times(0);
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    EXPECT_CALL(CommonEventPublisher::GetInstance(), PublishDiskAbnormalImpl(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}

/**
 * @tc.name: BlockInfoFailure_DoesNotAffectMarking_TestCase_001
 * @tc.desc: 整盘卷 BlockInfo 失败（-1）不阻塞不吞掉异常标记，fsType 不支持照常标记 FSTYPE_UNSUPPORTED。
 * @tc.type: FUNC
 * @tc.require: 20260807939139
 */
HWTEST_F(UeventBootstrapTest, BlockInfoFailure_DoesNotAffectMarking_TestCase_001, TestSize.Level0)
{
    UeventEnv env = MakeUenv("add", 8, 1, "/devices/sda", "disk", "block", "sda");
    std::string wholeDiskDump = "DISK 8 1 gpt\n";
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), CreateBlockDeviceNode(_, _, _, _))
        .WillOnce(Return(E_OK))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(MockStorageDaemonAdapter::GetInstance(), ReadPartitionTable(_, _, _))
        .WillOnce(DoAll(SetArgReferee<1>(wholeDiskDump), SetArgReferee<2>(0), Return(E_OK)));
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
        .WillOnce(DoAll(SetArgReferee<1>(std::string("uuid-zfs-4")),
                        SetArgReferee<2>(std::string("zfs")),
                        SetArgReferee<3>(std::string("")),
                        Return(E_OK)));
    EXPECT_CALL(DiskManager::GetInstance(), UpdateVolumeMetadata(_, _, _, _))
        .WillOnce(Return(E_OK));
    ExpectIsPartitioningFalse();
    EXPECT_CALL(DiskManager::GetInstance(), MarkVolumeAbnormal(_, FSTYPE_UNSUPPORTED))
        .WillOnce(Return(E_OK));
    EXPECT_CALL(DiskManager::GetInstance(), Mount(_, _))
        .Times(0);
    int32_t ret = UeventBootstrap::HandleDiskAdd(env);
    EXPECT_EQ(ret, DiskManagerErrNo::E_OK);
}
