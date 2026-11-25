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

#include "getdisksysinfo_fuzzer.h"

#include "disk_manager_stub.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "ipc/disk_manager_provider.h"
#include "message_option.h"
#include "message_parcel.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace {

constexpr size_t MAX_STRING_LEN = 4096;
// 与 interfaces/innerkits/IDiskManager.idl 中各方法的 [ipccode] 一致
constexpr uint32_t GET_DISK_SERIAL_NUMBER_IPC_CODE = 54;
constexpr uint32_t GET_DISK_POWER_CYCLE_COUNT_IPC_CODE = 55;
constexpr uint32_t GET_DISK_POWER_ON_HOURS_IPC_CODE = 56;
constexpr uint32_t DISK_SYS_INFO_IPC_CODES[] = {
    GET_DISK_SERIAL_NUMBER_IPC_CODE,
    GET_DISK_POWER_CYCLE_COUNT_IPC_CODE,
    GET_DISK_POWER_ON_HOURS_IPC_CODE,
};

} // namespace

bool GetDiskSysInfoFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    std::string sysPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);

    MessageParcel reply;
    MessageOption option;
    static DiskManager::DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);

    // 1. 结构化 parcel：InterfaceToken + sysPath，覆盖三个 sysPath 查询接口
    for (uint32_t code : DISK_SYS_INFO_IPC_CODES) {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(sysPath);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(code, datas, reply, option);
    }

    // 2. 原始字节 parcel：截断报文 / 缺参数 / 非字符串数据
    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    for (uint32_t code : DISK_SYS_INFO_IPC_CODES) {
        MessageParcel rawParcel;
        rawParcel.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        if (!remaining.empty()) {
            rawParcel.WriteBuffer(remaining.data(), remaining.size());
        }
        rawParcel.RewindRead(0);
        (void)provider.OnRemoteRequest(code, rawParcel, reply, option);
    }

    return true;
}

} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::GetDiskSysInfoFuzzTest(data, size);
    return 0;
}
