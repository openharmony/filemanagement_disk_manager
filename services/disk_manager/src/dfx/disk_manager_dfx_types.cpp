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

#include "disk_manager_dfx_types.h"

#include "nlohmann/json.hpp"

namespace OHOS {
namespace DiskManager {

VolumeReportInfo &VolumeReportInfo::WithVolumeId(const std::string &id)
{
    volumeId = id;
    return *this;
}

VolumeReportInfo &VolumeReportInfo::WithDiskId(const std::string &id)
{
    diskId = id;
    return *this;
}

VolumeReportInfo &VolumeReportInfo::WithDevPath(const std::string &path)
{
    devPath = path;
    return *this;
}

VolumeReportInfo &VolumeReportInfo::WithFsType(const std::string &type)
{
    fsType = type;
    return *this;
}

void VolumeReportInfo::MergeFrom(const VolumeReportInfo &other)
{
    if (!other.volumeId.empty()) {
        volumeId = other.volumeId;
    }
    if (!other.diskId.empty()) {
        diskId = other.diskId;
    }
    if (!other.devPath.empty()) {
        devPath = other.devPath;
    }
    if (!other.fsType.empty()) {
        fsType = other.fsType;
    }
    if (!other.fsUuid.empty()) {
        fsUuid = other.fsUuid;
    }
    if (!other.extra.empty()) {
        extra = extra.empty() ? other.extra : (extra + ";" + other.extra);
    }
}

std::string VolumeReportInfo::ToExtraData() const
{
    nlohmann::json js;
    if (!volumeId.empty()) {
        js["volumeId"] = volumeId;
    }
    if (!diskId.empty()) {
        js["diskId"] = diskId;
    }
    if (!devPath.empty()) {
        js["devPath"] = devPath;
    }
    if (!fsType.empty()) {
        js["fsType"] = fsType;
    }
    if (!fsUuid.empty()) {
        js["fsUuid"] = fsUuid;
    }
    if (!extra.empty()) {
        js["extra"] = extra;
    }
    return js.empty() ? "" : js.dump();
}

std::string DfxTruncate(const std::string &text, size_t maxLen)
{
    if (text.size() <= maxLen) {
        return text;
    }
    return text.substr(0, maxLen);
}
} // namespace DiskManager
} // namespace OHOS
