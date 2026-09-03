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

#include "mount_param.h"

namespace OHOS {
namespace DiskManager {

MountParam::MountParam(bool readOnly) : readOnly_(readOnly) {}

bool MountParam::GetReadOnly() const
{
    return readOnly_;
}

void MountParam::SetReadOnly(bool readOnly)
{
    readOnly_ = readOnly;
}

bool MountParam::IsFromEdmMount() const
{
    return fromEdmMount_;
}
 
void MountParam::SetFromEdmMount(bool fromEdmMount)
{
    fromEdmMount_ = fromEdmMount;
}

bool MountParam::Marshalling(Parcel &parcel) const
{
    if (!parcel.WriteBool(readOnly_) || !parcel.WriteBool(fromEdmMount_)) {
        return false;
    }
    return true;
}

MountParam *MountParam::Unmarshalling(Parcel &parcel)
{
    MountParam *obj = new (std::nothrow) MountParam();
    if (obj == nullptr) {
        return nullptr;
    }
    obj->readOnly_ = parcel.ReadBool();
    obj->fromEdmMount_ = parcel.ReadBool();
    return obj;
}

} // namespace DiskManager
} // namespace OHOS
