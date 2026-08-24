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

#include "crypt_param.h"

namespace OHOS {
namespace DiskManager {
namespace {
constexpr size_t PARCEL_STRING_MAX_LEN = 4096;
}

CryptParam::CryptParam(const std::string &type, const std::string &cipher,
    int32_t keySize, const std::string &keyFile) : type_(type), cipher_(cipher),
    keySize_(keySize), keyFile_(keyFile) {}

std::string CryptParam::GetType() const
{
    return type_;
}

void CryptParam::SetType(const std::string &type)
{
    type_ = type;
}

std::string CryptParam::GetCipher() const
{
    return cipher_;
}

void CryptParam::SetCipher(const std::string &cipher)
{
    cipher_ = cipher;
}

int32_t CryptParam::GetKeySize() const
{
    return keySize_;
}

void CryptParam::SetKeySize(int32_t keySize)
{
    keySize_ = keySize;
}

std::string CryptParam::GetKeyFile() const
{
    return keyFile_;
}

void CryptParam::SetKeyFile(const std::string &keyFile)
{
    keyFile_ = keyFile;
}

bool CryptParam::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(type_)) {
        return false;
    }
    if (!parcel.WriteString(cipher_)) {
        return false;
    }
    if (!parcel.WriteInt32(keySize_)) {
        return false;
    }
    if (!parcel.WriteString(keyFile_)) {
        return false;
    }
    return true;
}

CryptParam *CryptParam::Unmarshalling(Parcel &parcel)
{
    CryptParam *obj = new (std::nothrow) CryptParam();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->type_ = parcel.ReadString();
    if (obj->type_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->cipher_ = parcel.ReadString();
    if (obj->cipher_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->keySize_ = parcel.ReadInt32();
    obj->keyFile_ = parcel.ReadString();
    if (obj->keyFile_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    return obj;
}

} // namespace DiskManager
} // namespace OHOS
