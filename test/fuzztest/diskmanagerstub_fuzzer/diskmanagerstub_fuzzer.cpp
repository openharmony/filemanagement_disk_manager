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

#include "diskmanagerstub_fuzzer.h"

#include "disk_manager_stub.h"
#include "ipc/disk_manager_provider.h"
#include "message_option.h"
#include "message_parcel.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace {

// 与 interfaces/innerkits/IDiskManager.idl 中各方法的 [ipccode] 一致；IDL 增删改码时请同步本表。
constexpr uint32_t DISK_MANAGER_IPC_CODES[] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 39, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 30, 31, 32, 33, 34, 35, 36, 37, 38,
};

} // namespace

bool DiskmanagerStubFuzzTest(DiskManager::DiskManagerProvider &provider, const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    for (uint32_t code : DISK_MANAGER_IPC_CODES) {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        if (size > 0) {
            datas.WriteBuffer(data, size);
        }
        datas.RewindRead(0);

        MessageParcel reply;
        MessageOption option;
        (void)provider.OnRemoteRequest(code, datas, reply, option);
    }
    return true;
}

} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    static OHOS::DiskManager::DiskManagerProvider provider(OHOS::DISK_MANAGER_SA_ID, false);
    OHOS::DiskmanagerStubFuzzTest(provider, data, size);
    return 0;
}
