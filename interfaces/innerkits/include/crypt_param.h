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

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>

namespace OHOS {
namespace DiskManager {
using json = nlohmann::json;

class CryptParam : public Parcelable {
public:
    CryptParam() = default;
    CryptParam(const std::string &passPhrase, const std::string &type, const std::string &cipher,
               int32_t keySize, const std::string &hash);

    std::string GetPassPhrase() const;
    void SetPassPhrase(const std::string &passPhrase);
    std::string GetType() const;
    void SetType(const std::string &type);
    std::string GetCipher() const;
    void SetCipher(const std::string &cipher);
    int32_t GetKeySize() const;
    void SetKeySize(int32_t keySize);
    std::string GetHash() const;
    void SetHash(const std::string &hash);

    bool Marshalling(Parcel &parcel) const override;
    static CryptParam *Unmarshalling(Parcel &parcel);

    std::string Serialize() const
    {
        return json{{"passPhrase", passPhrase_},
                    {"type", type_},
                    {"cipher", cipher_},
                    {"keySize", keySize_},
                    {"hash", hash_}}.dump();
    }

    static CryptParam Deserialize(const std::string &text)
    {
        CryptParam param;
        nlohmann::json j = nlohmann::json::parse(text, nullptr, false);
        if (!j.is_object()) {
            return param;
        }
        param.passPhrase_ = j.value("passPhrase", std::string{});
        param.type_ = j.value("type", std::string{});
        param.cipher_ = j.value("cipher", std::string{});
        param.keySize_ = j.value("keySize", 0);
        param.hash_ = j.value("hash", std::string{});
        return param;
    }

private:
    std::string passPhrase_;
    std::string type_;
    std::string cipher_;
    int32_t keySize_ {0};
    std::string hash_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_CRYPT_PARAM_H
