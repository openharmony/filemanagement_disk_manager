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

#ifndef OHOS_DISK_MANAGER_TEST_MOCK_ACCESSTOKEN_KIT_H
#define OHOS_DISK_MANAGER_TEST_MOCK_ACCESSTOKEN_KIT_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace Security {
namespace AccessToken {

using AccessTokenID = uint32_t;

enum ATokenTypeEnum : uint32_t {
    TOKEN_INVALID = 0,
    TOKEN_HAP = 1,
    TOKEN_NATIVE = 2,
    TOKEN_SHELL = 3,
};

enum PermissionState : int32_t {
    PERMISSION_DENIED = -1,
    PERMISSION_GRANTED = 0,
};

class AccessTokenKit {
public:
    static ATokenTypeEnum GetTokenTypeFlag(AccessTokenID tokenID);
    static int32_t VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName);
    static bool IsSystemAppByFullTokenID(uint64_t fullTokenId);
};

} // namespace AccessToken
} // namespace Security
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_TEST_MOCK_ACCESSTOKEN_KIT_H