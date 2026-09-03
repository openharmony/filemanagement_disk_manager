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
#include <securec.h>

#include "adapter/edm_adapter.h"
#include "disk.h"
#include "disk_manager_errno.h"
#include "disk_manager_mock.h"
#include "mock_parameters.h"

using namespace OHOS::DiskManager;
using namespace testing;
using namespace testing::ext;

class EdmAdapterTest : public testing::Test {
public:
    void SetUp() override
    {
        OHOS::system::g_mockGetParameterResult.clear();
    }
    void TearDown() override
    {
        auto &dm = DiskManager::GetInstance();
        Mock::VerifyAndClearExpectations(&dm);
        Mock::AllowLeak(&dm);
    }
};

static void SetEnterpriseParameter()
{
    OHOS::system::g_mockGetParameterResult = "true";
}

HWTEST_F(EdmAdapterTest, GetInstance_Singleton_001, TestSize.Level0)
{
    auto &a1 = EdmAdapter::GetInstance();
    auto &a2 = EdmAdapter::GetInstance();
    EXPECT_EQ(&a1, &a2);
}

HWTEST_F(EdmAdapterTest, IsEdmEnableOddBurn_NonEnterprise_ReturnsTrue_001, TestSize.Level0)
{
    OHOS::system::g_mockGetParameterResult.clear();
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsEdmEnableOddBurn("disk-1", 100));
}

HWTEST_F(EdmAdapterTest, IsEdmEnableOddBurn_EnterpriseDiskNotFound_ReturnsTrue_001, TestSize.Level0)
{
    SetEnterpriseParameter();
    auto &dm = DiskManager::GetInstance();
    EXPECT_CALL(dm, GetDiskById("disk-1", _))
        .WillOnce(Return(-1));
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsEdmEnableOddBurn("disk-1", 100));
}

HWTEST_F(EdmAdapterTest, IsEdmEnableOddBurn_EnterpriseNotCd_ReturnsTrue_001, TestSize.Level0)
{
    SetEnterpriseParameter();
    Disk disk("disk-1", 4096, "sda", USB_FLAG);
    auto &dm = DiskManager::GetInstance();
    EXPECT_CALL(dm, GetDiskById("disk-1", _))
        .WillOnce(DoAll(SetArgReferee<1>(disk), Return(E_OK)));
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsEdmEnableOddBurn("disk-1", 100));
}

HWTEST_F(EdmAdapterTest, IsEdmEnableOddBurn_EnterpriseSataOddNotDisabled_ReturnsTrue_001, TestSize.Level0)
{
    SetEnterpriseParameter();
    Disk disk("disk-1", 4096, "sr0", CD_FLAG);
    disk.sysPath_ = "/dev/block/ata/sr0";
    auto &dm = DiskManager::GetInstance();
    EXPECT_CALL(dm, GetDiskById("disk-1", _))
        .WillOnce(DoAll(SetArgReferee<1>(disk), Return(E_OK)));
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsEdmEnableOddBurn("disk-1", 100));
}

HWTEST_F(EdmAdapterTest, IsEdmEnableOddBurn_EnterpriseExternalOdd_ReturnsTrue_001, TestSize.Level0)
{
    SetEnterpriseParameter();
    Disk disk("disk-1", 4096, "sr0", CD_FLAG);
    disk.SetVendorId("0781");
    disk.SetProductId("5581");
    disk.SetSerialNumber("SN001");
    auto &dm = DiskManager::GetInstance();
    EXPECT_CALL(dm, GetDiskById("disk-1", _))
        .WillOnce(DoAll(SetArgReferee<1>(disk), Return(E_OK)));
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_FALSE(adapter.IsEdmEnableOddBurn("disk-1", 100));
}

HWTEST_F(EdmAdapterTest, IsExternalOddBurnAllowed_ReturnsTrue_001, TestSize.Level0)
{
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_TRUE(adapter.IsExternalOddBurnAllowed(100, "5581", "0781", "SN001"));
}

HWTEST_F(EdmAdapterTest, IsEdmControlMountEnabled_NonEnterprise_ReturnsFalse_001, TestSize.Level0)
{
    OHOS::system::g_mockGetParameterResult.clear();
    VolumeExternal vol;
    MountParam param;
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_FALSE(adapter.IsEdmControlMountEnabled(vol, param));
}

HWTEST_F(EdmAdapterTest, IsEdmControlMountEnabled_FromEdmMountSkipIntercept_ReturnsFalse_001, TestSize.Level0)
{
    OHOS::system::g_mockGetParameterResult = "true";
    VolumeExternal vol;
    MountParam param;
    param.SetFromEdmMount(true);
    auto &adapter = EdmAdapter::GetInstance();
    EXPECT_FALSE(adapter.IsEdmControlMountEnabled(vol, param));
}
