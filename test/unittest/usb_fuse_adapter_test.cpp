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

#include "mock_usb_fuse_adapter.h"
#include "disk_manager_errno.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class UsbFuseAdapterTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseMount_TestCase_001, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, NotifyUsbFuseMount(_, _, _)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    int32_t result = UsbFuseAdapter::GetInstance().NotifyUsbFuseMount(3, "vol-1", "uuid-1");
    EXPECT_EQ(result, E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseMount_TestCase_002, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, NotifyUsbFuseMount(_, _, _)).WillOnce(Return(E_OK));
    int32_t result = UsbFuseAdapter::GetInstance().NotifyUsbFuseMount(5, "vol-2", "uuid-2");
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseUmount_TestCase_001, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, NotifyUsbFuseUmount(_)).WillOnce(Return(E_DAEMON_IPC_FAILED));
    int32_t result = UsbFuseAdapter::GetInstance().NotifyUsbFuseUmount("vol-1");
    EXPECT_EQ(result, E_DAEMON_IPC_FAILED);
}

HWTEST_F(UsbFuseAdapterTest, NotifyUsbFuseUmount_TestCase_002, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, NotifyUsbFuseUmount(_)).WillOnce(Return(E_OK));
    int32_t result = UsbFuseAdapter::GetInstance().NotifyUsbFuseUmount("vol-2");
    EXPECT_EQ(result, E_OK);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseByType_TestCase_001, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseByType(_)).WillOnce(Return(true));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseByType("vfat");
    EXPECT_TRUE(result);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseByType_TestCase_002, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseByType(_)).WillOnce(Return(false));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseByType("ext4");
    EXPECT_FALSE(result);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseByType_TestCase_003, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseByType("ntfs")).WillOnce(Return(true));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseByType("ntfs");
    EXPECT_TRUE(result);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_TestCase_001, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(false));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType("vfat");
    EXPECT_FALSE(result);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_TestCase_002, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(true));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType("vfat");
    EXPECT_TRUE(result);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_TestCase_003, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseEnabledForFsType(_)).WillOnce(Return(false));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType("");
    EXPECT_FALSE(result);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_TestCase_004, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseEnabledForFsType("vfat")).WillOnce(Return(true));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType("vfat");
    EXPECT_TRUE(result);
}

HWTEST_F(UsbFuseAdapterTest, IsUsbFuseEnabledForFsType_TestCase_005, TestSize.Level0)
{
    MockUsbFuseAdapter &mock = MockUsbFuseAdapter::GetInstance();
    EXPECT_CALL(mock, IsUsbFuseEnabledForFsType("ntfs")).WillOnce(Return(false));
    bool result = UsbFuseAdapter::GetInstance().IsUsbFuseEnabledForFsType("ntfs");
    EXPECT_FALSE(result);
}

} // namespace DiskManager
} // namespace OHOS