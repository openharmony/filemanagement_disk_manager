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

#include "accesstoken_kit.h"

int32_t g_accessTokenType = 1;
bool g_isSystemApp = true;
int32_t g_permissionGranted = 0;

namespace OHOS {
namespace Security {
namespace AccessToken {

ATokenTypeEnum AccessTokenKit::GetTokenTypeFlag(AccessTokenID tokenID)
{
    if (g_accessTokenType == -1) {
        return TOKEN_INVALID;
    }
    if (g_accessTokenType == 0) {
        return TOKEN_HAP;
    }
    if (g_accessTokenType == 1) {
        return TOKEN_NATIVE;
    }
    return TOKEN_NATIVE;
}

int32_t AccessTokenKit::VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName)
{
    return g_permissionGranted;
}

bool AccessTokenKit::IsSystemAppByFullTokenID(uint64_t fullTokenId)
{
    return g_isSystemApp;
}

int32_t AccessTokenKit::GetNativeTokenInfo(AccessTokenID tokenID, NativeTokenInfo &nativeTokenInfoRes)
{
    nativeTokenInfoRes.processName = "foundation";
    return 0;
}

int32_t AccessTokenKit::GetHapTokenInfo(AccessTokenID tokenID, HapTokenInfo &hapTokenInfoRes)
{
    hapTokenInfoRes.bundleName = "com.ohos.test";
    return 0;
}

} // namespace AccessToken
} // namespace Security
} // namespace OHOS