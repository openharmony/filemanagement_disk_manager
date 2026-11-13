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

#ifndef OHOS_DISK_MANAGER_LOCAL_DISK_INFO_H
#define OHOS_DISK_MANAGER_LOCAL_DISK_INFO_H

#include "parcel.h"

#include <cstdint>
#include <string>

namespace OHOS {
namespace DiskManager {

/**
 * @brief 本地磁盘硬件信息（Public API，三方应用可用）。
 *
 * 包含磁盘基础硬件属性，不包含序列号等敏感信息（通过独立方法查询）。
 */
class LocalDiskInfo : public Parcelable {
public:
    LocalDiskInfo() = default;
    ~LocalDiskInfo() override = default;

    std::string GetSysPath() const { return sysPath_; }
    void SetSysPath(const std::string &sysPath) { sysPath_ = sysPath; }

    std::string GetModel() const { return model_; }
    void SetModel(const std::string &model) { model_ = model; }

    std::string GetDescription() const { return description_; }
    void SetDescription(const std::string &description) { description_ = description; }

    int64_t GetSize() const { return size_; }
    void SetSize(int64_t size) { size_ = size; }

    int32_t GetMediaType() const { return mediaType_; }
    void SetMediaType(int32_t mediaType) { mediaType_ = mediaType; }

    std::string GetState() const { return state_; }
    void SetState(const std::string &state) { state_ = state; }

    bool Marshalling(Parcel &parcel) const override;
    static LocalDiskInfo *Unmarshalling(Parcel &parcel);

private:
    std::string sysPath_;
    std::string model_;
    std::string description_;
    int64_t size_ = 0;
    int32_t mediaType_ = 0;
    std::string state_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_LOCAL_DISK_INFO_H
