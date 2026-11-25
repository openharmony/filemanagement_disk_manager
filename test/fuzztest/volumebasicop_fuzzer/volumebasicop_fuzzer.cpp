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

#include "volumebasicop_fuzzer.h"

#include "disk_manager_stub.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "ipc/disk_manager_provider.h"
#include "message_option.h"
#include "message_parcel.h"
#include "mount_param.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace {

constexpr size_t MAX_STRING_LEN = 4096;
// 与 interfaces/innerkits/IDiskManager.idl 中各方法的 [ipccode] 一致
constexpr uint32_t MOUNT_IPC_CODE = 1;
constexpr uint32_t UNMOUNT_IPC_CODE = 2;
constexpr uint32_t FORMAT_IPC_CODE = 3;
constexpr uint32_t TRY_TO_FIX_IPC_CODE = 4;
constexpr uint32_t SET_VOLUME_DESCRIPTION_IPC_CODE = 5;
constexpr uint32_t VOLUME_BASIC_OP_IPC_CODES[] = {
    MOUNT_IPC_CODE,
    UNMOUNT_IPC_CODE,
    FORMAT_IPC_CODE,
    TRY_TO_FIX_IPC_CODE,
    SET_VOLUME_DESCRIPTION_IPC_CODE,
};

} // namespace

bool VolumeBasicOpFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    std::string volumeId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string fsUuid = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string fsType = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string description = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    bool readOnly = fdp.ConsumeBool();
    std::string mountData = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);

    MessageParcel reply;
    MessageOption option;
    static DiskManager::DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);

    // 1. Mount（ipccode 1）：volumeId + MountParam
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(volumeId);
        DiskManager::MountParam mountParam(readOnly);
        mountParam.SetMountData(mountData);
        mountParam.Marshalling(datas);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(MOUNT_IPC_CODE, datas, reply, option);
    }

    // 2. Unmount（ipccode 2）：volumeId
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(volumeId);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(UNMOUNT_IPC_CODE, datas, reply, option);
    }

    // 3. Format（ipccode 3）：volumeId + fsType
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(volumeId);
        datas.WriteString(fsType);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(FORMAT_IPC_CODE, datas, reply, option);
    }

    // 4. TryToFix（ipccode 4）：volumeId
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(volumeId);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(TRY_TO_FIX_IPC_CODE, datas, reply, option);
    }

    // 5. SetVolumeDescription（ipccode 5）：fsUuid + description
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(fsUuid);
        datas.WriteString(description);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(SET_VOLUME_DESCRIPTION_IPC_CODE, datas, reply, option);
    }

    // 6. 原始字节 parcel：截断报文 / 缺参数 / 非字符串数据，遍历全组 code
    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    for (uint32_t code : VOLUME_BASIC_OP_IPC_CODES) {
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
    OHOS::VolumeBasicOpFuzzTest(data, size);
    return 0;
}
