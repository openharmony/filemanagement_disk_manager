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

#include "externaldiskinfo_fuzzer.h"

#include "external_disk_info.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "parcel.h"

#include <memory>
#include <string>
#include <vector>

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr int32_t MAX_VOLUME_ID_COUNT = 10;
constexpr size_t MAX_STRING_LEN = 4096;

void FuzzExternalDiskInfoSetters(FuzzedDataProvider &fdp, ExternalDiskInfo &info)
{
    info.SetDiskId(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
    info.SetDiskType(fdp.ConsumeIntegral<int32_t>());
    int32_t volCount = fdp.ConsumeIntegralInRange<int32_t>(0, MAX_VOLUME_ID_COUNT);
    std::vector<std::string> volIds;
    volIds.reserve(volCount);
    for (int32_t i = 0; i < volCount; ++i) {
        volIds.push_back(fdp.ConsumeRandomLengthString(MAX_STRING_LEN));
    }
    info.SetVolumeIds(std::move(volIds));
    info.SetVendorId(fdp.ConsumeIntegral<int32_t>());
    info.SetProductId(fdp.ConsumeIntegral<int32_t>());
}
} // namespace

bool ExternalDiskInfoFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);
    ExternalDiskInfo info;
    FuzzExternalDiskInfoSetters(fdp, info);

    info.GetDiskId();
    info.GetDiskType();
    info.GetVolumeIds();
    info.GetVendorId();
    info.GetProductId();

    Parcel parcel;
    info.Marshalling(parcel);
    auto unmarshallingInfo = std::unique_ptr<ExternalDiskInfo>(ExternalDiskInfo::Unmarshalling(parcel));

    auto remaining = fdp.ConsumeRemainingBytes<uint8_t>();
    Parcel rawParcel;
    if (!remaining.empty()) {
        rawParcel.WriteBuffer(remaining.data(), remaining.size());
    }
    rawParcel.RewindRead(0);
    auto rawInfo = std::unique_ptr<ExternalDiskInfo>(ExternalDiskInfo::Unmarshalling(rawParcel));

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::ExternalDiskInfoFuzzTest(data, size);
    return 0;
}
