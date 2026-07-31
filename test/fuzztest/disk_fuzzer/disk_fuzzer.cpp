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

#include "disk_fuzzer.h"

#include "disk.h"
#include "parcel.h"

#include <memory>

namespace OHOS {
using namespace DiskManager;

bool DiskFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return false;
    }

    Parcel parcel;
    Disk disk("disk-1-1", 1024, "sda", 2);

    int32_t flag = *(reinterpret_cast<const int32_t *>(data));
    disk.SetDiskType(flag);
    disk.SetSizeBytes(static_cast<int64_t>(flag));
    disk.GetDiskId();
    disk.GetSizeBytes();
    disk.GetDiskType();
    disk.GetRemovable();
    disk.IsRemovable();
    disk.GetExtraInfo();
    disk.GetVendor();
    disk.GetCdromState();
    disk.GetSysPath();
    disk.GetDevName();
    disk.IsInternalDataDisk();

    disk.Marshalling(parcel);
    auto unmarshallingDisk = std::unique_ptr<Disk>(Disk::Unmarshalling(parcel));

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DiskFuzzTest(data, size);
    return 0;
}
