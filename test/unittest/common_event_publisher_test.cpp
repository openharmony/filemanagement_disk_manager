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
 * @file common_event_publisher_test.cpp
 * @brief 单元测试：CommonEventPublisher（卷/盘公共事件发布）。
 *
 * 覆盖目标对齐 common_event_publisher.cpp：STATE_INFOS 循环与提前返回、MOUNTED 附加参数、
 * PublishDiskChange 的 MOUNTED/REMOVED 分支。单用例行数约束：大于 5 行且小于 50 行（仅计函数体）。
 *
 * 分支覆盖以 EXPECT_NO_THROW 为主；本 UT 目标在 BUILD.gn 中已开启 use_exceptions。
 */

#include <gtest/gtest.h>

#include "disk.h"
#include "notification/common_event_publisher.h"
#include "volume_external.h"

namespace OHOS {
namespace DiskManager {

using namespace testing::ext;

namespace {

constexpr int32_t kUtVolumeFlags = 0x0b;
constexpr int64_t kUtDiskSizeBytes = 4096;
constexpr int64_t kUtRemovedDiskSizeBytes = 2048;

VolumeExternal MakeSampleVolume()
{
    VolumeCore core("vol-ut-1", EXTERNAL, "disk-ut-1");
    VolumeExternal v(core);
    v.SetFlags(kUtVolumeFlags);
    v.SetFsUuid("fs-uuid-ut");
    v.SetPath("/mnt/ut_volume");
    v.SetFsType(static_cast<int32_t>(VFAT));
    return v;
}

#ifdef CDC_STORAGE
VolumeExternal MakeDvrVolume()
{
    VolumeCore core("vol-dvr-ut", EXTERNAL, "disk-dvr-ut");
    VolumeExternal v(core);
    v.SetFlags(static_cast<int32_t>(DVR));
    v.SetFsUuid("fs-uuid-dvr");
    v.SetPath("/mnt/data/dvr/fs-uuid-dvr");
    v.SetFsType(static_cast<int32_t>(VFAT));
    return v;
}
#endif

Disk MakeSampleDisk()
{
    return Disk("disk-ut-1", kUtDiskSizeBytes, "/dev/block/disk-ut-1", USB_FLAG);
}

} // namespace

class CommonEventPublisherTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GTEST_LOG_(INFO) << "CommonEventPublisherTest SetUpTestCase";
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "CommonEventPublisherTest TearDownTestCase";
    }

    void SetUp() override
    {
        GTEST_LOG_(INFO) << "CommonEventPublisherTest SetUp";
    }

    void TearDown() override
    {
        GTEST_LOG_(INFO) << "CommonEventPublisherTest TearDown";
    }
};

/**
 * @tc.name: PublishVolumeChange_TestCase_001
 * @tc.desc: 静态 API 冒烟（已映射 UNMOUNTED）及 CHECKING、FUSE_REMOVED 无 STATE_INFOS 映射时的提前返回分支。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CommonEventPublisherTest, PublishVolumeChange_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PublishVolumeChange_TestCase_001 Start";

    VolumeExternal vMapped = MakeSampleVolume();
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(UNMOUNTED, vMapped));

    VolumeExternal vChecking = MakeSampleVolume();
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(CHECKING, vChecking));

    VolumeExternal vFuseRemoved = MakeSampleVolume();
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(FUSE_REMOVED, vFuseRemoved));

    GTEST_LOG_(INFO) << "PublishVolumeChange_TestCase_001 End";
}

/**
 * @tc.name: PublishVolumeChange_TestCase_002
 * @tc.desc: STATE_INFOS 中除 MOUNTED 外的状态各调用一次，覆盖循环内匹配与 break，以及非 MOUNTED 路径。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CommonEventPublisherTest, PublishVolumeChange_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PublishVolumeChange_TestCase_002 Start";

    const VolumeState nonMountedMapped[] = {
        REMOVED,
        UNMOUNTED,
        BAD_REMOVAL,
        EJECTING,
        DAMAGED,
        DAMAGED_MOUNTED,
        ENCRYPTING,
        ENCRYPTED_AND_LOCKED,
        ENCRYPTED_AND_UNLOCKED,
        DECRYPTING,
    };
    for (VolumeState s : nonMountedMapped) {
        VolumeExternal vol = MakeSampleVolume();
        EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(s, vol));
    }

    GTEST_LOG_(INFO) << "PublishVolumeChange_TestCase_002 End";
}

/**
 * @tc.name: PublishVolumeChange_TestCase_003
 * @tc.desc: UNMOUNTED 与 MOUNTED 组合：覆盖已映射非 MOUNTED 与 MOUNTED 下 path/fsType 附加及发布路径。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CommonEventPublisherTest, PublishVolumeChange_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PublishVolumeChange_TestCase_003 Start";

    VolumeExternal vUnmounted = MakeSampleVolume();
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(UNMOUNTED, vUnmounted));

    VolumeExternal vMountedShort = MakeSampleVolume();
    vMountedShort.SetPath("/data/ut_mounted");
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(MOUNTED, vMountedShort));

    VolumeExternal vMountedExfat = MakeSampleVolume();
    vMountedExfat.SetPath("/mnt/mounted_branch");
    vMountedExfat.SetFsType(static_cast<int32_t>(EXFAT));
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(MOUNTED, vMountedExfat));

    GTEST_LOG_(INFO) << "PublishVolumeChange_TestCase_003 End";
}

/**
 * @tc.name: PublishDiskChange_TestCase_001
 * @tc.desc: kind==MOUNTED 的 if 分支与 kind==REMOVED 的 else 分支各执行一次。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CommonEventPublisherTest, PublishDiskChange_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PublishDiskChange_TestCase_001 Start";

    Disk diskMounted = MakeSampleDisk();
    EXPECT_NO_THROW(CommonEventPublisher::PublishDiskChange(DiskEventKind::MOUNTED, diskMounted));

    Disk diskRemoved("disk-removed-ut", kUtRemovedDiskSizeBytes, "/dev/block/disk-removed-ut", CD_FLAG);
    EXPECT_NO_THROW(CommonEventPublisher::PublishDiskChange(DiskEventKind::REMOVED, diskRemoved));

    GTEST_LOG_(INFO) << "PublishDiskChange_TestCase_001 End";
}

#ifdef CDC_STORAGE
/**
 * @tc.name: PublishVolumeChange_DvrOnlyU0_TestCase_001
 * @tc.desc: CDC_STORAGE 下，volume.GetFlags()==DVR 时 PublishVolumeChange 必须走
 *           PublishCommonEventAsUser(DVR_ALLOWED_USER_ID=0) 提前返回分支；用例覆盖
 *           MOUNTED / UNMOUNTED / BAD_REMOVAL / EJECTING 等多种状态，均不应抛异常。
 *           本用例仅在 car 平台（car_device_enable=true）构建。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CommonEventPublisherTest, PublishVolumeChange_DvrOnlyU0_TestCase_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PublishVolumeChange_DvrOnlyU0_TestCase_001 Start";

    const VolumeState dvrStates[] = {MOUNTED, UNMOUNTED, BAD_REMOVAL, EJECTING, REMOVED};
    for (VolumeState s : dvrStates) {
        VolumeExternal vol = MakeDvrVolume();
        ASSERT_EQ(vol.GetFlags(), static_cast<int32_t>(DVR));
        EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(s, vol));
    }

    GTEST_LOG_(INFO) << "PublishVolumeChange_DvrOnlyU0_TestCase_001 End";
}

/**
 * @tc.name: PublishVolumeChange_DvrOnlyU0_TestCase_002
 * @tc.desc: 同一发布器对 DVR 卷与普通卷应能无副作用地连续调用——验证 DVR 分支 return 后
 *           普通卷的全用户广播路径仍可正常进入，不会因 DVR 卷状态而被卡住。
 *           本用例仅在 car 平台（car_device_enable=true）构建。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CommonEventPublisherTest, PublishVolumeChange_DvrOnlyU0_TestCase_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PublishVolumeChange_DvrOnlyU0_TestCase_002 Start";

    VolumeExternal vDvr = MakeDvrVolume();
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(MOUNTED, vDvr));

    VolumeExternal vNormal = MakeSampleVolume();
    ASSERT_NE(vNormal.GetFlags(), static_cast<int32_t>(DVR));
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(MOUNTED, vNormal));

    VolumeExternal vDvr2 = MakeDvrVolume();
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(UNMOUNTED, vDvr2));

    GTEST_LOG_(INFO) << "PublishVolumeChange_DvrOnlyU0_TestCase_002 End";
}

/**
 * @tc.name: PublishVolumeChange_DvrOnlyU0_TestCase_003
 * @tc.desc: 严格相等判定：仅 volume.GetFlags() == DVR(=6) 才走 PublishCommonEventAsUser(0)
 *           U0 分支；其它常见 flag（SD_FLAG=1、USB_FLAG=2、CD_FLAG=3、0）均走全用户广播；
 *           各情形均不应抛异常。本用例仅在 car 平台（car_device_enable=true）构建。
 * @tc.type: FUNC
 * @tc.require: NA
 */
HWTEST_F(CommonEventPublisherTest, PublishVolumeChange_DvrOnlyU0_TestCase_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PublishVolumeChange_DvrOnlyU0_TestCase_003 Start";

    const int32_t nonDvrFlags[] = {
        static_cast<int32_t>(SD_FLAG),
        static_cast<int32_t>(USB_FLAG),
        static_cast<int32_t>(CD_FLAG),
        0,
    };
    for (int32_t f : nonDvrFlags) {
        VolumeExternal v = MakeDvrVolume();
        v.SetFlags(f);
        EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(MOUNTED, v));
    }

    VolumeExternal exact = MakeDvrVolume();
    exact.SetFlags(static_cast<int32_t>(DVR));
    EXPECT_NO_THROW(CommonEventPublisher::PublishVolumeChange(MOUNTED, exact));

    GTEST_LOG_(INFO) << "PublishVolumeChange_DvrOnlyU0_TestCase_003 End";
}
#endif // CDC_STORAGE

} // namespace DiskManager
} // namespace OHOS
