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
#ifndef OHOS_DISK_MANAGER_MESSAGE_PARCEL_MOCK_H
#define OHOS_DISK_MANAGER_MESSAGE_PARCEL_MOCK_H

#include <gmock/gmock.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "iremote_broker.h"
#include "message_parcel.h"

namespace OHOS::DiskManager {
class MockMessageParcel {
public:
    virtual ~MockMessageParcel() = default;
    virtual bool WriteInterfaceToken(std::u16string name) = 0;
    virtual std::u16string ReadInterfaceToken() = 0;
    virtual bool WriteParcelable(const Parcelable *object) = 0;
    virtual bool WriteInt32(int32_t value) = 0;
    virtual bool WriteInt64(int64_t value) = 0;
    virtual int32_t ReadInt32() = 0;
    virtual bool ReadInt32(int32_t &value) = 0;
    virtual bool ReadInt64(int64_t &value) = 0;
    virtual bool WriteRemoteObject(const Parcelable *object) = 0;
    virtual bool WriteRemoteObject(const sptr<IRemoteObject> &object) = 0;
    virtual sptr<IRemoteObject> ReadRemoteObject() = 0;
    virtual bool ReadBool() = 0;
    virtual bool ReadBool(bool &value) = 0;
    virtual bool WriteBool(bool value) = 0;
    virtual bool WriteString(const std::string &value) = 0;
    virtual bool WriteString16(const std::u16string &value) = 0;
    virtual const std::u16string ReadString16() = 0;
    virtual bool WriteCString(const char *value) = 0;
    virtual bool WriteFileDescriptor(int fd) = 0;
    virtual bool ReadString(std::string &value) = 0;
    virtual int ReadFileDescriptor() = 0;
    virtual bool ReadStringVector(std::vector<std::string> *value) = 0;
    virtual bool ReadUint32(uint32_t &value) = 0;
    virtual bool WriteUint64(uint64_t value) = 0;
    virtual bool WriteUint16(uint16_t value) = 0;
    virtual bool ReadUint64(uint64_t &value) = 0;
    virtual bool WriteStringVector(const std::vector<std::string> &val) = 0;
    virtual bool WriteUint32(uint32_t value) = 0;
    virtual bool WriteRawData(const void *data, size_t size) = 0;
    virtual const void *ReadRawData(size_t size) = 0;
    virtual uint32_t ReadUint32() = 0;
    virtual uint32_t ReadUint16() = 0;
    virtual const char *ReadCString() = 0;

    static inline std::shared_ptr<MockMessageParcel> messageParcel = nullptr;
};

class MessageParcelMock : public MockMessageParcel {
public:
    MOCK_METHOD(bool, WriteInterfaceToken, (std::u16string name), (override));
    MOCK_METHOD(std::u16string, ReadInterfaceToken, (), (override));
    MOCK_METHOD(bool, WriteParcelable, (const Parcelable *object), (override));
    MOCK_METHOD(bool, WriteInt32, (int32_t value), (override));
    MOCK_METHOD(bool, WriteInt64, (int64_t value), (override));
    MOCK_METHOD(int32_t, ReadInt32, (), (override));
    MOCK_METHOD(bool, ReadInt32, (int32_t & value), (override));
    MOCK_METHOD(bool, ReadInt64, (int64_t & value), (override));
    MOCK_METHOD(bool, WriteRemoteObject, (const Parcelable *object), (override));
    MOCK_METHOD(bool, WriteRemoteObject, (const sptr<IRemoteObject> &object), (override));
    MOCK_METHOD(sptr<IRemoteObject>, ReadRemoteObject, (), (override));
    MOCK_METHOD(bool, ReadBool, (), (override));
    MOCK_METHOD(bool, ReadBool, (bool &value), (override));
    MOCK_METHOD(bool, WriteBool, (bool value), (override));
    MOCK_METHOD(bool, WriteString, (const std::string &value), (override));
    MOCK_METHOD(bool, WriteString16, (const std::u16string &value), (override));
    MOCK_METHOD(const std::u16string, ReadString16, (), (override));
    MOCK_METHOD(bool, WriteCString, (const char *value), (override));
    MOCK_METHOD(bool, WriteFileDescriptor, (int fd), (override));
    MOCK_METHOD(bool, ReadString, (std::string & value), (override));
    MOCK_METHOD(int, ReadFileDescriptor, (), (override));
    MOCK_METHOD(bool, ReadStringVector, (std::vector<std::string> * value), (override));
    MOCK_METHOD(bool, ReadUint32, (uint32_t & value), (override));
    MOCK_METHOD(bool, WriteUint64, (uint64_t value), (override));
    MOCK_METHOD(bool, WriteUint16, (uint16_t value), (override));
    MOCK_METHOD(bool, ReadUint64, (uint64_t & value), (override));
    MOCK_METHOD(bool, WriteStringVector, (const std::vector<std::string> &val), (override));
    MOCK_METHOD(bool, WriteUint32, (uint32_t value), (override));
    MOCK_METHOD(bool, WriteRawData, (const void *data, size_t size), (override));
    MOCK_METHOD(const void *, ReadRawData, (size_t size), (override));
    MOCK_METHOD(uint32_t, ReadUint32, (), (override));
    MOCK_METHOD(uint32_t, ReadUint16, (), (override));
    MOCK_METHOD(const char *, ReadCString, (), (override));
};
} // namespace OHOS::DiskManager

#endif // OHOS_DISK_MANAGER_MESSAGE_PARCEL_MOCK_H
