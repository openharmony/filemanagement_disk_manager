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

#include "external_volume_info.h"

namespace OHOS {
namespace DiskManager {

namespace {
constexpr size_t PARCEL_STRING_MAX_LEN = 4096;
} // namespace

bool ExternalVolumeInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(volumeId_)) {
        return false;
    }
    if (!parcel.WriteString(uuid_)) {
        return false;
    }
    if (!parcel.WriteString(diskId_)) {
        return false;
    }
    if (!parcel.WriteString(description_)) {
        return false;
    }
    if (!parcel.WriteInt32(state_)) {
        return false;
    }
    if (!parcel.WriteInt64(totalSize_)) {
        return false;
    }
    if (!parcel.WriteInt64(freeSize_)) {
        return false;
    }
    if (!parcel.WriteString(path_)) {
        return false;
    }
    if (!parcel.WriteString(fsType_)) {
        return false;
    }
    return true;
}

ExternalVolumeInfo *ExternalVolumeInfo::Unmarshalling(Parcel &parcel)
{
    ExternalVolumeInfo *obj = new (std::nothrow) ExternalVolumeInfo();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->volumeId_ = parcel.ReadString();
    if (obj->volumeId_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->uuid_ = parcel.ReadString();
    if (obj->uuid_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->diskId_ = parcel.ReadString();
    if (obj->diskId_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->description_ = parcel.ReadString();
    if (obj->description_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->state_ = parcel.ReadInt32();
    obj->totalSize_ = parcel.ReadInt64();
    obj->freeSize_ = parcel.ReadInt64();
    obj->path_ = parcel.ReadString();
    if (obj->path_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->fsType_ = parcel.ReadString();
    if (obj->fsType_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    return obj;
}

} // namespace DiskManager
} // namespace OHOS
