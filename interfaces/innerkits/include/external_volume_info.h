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

#ifndef OHOS_DISK_MANAGER_EXTERNAL_VOLUME_INFO_H
#define OHOS_DISK_MANAGER_EXTERNAL_VOLUME_INFO_H

#include "parcel.h"

#include <cstdint>
#include <string>

namespace OHOS {
namespace DiskManager {

/**
 * @brief 外置卷基础属性信息（Public API，三方应用可用）。
 *
 * 仅包含三方应用可见的非敏感字段，totalSize/freeSize 仅在 MOUNTED 状态有效。
 */
class ExternalVolumeInfo : public Parcelable {
public:
    ExternalVolumeInfo() = default;
    ~ExternalVolumeInfo() override = default;

    std::string GetVolumeId() const { return volumeId_; }
    void SetVolumeId(const std::string &volumeId) { volumeId_ = volumeId; }

    std::string GetUuid() const { return uuid_; }
    void SetUuid(const std::string &uuid) { uuid_ = uuid; }

    std::string GetDiskId() const { return diskId_; }
    void SetDiskId(const std::string &diskId) { diskId_ = diskId; }

    std::string GetDescription() const { return description_; }
    void SetDescription(const std::string &description) { description_ = description; }

    int32_t GetState() const { return state_; }
    void SetState(int32_t state) { state_ = state; }

    int64_t GetTotalSize() const { return totalSize_; }
    void SetTotalSize(int64_t totalSize) { totalSize_ = totalSize; }

    int64_t GetFreeSize() const { return freeSize_; }
    void SetFreeSize(int64_t freeSize) { freeSize_ = freeSize; }

    std::string GetPath() const { return path_; }
    void SetPath(const std::string &path) { path_ = path; }

    std::string GetFsType() const { return fsType_; }
    void SetFsType(const std::string &fsType) { fsType_ = fsType; }

    bool Marshalling(Parcel &parcel) const override;
    static ExternalVolumeInfo *Unmarshalling(Parcel &parcel);

private:
    std::string volumeId_;
    std::string uuid_;
    std::string diskId_;
    std::string description_;
    int32_t state_ = 0;
    int64_t totalSize_ = 0;
    int64_t freeSize_ = 0;
    std::string path_;
    std::string fsType_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_EXTERNAL_VOLUME_INFO_H
