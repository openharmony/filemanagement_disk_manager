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

#ifndef OHOS_DISK_MANAGER_EXTERNAL_DISK_INFO_H
#define OHOS_DISK_MANAGER_EXTERNAL_DISK_INFO_H

#include "parcel.h"

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {

/**
 * @brief 外置磁盘基础属性信息（Public API，三方应用可用）。
 *
 * 仅包含三方应用可见的非敏感字段，不包含 sysPath、devName 等内部字段。
 */
class ExternalDiskInfo : public Parcelable {
public:
    ExternalDiskInfo() = default;
    ~ExternalDiskInfo() override = default;

    std::string GetDiskId() const { return diskId_; }
    void SetDiskId(const std::string &diskId) { diskId_ = diskId; }

    int32_t GetDiskType() const { return diskType_; }
    void SetDiskType(int32_t diskType) { diskType_ = diskType; }

    const std::vector<std::string> &GetVolumeIds() const { return volumeIds_; }
    void SetVolumeIds(std::vector<std::string> volumeIds) { volumeIds_ = std::move(volumeIds); }

    int32_t GetVendorId() const { return vendorId_; }
    void SetVendorId(int32_t vendorId) { vendorId_ = vendorId; }

    int32_t GetProductId() const { return productId_; }
    void SetProductId(int32_t productId) { productId_ = productId; }

    bool Marshalling(Parcel &parcel) const override;
    static ExternalDiskInfo *Unmarshalling(Parcel &parcel);

private:
    std::string diskId_;
    int32_t diskType_ = 0;
    std::vector<std::string> volumeIds_;
    int32_t vendorId_ = 0;
    int32_t productId_ = 0;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_EXTERNAL_DISK_INFO_H
