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

#ifndef OHOS_DISK_MANAGER_CRYPT_PARAM_H
#define OHOS_DISK_MANAGER_CRYPT_PARAM_H

#include "parcel.h"

#include <cstdint>
#include <string>

namespace OHOS {
namespace DiskManager {

class CryptParam : public Parcelable {
public:
    CryptParam() = default;
    CryptParam(const std::string &type, const std::string &cipher,
               int32_t keySize, const std::string &keyFile);

    std::string GetType() const;
    void SetType(const std::string &type);
    std::string GetCipher() const;
    void SetCipher(const std::string &cipher);
    int32_t GetKeySize() const;
    void SetKeySize(int32_t keySize);
    std::string GetKeyFile() const;
    void SetKeyFile(const std::string &keyFile);

    bool Marshalling(Parcel &parcel) const override;
    static CryptParam *Unmarshalling(Parcel &parcel);

private:
    std::string type_;
    std::string cipher_;
    int32_t keySize_ {0};
    std::string keyFile_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_CRYPT_PARAM_H
