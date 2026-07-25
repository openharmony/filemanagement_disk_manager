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
#include "adapter/pc_encryption_adapter.h"

namespace OHOS {
namespace DiskManager {

using namespace testing::ext;

class PcEncryptionAdapterTest : public testing::Test {
public:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(PcEncryptionAdapterTest, QueryEncryptionStatus_HandlerNullptr_001, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    int32_t encStatus = 0;
    EXPECT_FALSE(adapter.QueryEncryptionStatus("/mnt/data/voldata/data1", encStatus));
}

HWTEST_F(PcEncryptionAdapterTest, QueryEncryptionStatus_HandlerNullptr_002, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    int32_t encStatus = 0;
    EXPECT_FALSE(adapter.QueryEncryptionStatus("/mnt/data/voldata/data2", encStatus));
}

HWTEST_F(PcEncryptionAdapterTest, NotifyVolumeMounted_HandlerNullptr_001, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    EXPECT_NO_FATAL_FAILURE(adapter.NotifyVolumeMounted("disk-1", "vol-1", "/mnt/data/voldata/data1"));
}

HWTEST_F(PcEncryptionAdapterTest, NotifyVolumeMounted_HandlerNullptr_002, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    EXPECT_NO_FATAL_FAILURE(adapter.NotifyVolumeMounted("disk-2", "vol-2", "/mnt/data/voldata/data2"));
}

HWTEST_F(PcEncryptionAdapterTest, GetInstance_001, TestSize.Level0)
{
    auto &adapter1 = PcEncryptionAdapter::GetInstance();
    auto &adapter2 = PcEncryptionAdapter::GetInstance();
    EXPECT_EQ(&adapter1, &adapter2);
}

} // namespace DiskManager
} // namespace OHOS