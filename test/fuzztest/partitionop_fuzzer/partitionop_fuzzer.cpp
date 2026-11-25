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

#include "partitionop_fuzzer.h"

#include "disk_manager_stub.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "ipc/disk_manager_provider.h"
#include "message_option.h"
#include "message_parcel.h"
#include "partition_types.h"
#include "system_ability_definition.h"

namespace OHOS {
namespace {

constexpr size_t MAX_STRING_LEN = 4096;
// 与 interfaces/innerkits/IDiskManager.idl 中各方法的 [ipccode] 一致
constexpr uint32_t GET_PARTITION_TABLE_IPC_CODE = 46;
constexpr uint32_t CREATE_PARTITION_IPC_CODE = 47;
constexpr uint32_t DELETE_PARTITION_IPC_CODE = 48;
constexpr uint32_t FORMAT_PARTITION_IPC_CODE = 49;
constexpr uint32_t PARTITION_OP_IPC_CODES[] = {
    GET_PARTITION_TABLE_IPC_CODE,
    CREATE_PARTITION_IPC_CODE,
    DELETE_PARTITION_IPC_CODE,
    FORMAT_PARTITION_IPC_CODE,
};

} // namespace

bool PartitionOpFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    std::string diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    int32_t partitionNum = fdp.ConsumeIntegral<int32_t>();
    int64_t startSector = fdp.ConsumeIntegral<int64_t>();
    int64_t endSector = fdp.ConsumeIntegral<int64_t>();
    std::string typeCode = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);

    MessageParcel reply;
    MessageOption option;
    static DiskManager::DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);

    // 1. GetPartitionTable（ipccode 46）：diskId
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(diskId);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(GET_PARTITION_TABLE_IPC_CODE, datas, reply, option);
    }

    // 2. CreatePartition（ipccode 47）：diskId + PartitionParams
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(diskId);
        DiskManager::PartitionParams params(partitionNum, startSector, endSector, typeCode);
        params.Marshalling(datas);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(CREATE_PARTITION_IPC_CODE, datas, reply, option);
    }

    // 3. DeletePartition（ipccode 48）：diskId + partitionNum
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(diskId);
        datas.WriteInt32(partitionNum);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(DELETE_PARTITION_IPC_CODE, datas, reply, option);
    }

    // 4. FormatPartition（ipccode 49）：diskId + partitionNum + FormatParams
    {
        std::string fsType = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        bool quickFormat = fdp.ConsumeBool();
        std::string volumeName = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(diskId);
        datas.WriteInt32(partitionNum);
        DiskManager::FormatParams formatParams(fsType, quickFormat, volumeName);
        formatParams.Marshalling(datas);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(FORMAT_PARTITION_IPC_CODE, datas, reply, option);
    }

    // 5. 原始字节 parcel：截断报文 / 缺参数 / 坏 Parcelable 数据，遍历全组 code
    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    for (uint32_t code : PARTITION_OP_IPC_CODES) {
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
    OHOS::PartitionOpFuzzTest(data, size);
    return 0;
}
