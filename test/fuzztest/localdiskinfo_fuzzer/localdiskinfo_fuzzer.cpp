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

#include "localdiskinfo_fuzzer.h"
#include "local_disk_info.h"
#include "parcel.h"

#include <memory>
#include <cstdint>
#include <cstring>

namespace OHOS {
using namespace DiskManager;

void FuzzLocalDiskInfoSetters(const int32_t flag, LocalDiskInfo &disk)
{
    disk.SetSysPath(std::string("/sys/block/nvme0n1"));
    disk.SetModel(std::string("test_model"));
    disk.SetDescription(std::string("test_model"));
    disk.SetSize(static_cast<int64_t>(flag));
    disk.SetMediaType(flag);
    disk.SetState(std::string("live"));
}

bool LocalDiskInfoFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int32_t)) {
        return false;
    }
    int32_t flag = *(reinterpret_cast<const int32_t *>(data));
    LocalDiskInfo disk;
    FuzzLocalDiskInfoSetters(flag, disk);

    Parcel parcel;
    if (!disk.Marshalling(parcel)) {
        return false;
    }
    auto result = std::unique_ptr<LocalDiskInfo>(LocalDiskInfo::Unmarshalling(parcel));
    if (result != nullptr) {
        result->GetSysPath();
        result->GetModel();
        result->GetDescription();
        result->GetSize();
        result->GetMediaType();
        result->GetState();
    }
    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::LocalDiskInfoFuzzTest(data, size);
    return 0;
}
