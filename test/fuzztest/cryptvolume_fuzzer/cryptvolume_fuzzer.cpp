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

#include "cryptvolume_fuzzer.h"

#include "crypt_param.h"
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
constexpr uint32_t BIND_BLOCK_LOOP_DEV_IPC_CODE = 15;
constexpr uint32_t CREATE_DM_CRYPT_VOLUME_IPC_CODE = 16;
constexpr uint32_t DESTROY_DM_CRYPT_VOLUME_IPC_CODE = 17;
constexpr uint32_t UNBIND_BLOCK_LOOP_DEV_IPC_CODE = 18;
constexpr uint32_t MOUNT_VOLUME_BY_PATH_IPC_CODE = 19;
constexpr uint32_t UMOUNT_VOLUME_BY_PATH_IPC_CODE = 20;
constexpr uint32_t CRYPT_VOLUME_IPC_CODES[] = {
    BIND_BLOCK_LOOP_DEV_IPC_CODE,
    CREATE_DM_CRYPT_VOLUME_IPC_CODE,
    DESTROY_DM_CRYPT_VOLUME_IPC_CODE,
    UNBIND_BLOCK_LOOP_DEV_IPC_CODE,
    MOUNT_VOLUME_BY_PATH_IPC_CODE,
    UMOUNT_VOLUME_BY_PATH_IPC_CODE,
};

} // namespace

bool CryptVolumeFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    std::string diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string mapperName = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string loopPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string volPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    uint64_t offset = fdp.ConsumeIntegral<uint64_t>();
    uint64_t sizeLimit = fdp.ConsumeIntegral<uint64_t>();

    MessageParcel reply;
    MessageOption option;
    static DiskManager::DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);

    // 1. BindBlockLoopDev（ipccode 15）：diskId + offset + sizeLimit
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(diskId);
        datas.WriteUint64(offset);
        datas.WriteUint64(sizeLimit);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(BIND_BLOCK_LOOP_DEV_IPC_CODE, datas, reply, option);
    }

    // 2. CreateDmCryptVolume（ipccode 16）：CryptParam + loopPath + mapperName（in/out）
    {
        std::string type = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        std::string cipher = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        int32_t keySize = fdp.ConsumeIntegral<int32_t>();
        std::string keyFile = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        DiskManager::CryptParam cryptParam(type, cipher, keySize, keyFile);
        cryptParam.Marshalling(datas);
        datas.WriteString(loopPath);
        datas.WriteString(mapperName);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(CREATE_DM_CRYPT_VOLUME_IPC_CODE, datas, reply, option);
    }

    // 3. DestroyDmCryptVolume（ipccode 17）：mapperName
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(mapperName);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(DESTROY_DM_CRYPT_VOLUME_IPC_CODE, datas, reply, option);
    }

    // 4. UnbindBlockLoopDev（ipccode 18）：loopPath
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(loopPath);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(UNBIND_BLOCK_LOOP_DEV_IPC_CODE, datas, reply, option);
    }

    // 5. MountVolumeByPath（ipccode 19）：diskId + volPath + MountParam
    {
        bool readOnly = fdp.ConsumeBool();
        std::string mountData = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(diskId);
        datas.WriteString(volPath);
        DiskManager::MountParam mountParam(readOnly);
        mountParam.SetMountData(mountData);
        mountParam.Marshalling(datas);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(MOUNT_VOLUME_BY_PATH_IPC_CODE, datas, reply, option);
    }

    // 6. UmountVolumeByPath（ipccode 20）：diskId + volPath
    {
        MessageParcel datas;
        datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
        datas.WriteString(diskId);
        datas.WriteString(volPath);
        datas.RewindRead(0);
        (void)provider.OnRemoteRequest(UMOUNT_VOLUME_BY_PATH_IPC_CODE, datas, reply, option);
    }

    // 7. 原始字节 parcel：截断报文 / 缺参数 / 坏 Parcelable 数据，遍历全组 code
    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    for (uint32_t code : CRYPT_VOLUME_IPC_CODES) {
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
    OHOS::CryptVolumeFuzzTest(data, size);
    return 0;
}
