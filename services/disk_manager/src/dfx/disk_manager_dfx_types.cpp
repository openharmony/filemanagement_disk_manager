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

static void FillReportInfoFromJson(const nlohmann::json &js, VolumeReportInfo &info)
{
    if (js.contains("devPath") && js["devPath"].is_string()) {
        info.devPath = js["devPath"].get<std::string>();
    }
    if (js.contains("fsType") && js["fsType"].is_string()) {
        info.fsType = js["fsType"].get<std::string>();
    }
    if (js.contains("volumeId") && js["volumeId"].is_string()) {
        info.volumeId = js["volumeId"].get<std::string>();
    }
    if (js.contains("diskId") && js["diskId"].is_string()) {
        info.diskId = js["diskId"].get<std::string>();
    }
    if (js.contains("extra") && js["extra"].is_string()) {
        info.extra = js["extra"].get<std::string>();
    } else if (js.contains("tools")) {
        info.extra = DfxTruncate(js["tools"].dump());
    }
}

AutoMountSkipReason ResolveAutoMountSkipReason(bool autoMountEnabled, const std::string &type,
                                               const std::string &uuid)
{
    if (!autoMountEnabled) {
        return AutoMountSkipReason::AUTO_MOUNT_DISABLED;
    }
    const bool typeEmpty = type.empty();
    const bool uuidEmpty = uuid.empty();
    if (typeEmpty && uuidEmpty) {
        return AutoMountSkipReason::MISSING_FS_TYPE_AND_UUID;
    }
    if (typeEmpty) {
        return AutoMountSkipReason::MISSING_FS_TYPE;
    }
    return AutoMountSkipReason::MISSING_UUID;
}

const char *AutoMountSkipReasonToString(AutoMountSkipReason reason)
{
    switch (reason) {
        case AutoMountSkipReason::AUTO_MOUNT_DISABLED:
            return "autoMountDisabled";
        case AutoMountSkipReason::MISSING_FS_TYPE:
            return "missingFsType";
        case AutoMountSkipReason::MISSING_UUID:
            return "missingUuid";
        case AutoMountSkipReason::MISSING_FS_TYPE_AND_UUID:
            return "missingFsTypeAndUuid";
        default:
            return "unknown";
    }
}

VolumeReportInfo BuildAutoMountSkipReportInfo(const AutoMountSkipContext &ctx)
{
    VolumeReportInfo info;
    info.WithVolumeId(ctx.volId).WithDiskId(ctx.diskId).WithDevPath(ctx.volDevPath);
    if (!ctx.type.empty()) {
        info.WithFsType(ctx.type);
    }
    if (!ctx.uuid.empty()) {
        info.fsUuid = ctx.uuid;
    }
    return info;
}

VolumeReportInfo ParseOpDiagText(const std::string &opDiag)
{
    VolumeReportInfo info;
    if (opDiag.empty()) {
        return info;
    }
    nlohmann::json js = nlohmann::json::parse(opDiag, nullptr, false);
    if (js.is_discarded() || !js.is_object()) {
        info.extra = DfxTruncate(opDiag);
        return info;
    }
    FillReportInfoFromJson(js, info);
    return info;
}

OpDiagReport ParseOpDiagReport(const std::string &opDiag)
{
    OpDiagReport report;
    if (opDiag.empty()) {
        return report;
    }
    nlohmann::json js = nlohmann::json::parse(opDiag, nullptr, false);
    if (js.is_discarded() || !js.is_object()) {
        return report;
    }
    if (!js.contains("ret") || !js["ret"].is_number_integer()) {
        return report;
    }
    if (!js.contains("funcName") || !js["funcName"].is_string()) {
        return report;
    }
    report.ret = js["ret"].get<int32_t>();
    report.funcName = js["funcName"].get<std::string>();
    if (js.contains("bizStage") && js["bizStage"].is_number_integer()) {
        report.bizStage = js["bizStage"].get<int32_t>();
    }
    if (js.contains("opType") && js["opType"].is_number_integer()) {
        report.opType = static_cast<VolumeOpType>(js["opType"].get<int32_t>());
    }
    FillReportInfoFromJson(js, report.info);
    report.valid = true;
    return report;
}
} // namespace DiskManager
} // namespace OHOS
