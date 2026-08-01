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

#include "volumeexternal_fuzzer.h"

#include "volume_external.h"
#include "parcel.h"

#include <memory>

namespace OHOS {
using namespace DiskManager;

bool VolumeExternalFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return false;
    }

    Parcel parcel;
    VolumeExternal vol;

    int32_t flag = *(reinterpret_cast<const int32_t *>(data));
    vol.SetFlags(flag);
    vol.SetFsType(flag);
    vol.SetFsUuid("test-uuid");
    vol.SetPath("/mnt/data/usb");
    vol.SetDescription("test-desc");
    vol.SetUserData(true);
    vol.SetFreeSize(static_cast<int64_t>(flag));
    vol.SetPartitionNum(flag);

    vol.GetFlags();
    vol.GetFsType();
    vol.GetFsTypeString();
    vol.GetUuid();
    vol.GetPath();
    vol.GetDescription();
    vol.GetUserData();
    vol.GetFreeSize();
    vol.GetPartitionNum();
    vol.GetFsTypeByStr("ext4");
    vol.Reset();

    vol.Marshalling(parcel);
    auto unmarshallingVol = std::unique_ptr<VolumeExternal>(VolumeExternal::Unmarshalling(parcel));

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::VolumeExternalFuzzTest(data, size);
    return 0;
}
