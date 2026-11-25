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

#include "mountdmdevice_fuzzer.h"

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
constexpr uint32_t MOUNT_DM_DEVICE_IPC_CODE = 57;

} // namespace

bool MountDmDeviceFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    std::string diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string dmPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string dstPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string fsType = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    bool readOnly = fdp.ConsumeBool();
    std::string mountData = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);

    MessageParcel datas;
    datas.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
    datas.WriteString(diskId);
    datas.WriteString(dmPath);
    datas.WriteString(dstPath);
    datas.WriteString(fsType);
    DiskManager::MountParam mountParam(readOnly);
    mountParam.SetMountData(mountData);
    mountParam.Marshalling(datas);
    datas.RewindRead(0);

    MessageParcel reply;
    MessageOption option;
    static DiskManager::DiskManagerProvider provider(DISK_MANAGER_SA_ID, false);
    (void)provider.OnRemoteRequest(MOUNT_DM_DEVICE_IPC_CODE, datas, reply, option);

    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    MessageParcel rawParcel;
    rawParcel.WriteInterfaceToken(DiskManager::DiskManagerStub::GetDescriptor());
    if (!remaining.empty()) {
        rawParcel.WriteBuffer(remaining.data(), remaining.size());
    }
    rawParcel.RewindRead(0);
    (void)provider.OnRemoteRequest(MOUNT_DM_DEVICE_IPC_CODE, rawParcel, reply, option);

    return true;
}

} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::MountDmDeviceFuzzTest(data, size);
    return 0;
}
