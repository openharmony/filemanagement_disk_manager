/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "message_parcel_mock.h"

namespace OHOS {
using namespace OHOS::DiskManager;

Parcelable::Parcelable() : Parcelable(false) {}

Parcelable::Parcelable(bool asRemote)
{
    asRemote_ = asRemote;
    behavior_ = 0;
}

bool MessageParcel::WriteInterfaceToken(std::u16string name)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteInterfaceToken(name);
}

std::u16string MessageParcel::ReadInterfaceToken()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return std::u16string();
    }
    return MockMessageParcel::messageParcel->ReadInterfaceToken();
}

bool Parcel::WriteParcelable(const Parcelable *object)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteParcelable(object);
}

bool Parcel::WriteInt32(int32_t value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteInt32(value);
}

bool Parcel::WriteInt64(int64_t value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteInt64(value);
}

int32_t Parcel::ReadInt32()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return -1;
    }
    return MockMessageParcel::messageParcel->ReadInt32();
}

bool Parcel::ReadInt32(int32_t &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadInt32(value);
}

bool Parcel::ReadInt64(int64_t &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadInt64(value);
}

bool Parcel::WriteRemoteObject(const Parcelable *object)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteRemoteObject(object);
}

bool MessageParcel::WriteRemoteObject(const sptr<IRemoteObject> &object)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteRemoteObject(object);
}

sptr<IRemoteObject> MessageParcel::ReadRemoteObject()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return nullptr;
    }
    return MockMessageParcel::messageParcel->ReadRemoteObject();
}

bool Parcel::ReadBool()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadBool();
}

bool Parcel::ReadBool(bool &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadBool(value);
}

bool Parcel::WriteBool(bool value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteBool(value);
}

bool Parcel::WriteString(const std::string &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteString(value);
}

bool Parcel::WriteString16(const std::u16string &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteString16(value);
}

const std::u16string Parcel::ReadString16()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return std::u16string();
    }
    return MockMessageParcel::messageParcel->ReadString16();
}

bool Parcel::WriteCString(const char *value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteCString(value);
}

bool Parcel::ReadString(std::string &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadString(value);
}

bool Parcel::ReadStringVector(std::vector<std::string> *value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadStringVector(value);
}

bool MessageParcel::WriteFileDescriptor(int fd)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteFileDescriptor(fd);
}

int MessageParcel::ReadFileDescriptor()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return -1;
    }
    return MockMessageParcel::messageParcel->ReadFileDescriptor();
}

bool Parcel::ReadUint32(uint32_t &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadUint32(value);
}

bool Parcel::WriteUint64(uint64_t value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteUint64(value);
}

bool Parcel::WriteUint16(uint16_t value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteUint16(value);
}

bool Parcel::ReadUint64(uint64_t &value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->ReadUint64(value);
}

bool Parcel::WriteStringVector(const std::vector<std::string> &val)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteStringVector(val);
}

bool Parcel::WriteUint32(uint32_t value)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteUint32(value);
}

bool MessageParcel::WriteRawData(const void *data, size_t size)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return false;
    }
    return MockMessageParcel::messageParcel->WriteRawData(data, size);
}

const void *MessageParcel::ReadRawData(size_t size)
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return nullptr;
    }
    return MockMessageParcel::messageParcel->ReadRawData(size);
}

uint32_t Parcel::ReadUint32()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return 0;
    }
    return MockMessageParcel::messageParcel->ReadUint32();
}

uint16_t Parcel::ReadUint16()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return 0;
    }
    return static_cast<uint16_t>(MockMessageParcel::messageParcel->ReadUint16());
}

const char *Parcel::ReadCString()
{
    if (MockMessageParcel::messageParcel == nullptr) {
        return nullptr;
    }
    return MockMessageParcel::messageParcel->ReadCString();
}
} // namespace OHOS
