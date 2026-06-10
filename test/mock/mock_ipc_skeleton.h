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

#ifndef OHOS_DISK_MANAGER_MOCK_IPC_SKELETON_H
#define OHOS_DISK_MANAGER_MOCK_IPC_SKELETON_H

#include <cstdint>
#include <string>

namespace OHOS {

class MockIPCSkeleton {
public:
    static pid_t mockCallingUid_;
    static pid_t mockCallingPid_;
    static uint32_t mockCallingTokenID_;
    static uint64_t mockCallingFullTokenID_;

    static MockIPCSkeleton &GetInstance()
    {
        static MockIPCSkeleton instance;
        return instance;
    }

    static pid_t GetCallingUid() { return mockCallingUid_; }
    static pid_t GetCallingPid() { return mockCallingPid_; }
    static uint32_t GetCallingTokenID() { return mockCallingTokenID_; }
    static uint64_t GetCallingFullTokenID() { return mockCallingFullTokenID_; }
    static bool IsLocalCalling() { return true; }
    static std::string ResetCallingIdentity() { return ""; }
    static bool SetCallingIdentity(std::string &identity, bool flag = false) { return true; }
};

} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_IPC_SKELETON_H