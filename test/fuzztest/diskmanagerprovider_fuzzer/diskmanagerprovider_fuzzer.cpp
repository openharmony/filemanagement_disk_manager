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

#include "diskmanagerprovider_fuzzer.h"

#include "accesstoken_kit.h"
#include "disk_manager_stub.h"
#include "ipc/disk_manager_provider.h"
#include "ipc_skeleton.h"
#include "message_option.h"
#include "message_parcel.h"
#include "system_ability_definition.h"

namespace OHOS::Security::AccessToken {
ATokenTypeEnum AccessTokenKit::GetTokenTypeFlag(AccessTokenID tokenID)
{
    return Security::AccessToken::TOKEN_NATIVE;
}

int AccessTokenKit::VerifyAccessToken(AccessTokenID tokenID, const std::string &permissionName)
{
    return Security::AccessToken::PermissionState::PERMISSION_GRANTED;
}

int AccessTokenKit::GetNativeTokenInfo(AccessTokenID tokenID, NativeTokenInfo &nativeTokenInfoRes)
{
    nativeTokenInfoRes.processName = "storage_daemon";
    return 0;
}
} // namespace OHOS::Security::AccessToken

namespace OHOS {
#ifdef CONFIG_IPC_SINGLE
using namespace IPC_SINGLE;
#endif
constexpr uint32_t MOCK_TOKEN_ID = 100;

pid_t IPCSkeleton::GetCallingUid()
{
    return 0;
}

uint32_t IPCSkeleton::GetCallingTokenID()
{
    return MOCK_TOKEN_ID;
}

uint64_t IPCSkeleton::GetCallingFullTokenID()
{
    return MOCK_TOKEN_ID;
}
} // namespace OHOS

namespace OHOS::DiskManager {
namespace {
constexpr uint8_t BYTE_SHIFT_8 = 8;
constexpr uint8_t BYTE_SHIFT_16 = 16;
constexpr uint8_t BYTE_SHIFT_24 = 24;
constexpr uint8_t BYTE_INDEX_0 = 0;
constexpr uint8_t BYTE_INDEX_1 = 1;
constexpr uint8_t BYTE_INDEX_2 = 2;
constexpr uint8_t BYTE_INDEX_3 = 3;

constexpr uint32_t DISK_MANAGER_IPC_CODES[] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    26, 27, 39, 40, 41, 42, 43, 44, 46, 47, 48, 49,
};

uint32_t GetU32Data(const uint8_t *data)
{
    return static_cast<uint32_t>((data[BYTE_INDEX_0]) | (data[BYTE_INDEX_1] << BYTE_SHIFT_8) |
        (data[BYTE_INDEX_2] << BYTE_SHIFT_16) | (data[BYTE_INDEX_3] << BYTE_SHIFT_24));
}
} // namespace

bool DiskManagerProviderFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(uint32_t)) {
        return false;
    }

    static DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);

    uint32_t code = GetU32Data(data) % (sizeof(DISK_MANAGER_IPC_CODES) / sizeof(uint32_t));
    code = DISK_MANAGER_IPC_CODES[code];

    MessageParcel datas;
    datas.WriteInterfaceToken(DiskManagerStub::GetDescriptor());
    if (size > sizeof(uint32_t)) {
        datas.WriteBuffer(data + sizeof(uint32_t), size - sizeof(uint32_t));
    }
    datas.RewindRead(0);

    MessageParcel reply;
    MessageOption option;
    (void)provider.OnRemoteRequest(code, datas, reply, option);
    return true;
}
} // namespace OHOS::DiskManager

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DiskManager::DiskManagerProviderFuzzTest(data, size);
    return 0;
}
