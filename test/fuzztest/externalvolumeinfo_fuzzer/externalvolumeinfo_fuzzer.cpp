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

#include "externalvolumeinfo_fuzzer.h"

#include "external_volume_info.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "parcel.h"

#include <memory>
#include <string>

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr size_t MAX_STRING_LEN = 4096;

void FuzzExternalVolumeInfoSetters(FuzzedDataProvider &fdp, ExternalVolumeInfo &info)
{
    info.SetVolumeId(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
    info.SetUuid(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
    info.SetDiskId(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
    info.SetDescription(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
    info.SetState(fdp.ConsumeIntegral<int32_t>());
    info.SetTotalSize(fdp.ConsumeIntegral<int64_t>());
    info.SetFreeSize(fdp.ConsumeIntegral<int64_t>());
    info.SetPath(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
    info.SetFsType(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
}
} // namespace

bool ExternalVolumeInfoFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    ExternalVolumeInfo info;
    FuzzExternalVolumeInfoSetters(fdp, info);

    info.GetVolumeId();
    info.GetUuid();
    info.GetDiskId();
    info.GetDescription();
    info.GetState();
    info.GetTotalSize();
    info.GetFreeSize();
    info.GetPath();
    info.GetFsType();

    Parcel parcel;
    info.Marshalling(parcel);
    auto unmarshallingInfo = std::unique_ptr<ExternalVolumeInfo>(ExternalVolumeInfo::Unmarshalling(parcel));

    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    Parcel rawParcel;
    if (!remaining.empty()) {
        rawParcel.WriteBuffer(remaining.data(), remaining.size());
    }
    rawParcel.RewindRead(0);
    auto rawInfo = std::unique_ptr<ExternalVolumeInfo>(ExternalVolumeInfo::Unmarshalling(rawParcel));

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::ExternalVolumeInfoFuzzTest(data, size);
    return 0;
}
