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

#include "ipc_caller_auth.h"
#include "mock_ipc_skeleton.h"

namespace OHOS::DiskManager {
namespace {
constexpr int32_t UID_BASE_DIVISOR = 200000;
}

TEST(IpcCallerAuthAppUidTest, GetCallingAppUidReturnsMockValue)
{
    MockIPCSkeleton::mockCallingAppUid_ = 100 * UID_BASE_DIVISOR + 10000;
    EXPECT_EQ(IpcCallerAuth::GetCallingAppUid(), 100 * UID_BASE_DIVISOR + 10000);
}

TEST(IpcCallerAuthAppUidTest, GetCallingUserIdFallsBackToAppUid)
{
    MockIPCSkeleton::mockCallingAppUid_ = 101 * UID_BASE_DIVISOR + 20000;
    EXPECT_EQ(IpcCallerAuth::GetCallingUserId(), 101);
}

TEST(IpcCallerAuthAppUidTest, GetCallingUserIdUsesAppUidNotRealUid)
{
    MockIPCSkeleton::mockCallingAppUid_ = 102 * UID_BASE_DIVISOR + 30000;
    MockIPCSkeleton::mockCallingUid_ = 102;
    EXPECT_EQ(IpcCallerAuth::GetCallingUserId(), 102);
    MockIPCSkeleton::mockCallingUid_ = 0;
    MockIPCSkeleton::mockCallingAppUid_ = 0;
}
} // namespace OHOS::DiskManager
