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

#include "external_disk_info.h"

namespace OHOS {
namespace DiskManager {

namespace {
constexpr uint32_t EXTERNAL_DISK_VOLUME_IDS_PARCEL_MAX = 256U;
constexpr size_t PARCEL_STRING_MAX_LEN = 4096;
} // namespace

bool ExternalDiskInfo::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteString(diskId_)) {
        return false;
    }
    if (!parcel.WriteInt32(diskType_)) {
        return false;
    }
    const uint32_t nVolumes = static_cast<uint32_t>(volumeIds_.size());
    if (!parcel.WriteUint32(nVolumes)) {
        return false;
    }
    for (const auto &vid : volumeIds_) {
        if (!parcel.WriteString(vid)) {
            return false;
        }
    }
    if (!parcel.WriteInt32(vendorId_)) {
        return false;
    }
    if (!parcel.WriteInt32(productId_)) {
        return false;
    }
    return true;
}

ExternalDiskInfo *ExternalDiskInfo::Unmarshalling(Parcel &parcel)
{
    ExternalDiskInfo *obj = new (std::nothrow) ExternalDiskInfo();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->diskId_ = parcel.ReadString();
    if (obj->diskId_.size() > PARCEL_STRING_MAX_LEN) {
        delete obj;
        return nullptr;
    }
    obj->diskType_ = parcel.ReadInt32();
    const uint32_t nVolumes = parcel.ReadUint32();
    if (nVolumes > EXTERNAL_DISK_VOLUME_IDS_PARCEL_MAX) {
        delete obj;
        return nullptr;
    }
    obj->volumeIds_.clear();
    obj->volumeIds_.reserve(nVolumes);
    for (uint32_t i = 0; i < nVolumes; ++i) {
        std::string vid = parcel.ReadString();
        if (vid.size() > PARCEL_STRING_MAX_LEN) {
            delete obj;
            return nullptr;
        }
        obj->volumeIds_.push_back(std::move(vid));
    }
    obj->vendorId_ = parcel.ReadInt32();
    obj->productId_ = parcel.ReadInt32();
    return obj;
}

} // namespace DiskManager
} // namespace OHOS
