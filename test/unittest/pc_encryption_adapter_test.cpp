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

#include <atomic>
#include <chrono>
#include <thread>

#include "disk_manager_errno.h"
#include "disk_manager_utils.h"
#include "mock_dlfcn.h"
#include "adapter/pc_encryption_adapter.h"

namespace OHOS {
namespace DiskManager {

namespace {
constexpr int32_t MOCK_ENC_STATUS = 42;

int32_t MockQueryEncStatus(const std::string &volPath, int32_t &encStatus)
{
    encStatus = MOCK_ENC_STATUS;
    return 0;
}

std::atomic<int32_t> g_notifyCallCount{0};
int32_t MockNotifyMounted(const std::string &diskId, const std::string &volumeId,
                          const std::string &volPath)
{
    g_notifyCallCount.fetch_add(1);
    return 0;
}
} // namespace

using namespace testing::ext;

class PcEncryptionAdapterTest : public testing::Test {
public:
    void SetUp() override
    {
        auto &adapter = PcEncryptionAdapter::GetInstance();
        savedHandler_ = adapter.handler_;
    }
    void TearDown() override
    {
        auto &adapter = PcEncryptionAdapter::GetInstance();
        adapter.handler_ = savedHandler_;
        MockDlfcnConfig::GetInstance().ClearDlsymResults();
    }
private:
    void *savedHandler_ = nullptr;
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

HWTEST_F(PcEncryptionAdapterTest, QueryEncryptionStatus_Success_001, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    adapter.handler_ = reinterpret_cast<void *>(0x1);
    MockDlfcnConfig::GetInstance().SetDlsymResult(
        "PC_ENC_EXT_QueryVolEncryptionStatus",
        reinterpret_cast<uintptr_t>(&MockQueryEncStatus));

    int32_t encStatus = -1;
    EXPECT_TRUE(adapter.QueryEncryptionStatus("/mnt/data/voldata/data1", encStatus));
    EXPECT_EQ(encStatus, MOCK_ENC_STATUS);
}

HWTEST_F(PcEncryptionAdapterTest, QueryEncryptionStatus_DlsymFailed_001, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    adapter.handler_ = reinterpret_cast<void *>(0x1);

    int32_t encStatus = -1;
    EXPECT_FALSE(adapter.QueryEncryptionStatus("/mnt/data/voldata/data1", encStatus));
}

HWTEST_F(PcEncryptionAdapterTest, NotifyVolumeMounted_Success_001, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    adapter.handler_ = reinterpret_cast<void *>(0x1);
    MockDlfcnConfig::GetInstance().SetDlsymResult(
        "PC_ENC_EXT_ConfigureVolEncryptionPolicyOnMounted",
        reinterpret_cast<uintptr_t>(&MockNotifyMounted));

    g_notifyCallCount = 0;
    EXPECT_NO_FATAL_FAILURE(adapter.NotifyVolumeMounted("disk-1", "vol-1", "/mnt/data/voldata/data1"));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(g_notifyCallCount.load(), 1);
}

HWTEST_F(PcEncryptionAdapterTest, NotifyVolumeMounted_MultipleTasks_001, TestSize.Level0)
{
    auto &adapter = PcEncryptionAdapter::GetInstance();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    adapter.handler_ = reinterpret_cast<void *>(0x1);
    MockDlfcnConfig::GetInstance().SetDlsymResult(
        "PC_ENC_EXT_ConfigureVolEncryptionPolicyOnMounted",
        reinterpret_cast<uintptr_t>(&MockNotifyMounted));

    g_notifyCallCount = 0;
    for (int i = 0; i < 5; i++) {
        EXPECT_NO_FATAL_FAILURE(adapter.NotifyVolumeMounted("disk-m", "vol-m",
                                                              "/mnt/data/voldata/dataM"));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(g_notifyCallCount.load(), 5);
}

} // namespace DiskManager
} // namespace OHOS