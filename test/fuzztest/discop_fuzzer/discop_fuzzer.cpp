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

#include "discop_fuzzer.h"

#include "disk_manager_stub.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "ipc/disk_manager_provider.h"
#include "message_option.h"
#include "message_parcel.h"
#include "system_ability_definition.h"

#include <string>
#include <vector>

namespace OHOS {
namespace {

constexpr size_t MAX_STRING_LEN = 4096;
// 与 interfaces/innerkits/IDiskManager.idl 中各方法的 [ipccode] 一致
constexpr uint32_t ERASE_IPC_CODE = 40;
constexpr uint32_t EJECT_IPC_CODE = 41;
constexpr uint32_t CREATE_ISO_IMAGE_IPC_CODE = 42;
constexpr uint32_t BURN_IPC_CODE = 43;
constexpr uint32_t GET_VOLUME_OP_PROCESS_IPC_CODE = 44;
constexpr uint32_t DISC_OP_IPC_CODES[] = {
    ERASE_IPC_CODE,
    EJECT_IPC_CODE,
    CREATE_ISO_IMAGE_IPC_CODE,
    BURN_IPC_CODE,
    GET_VOLUME_OP_PROCESS_IPC_CODE,
};

void FuzzErase(DiskManager::DiskManagerProvider &provider, const std::string &volumeId)
{
    MessageParcel datas;
    datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
    datas.WriteString(volumeId);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    (void)provider.OnRemoteRequest(ERASE_IPC_CODE, datas, reply, option);
}

void FuzzEject(DiskManager::DiskManagerProvider &provider, const std::string &diskId)
{
    MessageParcel datas;
    datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
    datas.WriteString(diskId);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    (void)provider.OnRemoteRequest(EJECT_IPC_CODE, datas, reply, option);
}

void FuzzCreateIsoImage(DiskManager::DiskManagerProvider &provider, const std::string &volumeId,
                        const std::string &filePath)
{
    MessageParcel datas;
    datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
    datas.WriteString(volumeId);
    datas.WriteString(filePath);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    (void)provider.OnRemoteRequest(CREATE_ISO_IMAGE_IPC_CODE, datas, reply, option);
}

void FuzzBurn(DiskManager::DiskManagerProvider &provider, const std::string &volumeId,
              const std::string &burnOptions)
{
    MessageParcel datas;
    datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
    datas.WriteString(volumeId);
    datas.WriteString(burnOptions);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    (void)provider.OnRemoteRequest(BURN_IPC_CODE, datas, reply, option);
}

void FuzzGetVolumeOpProcess(DiskManager::DiskManagerProvider &provider, const std::string &volumeId)
{
    MessageParcel datas;
    datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
    datas.WriteString(volumeId);
    datas.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    (void)provider.OnRemoteRequest(GET_VOLUME_OP_PROCESS_IPC_CODE, datas, reply, option);
}

void FuzzRawParcels(DiskManager::DiskManagerProvider &provider, const std::vector<uint8_t> &remaining)
{
    MessageParcel reply;
    MessageOption option;
    for (uint32_t code : DISC_OP_IPC_CODES) {
        MessageParcel rawParcel;
        rawParcel.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        if (!remaining.empty()) {
            rawParcel.WriteBuffer(remaining.data(), remaining.size());
        }
        rawParcel.RewindRead(0);
        (void)provider.OnRemoteRequest(code, rawParcel, reply, option);
    }
}

} // namespace

bool DiscOpFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    std::string volumeId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string filePath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string burnOptions = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);

    static DiskManager::DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    FuzzErase(provider, volumeId);
    FuzzEject(provider, diskId);
    FuzzCreateIsoImage(provider, volumeId, filePath);
    FuzzBurn(provider, volumeId, burnOptions);
    FuzzGetVolumeOpProcess(provider, volumeId);
    FuzzRawParcels(provider, fdp.ConsumeRemainingBytes<uint8_t>());

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DiscOpFuzzTest(data, size);
    return 0;
}
