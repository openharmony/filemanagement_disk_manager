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

#include "mock_uevent_bootstrap.h"

namespace OHOS {
namespace DiskManager {

MockUeventBootstrap *MockUeventBootstrap::mockInstance_ = nullptr;

MockUeventBootstrap &MockUeventBootstrap::GetInstance()
{
    if (mockInstance_ == nullptr) {
        mockInstance_ = new MockUeventBootstrap();
    }
    return *mockInstance_;
}

int32_t MockUeventBootstrap::OnBlockDiskUevent(const std::string &rawUeventMsg)
{
    return GetInstance().OnBlockDiskUeventImpl(rawUeventMsg);
}

void MockUeventBootstrap::Init()
{
    GetInstance().InitImpl();
}

uint32_t MockUeventBootstrap::MatchConfig(const UeventEnv &env)
{
    return GetInstance().MatchConfigImpl(env);
}

int32_t MockUeventBootstrap::RediscoverDiskVolumes(const std::string &diskId)
{
    return GetInstance().RediscoverDiskVolumesImpl(diskId);
}

} // namespace DiskManager
} // namespace OHOS