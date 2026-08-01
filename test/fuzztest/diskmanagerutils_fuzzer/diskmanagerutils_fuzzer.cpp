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

#include "diskmanagerutils_fuzzer.h"

#include "disk_manager_utils.h"
#include "fuzzer/FuzzedDataProvider.h"

#include <string>

namespace OHOS {
using namespace DiskManager;

bool DiskManagerUtilsFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);

    std::string str1(fdp.ConsumeRandomLengthString(size));
    std::string str2(fdp.ConsumeRandomLengthString(size));
    std::string str3(fdp.ConsumeRandomLengthString(size));
    std::string str4(fdp.ConsumeRandomLengthString(size));
    std::string str5(fdp.ConsumeRandomLengthString(size));

    GetAnonyString(str1);

    IsFilePathInvalid(str1);
    IsFilePathInvalid(str2);

    IsMountPathValid(str3);

    IsVolumeIdValid(str4);

    IsDiskIdValid(str5);

    IsUuidValid(str1);

    size_t minLen = fdp.ConsumeIntegralInRange<size_t>(0, 20);
    size_t maxLen = fdp.ConsumeIntegralInRange<size_t>(minLen, 40);
    IsPureDigitsInRange(str2, minLen, maxLen);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DiskManagerUtilsFuzzTest(data, size);
    return 0;
}
