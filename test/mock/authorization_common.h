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

#ifndef OHOS_DISK_MANAGER_TEST_MOCK_AUTHORIZATION_COMMON_H
#define OHOS_DISK_MANAGER_TEST_MOCK_AUTHORIZATION_COMMON_H

#include <optional>
#include <string>
#include <vector>

#include "iremote_object.h"

namespace OHOS {
namespace AccountSA {

enum AuthorizationResultCode : int32_t {
    AUTHORIZATION_SUCCESS = 0,
    AUTHORIZATION_CANCELED = 12300301,
    AUTHORIZATION_INTERACTION_NOT_ALLOWED = 12300302,
    AUTHORIZATION_DENIED = 12300303,
    AUTHORIZATION_SERVICE_BUSY = 12300304,
    AUTHORIZATION_PRIVILEGE_NOT_SUPPORTED = 12300305,
};

class AuthorizationResult {
public:
    AuthorizationResult() = default;
    std::string privilege = "";
    AuthorizationResultCode resultCode = AuthorizationResultCode::AUTHORIZATION_SUCCESS;
    bool isReused = true;
    int32_t validityPeriod = 0;
    std::vector<uint8_t> token;
};

class ConnectAbilityInfo {
public:
    ConnectAbilityInfo() = default;
    std::string privilege = "";
    std::string description = "";
    std::string callingBundleName = "";
    std::string bundleName = "";
    std::string abilityName = "";
    int32_t callingUid = -1;
    int32_t callingPid = -1;
    int32_t timeout = -1;
    std::vector<uint8_t> challenge;
    std::string sessionId = "";
    std::string customName = "";
    bool isPublicApi = false;
};

struct SubjectInfo {
    int32_t pid = -1;
    int32_t uid = -1;
    std::string customName = "";
};

class AcquireAuthorizationOptions {
public:
    AcquireAuthorizationOptions() = default;
    bool hasContext = false;
    std::vector<uint8_t> challenge;
    bool isReuseNeeded = true;
    bool isInteractionAllowed = true;
    bool isContextValid = false;
    bool isPublicApi = false;
    std::optional<SubjectInfo> subjectInfo = std::nullopt;
};

} // namespace AccountSA
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_TEST_MOCK_AUTHORIZATION_COMMON_H
