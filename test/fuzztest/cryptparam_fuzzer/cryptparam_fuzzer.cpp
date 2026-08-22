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

#include "cryptparam_fuzzer.h"

#include "crypt_param.h"
#include "parcel.h"

#include <memory>

namespace OHOS {
using namespace DiskManager;

namespace {
const std::string DEFAULT_TYPE = "luks";
const std::string DEFAULT_CIPHER = "aes";
const std::string DEFAULT_KEY_FILE = "/keyfile";
} // namespace

void FuzzCryptParam(const int32_t flag, Parcel &parcel)
{
    CryptParam cryptParam(DEFAULT_TYPE, DEFAULT_CIPHER, flag, DEFAULT_KEY_FILE);
    cryptParam.SetType(DEFAULT_TYPE);
    cryptParam.SetCipher(DEFAULT_CIPHER);
    cryptParam.SetKeySize(flag);
    cryptParam.SetKeyFile(DEFAULT_KEY_FILE);
    cryptParam.GetType();
    cryptParam.GetCipher();
    cryptParam.GetKeySize();
    cryptParam.GetKeyFile();
    cryptParam.Marshalling(parcel);
    auto unmarshallingCryptParam = std::unique_ptr<CryptParam>(CryptParam::Unmarshalling(parcel));
}

bool CryptParamFuzzTest(const uint8_t *data, size_t size)
{
    if ((data == nullptr) || (size < sizeof(int32_t))) {
        return false;
    }

    int32_t flag = *(reinterpret_cast<const int32_t *>(data));

    Parcel parcel;
    FuzzCryptParam(flag, parcel);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::CryptParamFuzzTest(data, size);
    return 0;
}
