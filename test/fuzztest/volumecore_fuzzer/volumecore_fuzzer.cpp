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

namespace {
constexpr int32_t DEFAULT_VOLUME_TYPE = 2;
} // namespace

void FuzzVolumeCoreConstructors(const int32_t flag, Parcel &parcel)
{
    VolumeCore vol1("vol-1-1", DEFAULT_VOLUME_TYPE, "disk-1-1");
    vol1.Marshalling(parcel);
    auto unmarshallingVol1 = std::unique_ptr<VolumeCore>(VolumeCore::Unmarshalling(parcel));

    Parcel parcel2;
    VolumeCore vol2("vol-2-1", DEFAULT_VOLUME_TYPE, "disk-1-1", flag);
    vol2.Marshalling(parcel2);
    auto unmarshallingVol2 = std::unique_ptr<VolumeCore>(VolumeCore::Unmarshalling(parcel2));

    Parcel parcel3;
    VolumeCore vol3("vol-3-1", DEFAULT_VOLUME_TYPE, "disk-1-1", flag, "ext4", "extra");
    vol3.SetState(flag);
    vol3.SetExtraInfo("extraInfo");
    vol3.GetId();
    vol3.GetType();
    vol3.GetDiskId();
    vol3.GetState();
    vol3.GetFsType();
    vol3.GetExtraInfo();
    vol3.Marshalling(parcel3);
    auto unmarshallingVol3 = std::unique_ptr<VolumeCore>(VolumeCore::Unmarshalling(parcel3));
}

void FuzzVolumeInfoStrConstructors(Parcel &parcel)
{
    VolumeInfoStr infoStrDefault;
    VolumeInfoStr infoStr("vol-1-1", "ext4", "uuid-abc", "/mnt/data/usb", "desc", true);
    infoStr.Marshalling(parcel);
    auto unmarshallingInfoStr = std::unique_ptr<VolumeInfoStr>(VolumeInfoStr::Unmarshalling(parcel));
}

bool VolumeCoreFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return false;
    }

    int32_t flag = *(reinterpret_cast<const int32_t *>(data));

    Parcel parcel1;
    FuzzVolumeCoreConstructors(flag, parcel1);

    Parcel parcel4;
    FuzzVolumeInfoStrConstructors(parcel4);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::VolumeCoreFuzzTest(data, size);
    return 0;
}
