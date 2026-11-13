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

#include "local_disk_info.h"

namespace OHOS {
namespace DiskManager {

namespace {
constexpr size_t PARCEL_STRING_MAX_LEN = 4096;
} // namespace

bool LocalDiskInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(sysPath_)) {
        return false;
    }
    if (!parcel.WriteString(model_)) {
        return false;
    }
    if (!parcel.WriteString(description_)) {
        return false;
    }
    if (!parcel.WriteInt64(size_)) {
        return false;
    }
    if (!parcel.WriteInt32(mediaType_)) {
        return false;
    }
    if (!parcel.WriteString(state_)) {
        return false;
    }
    return true;
}

LocalDiskInfo *LocalDiskInfo::Unmarshalling(Parcel &parcel)
{
    LocalDiskInfo *obj = new (std::nothrow) LocalDiskInfo();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->sysPath_ = parcel.ReadString();
    if (obj->sysPath_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->model_ = parcel.ReadString();
    if (obj->model_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->description_ = parcel.ReadString();
    if (obj->description_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->size_ = parcel.ReadInt64();
    obj->mediaType_ = parcel.ReadInt32();
    obj->state_ = parcel.ReadString();
    if (obj->state_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    return obj;
}

} // namespace DiskManager
} // namespace OHOS
