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

#include "disk_manager_radar.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <unistd.h>

#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"
#include "hi_audit.h"
#include "hisysevent.h"

namespace OHOS {
namespace DiskManager {
namespace {
constexpr const char *DISK_MANAGER_DOMAIN = "FILEMANAGEMENT";
constexpr int32_t BEHAVIOR_PARAMS_LEN = 4;
constexpr int32_t FAULT_PARAMS_LEN = 9;
constexpr uint8_t MS_WIDTH = 3;
constexpr uint32_t MS_1000 = 1000;

std::string GetCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    std::stringstream strTime;
    struct tm result {};
    if (localtime_r(&time, &result) != nullptr) {
        strTime << std::put_time(&result, "%Y-%m-%d %H:%M:%S:") << std::setfill('0') << std::setw(MS_WIDTH)
                << (ms.count() % MS_1000);
    }
    return strTime.str();
}
} // namespace

DiskManagerRadar &DiskManagerRadar::GetInstance()
{
    static DiskManagerRadar instance;
    return instance;
}

void DiskManagerRadar::WriteBehaviorEvent(const std::string &funcName, const std::string &bundleName, int32_t pid,
                                          const std::string &time)
{
    HiSysEventParam params[BEHAVIOR_PARAMS_LEN] = {
        {.name = "PROC_NAME",
         .t = HISYSEVENT_STRING,
         .v = {.s = const_cast<char *>(funcName.c_str())},
         .arraySize = 0},
        {.name = "BUNDLENAME",
         .t = HISYSEVENT_STRING,
         .v = {.s = const_cast<char *>(bundleName.c_str())},
         .arraySize = 0},
        {.name = "PID", .t = HISYSEVENT_INT32, .v = {.i32 = pid}, .arraySize = 0},
        {.name = "TIME", .t = HISYSEVENT_STRING, .v = {.s = const_cast<char *>(time.c_str())}, .arraySize = 0},
    };
    int32_t res =
        OH_HiSysEvent_Write(DISK_MANAGER_DOMAIN, FILE_BACKUP_EVENTS, HISYSEVENT_BEHAVIOR, params, BEHAVIOR_PARAMS_LEN);
    if (res != E_OK) {
        LOGE("DiskManagerRadar behavior write failed res=%{public}d func=%{public}s", res, funcName.c_str());
    }
}

void DiskManagerRadar::WriteAuditForBehavior(const std::string &funcName, int32_t stage, const std::string &status,
                                             const VolumeReportInfo &info, int32_t ret)
{
    AuditLog auditLog;
    auditLog.isUserBehavior = false;
    auditLog.operationCount = 1;
    auditLog.cause = DEFAULT_ORG_PKG;
    auditLog.operationType = funcName;
    auditLog.operationScenario = "bizStage:" + std::to_string(stage);
    auditLog.operationStatus = status;
    auditLog.extend = "userId:" + std::to_string(DEFAULT_USER_ID) + ",ret:" + std::to_string(ret) +
                      ",extra:" + info.ToExtraData();
    HiAudit::GetInstance().Write(auditLog);
}

void DiskManagerRadar::ReportBehavior(const std::string &funcName, int32_t stage, const std::string &status,
                                      const VolumeReportInfo &info, int32_t ret)
{
    const std::string extra = info.ToExtraData();
    std::string time = GetCurrentTime() + "|" + status;
    if (!extra.empty()) {
        time += "|" + extra;
    }
    WriteBehaviorEvent(funcName, DEFAULT_ORG_PKG, static_cast<int32_t>(getpid()), time);
    WriteAuditForBehavior(funcName, stage, status, info, ret);
}

void DiskManagerRadar::WriteAuditForFault(const RadarParameter &param)
{
    AuditLog auditLog;
    auditLog.isUserBehavior = false;
    auditLog.operationCount = 1;
    auditLog.cause = param.orgPkg;
    auditLog.operationType = param.funcName;
    auditLog.operationScenario = "bizScene:" + std::to_string(static_cast<int32_t>(param.bizScene)) +
                                 ",bizStage:" + std::to_string(param.bizStage);
    auditLog.operationStatus = "fail";
    auditLog.extend = "userId:" + std::to_string(param.userId) + ",keyElxLevel:" + param.keyElxLevel +
                      ",toCallPkg:" + param.toCallPkg + ",fileStatus:" + param.fileStatus +
                      ",errorCode:" + std::to_string(param.errorCode);
    HiAudit::GetInstance().Write(auditLog);
}

bool DiskManagerRadar::RecordFault(const RadarParameter &param)
{
    HiSysEventParam params[FAULT_PARAMS_LEN] = {
        {.name = "ORG_PKG", .t = HISYSEVENT_STRING, .v = {.s = const_cast<char *>(param.orgPkg.c_str())},
         .arraySize = 0},
        {.name = "USER_ID", .t = HISYSEVENT_INT32, .v = {.i32 = param.userId}, .arraySize = 0},
        {.name = "FUNC", .t = HISYSEVENT_STRING, .v = {.s = const_cast<char *>(param.funcName.c_str())},
         .arraySize = 0},
        {.name = "BIZ_SCENE", .t = HISYSEVENT_INT32, .v = {.i32 = static_cast<int32_t>(param.bizScene)},
         .arraySize = 0},
        {.name = "BIZ_STAGE", .t = HISYSEVENT_INT32, .v = {.i32 = param.bizStage}, .arraySize = 0},
        {.name = "KEY_ELX_LEVEL", .t = HISYSEVENT_STRING, .v = {.s = const_cast<char *>(param.keyElxLevel.c_str())},
         .arraySize = 0},
        {.name = "TO_CALL_PKG", .t = HISYSEVENT_STRING, .v = {.s = const_cast<char *>(param.toCallPkg.c_str())},
         .arraySize = 0},
        {.name = "FILE_STATUS", .t = HISYSEVENT_STRING, .v = {.s = const_cast<char *>(param.fileStatus.c_str())},
         .arraySize = 0},
        {.name = "ERROR_CODE", .t = HISYSEVENT_INT32, .v = {.i32 = param.errorCode}, .arraySize = 0},
    };
    int32_t res =
        OH_HiSysEvent_Write(DISK_MANAGER_DOMAIN, FILE_STORAGE_FAULT, HISYSEVENT_FAULT, params, FAULT_PARAMS_LEN);
    WriteAuditForFault(param);
    if (res != E_OK) {
        LOGE("DiskManagerRadar fault write failed res=%{public}d func=%{public}s err=%{public}d", res,
             param.funcName.c_str(), param.errorCode);
        return false;
    }
    return true;
}

void DiskManagerRadar::ReportVolumeFault(const std::string &funcName, int32_t stage, VolumeOpType opType, int32_t err,
                                         const VolumeReportInfo &info)
{
    (void)opType;
    RadarParameter param;
    param.funcName = funcName;
    param.bizStage = stage;
    param.errorCode = err;
    param.fileStatus = info.ToExtraData();
    RecordFault(param);
}

void DiskManagerRadar::ReportMetadataFault(const std::string &funcName, int32_t err, const VolumeReportInfo &info)
{
    ReportVolumeFault(funcName, DFX_STAGE_MOUNT, VolumeOpType::OTHER, err, info);
}

void DiskManagerRadar::ReportUeventParseFault(const VolumeReportInfo &info)
{
    ReportVolumeFault("UeventBootstrap::OnBlockDiskUevent", DFX_STAGE_UEVENT_PARSE, VolumeOpType::OTHER,
                      DiskManagerErrNo::E_UEVENT_PARSE_FAILED, info);
}

void DiskManagerRadar::ReportAutoMountMetadataFault(const VolumeReportInfo &info, AutoMountSkipReason reason)
{
    VolumeReportInfo report = info;
    const bool typeEmpty = report.fsType.empty();
    const bool uuidEmpty = report.fsUuid.empty();
    report.extra = std::string("reason=") + AutoMountSkipReasonToString(reason) + ",typeEmpty=" +
                   std::to_string(typeEmpty) + ",uuidEmpty=" + std::to_string(uuidEmpty);
    ReportVolumeFault("UeventBootstrap::DiscoverSinglePartitionVolume", DFX_STAGE_MOUNT, VolumeOpType::MOUNT,
                      DiskManagerErrNo::E_PARAMS_INVALID, report);
}

void DiskManagerRadar::ReportDiscoverAutoMountSkipFault(const AutoMountSkipContext &ctx)
{
    LOGE("DiscoverSinglePartitionVolume skip auto mount volId=%{public}s diskId=%{public}s "
         "devPath=%{public}s typeEmpty=%{public}d uuidEmpty=%{public}d autoMount=%{public}d",
         ctx.volId.c_str(), ctx.diskId.c_str(), ctx.volDevPath.c_str(), ctx.type.empty() ? 1 : 0,
         ctx.uuid.empty() ? 1 : 0, ctx.autoMountEnabled ? 1 : 0);
    const AutoMountSkipReason reason = ResolveAutoMountSkipReason(ctx.autoMountEnabled, ctx.type, ctx.uuid);
    ReportAutoMountMetadataFault(BuildAutoMountSkipReportInfo(ctx), reason);
}
} // namespace DiskManager
} // namespace OHOS
