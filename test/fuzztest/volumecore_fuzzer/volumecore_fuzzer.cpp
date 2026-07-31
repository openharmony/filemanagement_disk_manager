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

#include "volumecore_fuzzer.h"

#include "volume_core.h"
#include "parcel.h"

#include <memory>

namespace OHOS {
using namespace DiskManager;

bool VolumeCoreFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return false;
    }

    int32_t flag = *(reinterpret_cast<const int32_t *>(data));

    Parcel parcel1;
    VolumeCore vol("vol-1-1", 2, "disk-1-1", flag, "ext4", "extra");
    vol.SetState(flag);
    vol.SetExtraInfo("extraInfo");
    vol.GetId();
    vol.GetType();
    vol.GetDiskId();
    vol.GetState();
    vol.GetFsType();
    vol.GetExtraInfo();
    vol.Marshalling(parcel1);
    auto unmarshallingVol = std::unique_ptr<VolumeCore>(VolumeCore::Unmarshalling(parcel1));

    Parcel parcel2;
    VolumeInfoStr infoStr("vol-1-1", "ext4", "uuid-abc", "/mnt/data/usb", "desc", true);
    infoStr.Marshalling(parcel2);
    auto unmarshallingInfoStr = std::unique_ptr<VolumeInfoStr>(VolumeInfoStr::Unmarshalling(parcel2));

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::VolumeCoreFuzzTest(data, size);
    return 0;
}
