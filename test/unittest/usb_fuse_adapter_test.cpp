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

#include "disk_manager_errno.h"
#include "mock_dlfcn.h"
#include "mock_parameters.h"
#include "usb_fuse_adapter.h"

namespace OHOS {
namespace DiskManager {

using namespace testing::ext;

class UsbFuseAdapterTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseMount_HandlerNullptr_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_EQ(adapter.NotifyUsbFuseMount(3, "vol-1", "uuid-1"), E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseMount_HandlerNullptr_002, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_EQ(adapter.NotifyUsbFuseMount(5, "vol-2", "uuid-2"), E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseUmount_HandlerNullptr_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_EQ(adapter.NotifyUsbFuseUmount("vol-1"), E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseUmount_HandlerNullptr_002, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_EQ(adapter.NotifyUsbFuseUmount("vol-2"), E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseByType_HandlerNullptr_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsUsbFuseByType("vfat"));
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseByType_HandlerNullptr_002, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsUsbFuseByType("ext4"));
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseByType_HandlerNullptr_003, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsUsbFuseByType("ntfs"));
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_HandlerNullptr_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_FALSE(adapter.IsUsbFuseEnabledForFsType("vfat"));
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_EmptyFsType_002, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_FALSE(adapter.IsUsbFuseEnabledForFsType(""));
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_HandlerNullptr_003, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_FALSE(adapter.IsUsbFuseEnabledForFsType("ntfs"));
}

class UsbFuseAdapterMockTest : public testing::Test {
public:
    void SetUp() override
    {
        auto &adapter = UsbFuseAdapter::GetInstance();
        adapter.handler_ = reinterpret_cast<void *>(0x1);
        MockDlfcnConfig::GetInstance().ClearDlsymResults();
        MockDlfcnConfig::GetInstance().SetMountResult(E_OK);
        MockDlfcnConfig::GetInstance().SetUMountResult(E_OK);
        MockDlfcnConfig::GetInstance().SetUsbFuseByTypeResult(E_OK, true);
        OHOS::system::g_mockGetBoolParameterResult = false;
    }

    void TearDown() override
    {
        auto &adapter = UsbFuseAdapter::GetInstance();
        adapter.handler_ = nullptr;
        MockDlfcnConfig::GetInstance().ClearDlsymResults();
        MockDlfcnConfig::GetInstance().SetDlopenResult(nullptr);
        OHOS::system::g_mockGetBoolParameterResult = false;
    }
};

HWTEST_F(UsbFuseAdapterMockTest, Init_DlopenSuccess_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    MockDlfcnConfig::GetInstance().SetDlopenResult(reinterpret_cast<void *>(0x2));
    adapter.Init();
    EXPECT_NE(adapter.handler_, nullptr);
    adapter.handler_ = reinterpret_cast<void *>(0x1);
}

HWTEST_F(UsbFuseAdapterMockTest, UnInit_HandlerNonNull_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_NE(adapter.handler_, nullptr);
    adapter.UnInit();
    EXPECT_EQ(adapter.handler_, nullptr);
    adapter.handler_ = reinterpret_cast<void *>(0x1);
}

HWTEST_F(UsbFuseAdapterMockTest, NotifyUsbFuseMount_DlsymNull_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_EQ(adapter.NotifyUsbFuseMount(3, "vol-1", "uuid-1"), E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterMockTest, NotifyUsbFuseUmount_DlsymNull_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_EQ(adapter.NotifyUsbFuseUmount("vol-1"), E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterMockTest, IsUsbFuseByType_DlsymNull_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsUsbFuseByType("vfat"));
}

HWTEST_F(UsbFuseAdapterMockTest, IsUsbFuseByType_FuncFailed_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    MockDlfcnConfig::GetInstance().SetDlsymResult("IsUsbFuseByType",
        reinterpret_cast<uintptr_t>(&MockIsUsbFuseByTypeFunc));
    MockDlfcnConfig::GetInstance().SetUsbFuseByTypeResult(E_DAEMON_IPC_FAILED, true);
    EXPECT_TRUE(adapter.IsUsbFuseByType("vfat"));
}

HWTEST_F(UsbFuseAdapterMockTest, IsUsbFuseByType_Success_Enabled_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    MockDlfcnConfig::GetInstance().SetDlsymResult("IsUsbFuseByType",
        reinterpret_cast<uintptr_t>(&MockIsUsbFuseByTypeFunc));
    MockDlfcnConfig::GetInstance().SetUsbFuseByTypeResult(E_OK, true);
    EXPECT_TRUE(adapter.IsUsbFuseByType("vfat"));
}

HWTEST_F(UsbFuseAdapterMockTest, IsUsbFuseEnabledForFsType_CcmEnabled_TypeTrue_001, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    MockDlfcnConfig::GetInstance().SetDlsymResult("IsUsbFuseByType",
        reinterpret_cast<uintptr_t>(&MockIsUsbFuseByTypeFunc));
    MockDlfcnConfig::GetInstance().SetUsbFuseByTypeResult(E_OK, true);
    OHOS::system::g_mockGetBoolParameterResult = true;
    EXPECT_TRUE(adapter.IsUsbFuseEnabledForFsType("vfat"));
}

HWTEST_F(UsbFuseAdapterMockTest, IsUsbFuseEnabledForFsType_CcmDisabled_003, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    OHOS::system::g_mockGetBoolParameterResult = false;
    EXPECT_FALSE(adapter.IsUsbFuseEnabledForFsType("vfat"));
}

HWTEST_F(UsbFuseAdapterMockTest, IsUsbFuseEnabledForFsType_CcmEnabled_EmptyFsType_004, TestSize.Level0)
{
    auto &adapter = UsbFuseAdapter::GetInstance();
    MockDlfcnConfig::GetInstance().SetDlsymResult("IsUsbFuseByType",
        reinterpret_cast<uintptr_t>(&MockIsUsbFuseByTypeFunc));
    MockDlfcnConfig::GetInstance().SetUsbFuseByTypeResult(E_OK, true);
    OHOS::system::g_mockGetBoolParameterResult = true;
    EXPECT_FALSE(adapter.IsUsbFuseEnabledForFsType(""));
}

} // namespace DiskManager
} // namespace OHOS