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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_RADAR_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_RADAR_H

#include <cstdint>
#include <string>

#include "disk_manager_dfx_types.h"

namespace OHOS {
namespace DiskManager {

constexpr const char *FILE_BACKUP_EVENTS = "FILE_BACKUP_EVENTS";
constexpr const char *FILE_STORAGE_FAULT = "FILE_STORAGE_FAULT";
constexpr const char *DEFAULT_ORG_PKG = "diskManager";
constexpr const char *DEFAULT_TO_CALL_PKG = "storage_daemon";
constexpr const char *DEFAULT_KEY_ELX_LEVEL = "NA";
constexpr int32_t DEFAULT_USER_ID = 100;

enum class BizScene : int32_t {
    EXTERNAL_VOLUME_MANAGER = 4,
};

struct RadarParameter {
    std::string orgPkg = DEFAULT_ORG_PKG;
    int32_t userId = DEFAULT_USER_ID;
    std::string funcName;
    BizScene bizScene = BizScene::EXTERNAL_VOLUME_MANAGER;
    int32_t bizStage = 0;
    std::string keyElxLevel = DEFAULT_KEY_ELX_LEVEL;
    std::string toCallPkg = DEFAULT_TO_CALL_PKG;
    std::string fileStatus;
    int32_t errorCode = 0;
};

class DiskManagerRadar {
public:
    static DiskManagerRadar &GetInstance();

    // Behavior event FILE_BACKUP_EVENTS; audit log is written automatically after report.
    void ReportBehavior(const std::string &funcName, int32_t stage, const std::string &status,
                        const VolumeReportInfo &info, int32_t ret = 0);

    // Fault event FILE_STORAGE_FAULT; audit log is written automatically after report.
    // Capability only for now: do not wire into business/adapter until enabled later.
    bool RecordFault(const RadarParameter &param);
    void ReportVolumeFault(const std::string &funcName, int32_t stage, VolumeOpType opType, int32_t err,
                           const VolumeReportInfo &info);
    void ReportMetadataFault(const std::string &funcName, int32_t err, const VolumeReportInfo &info);

private:
    DiskManagerRadar() = default;
    ~DiskManagerRadar() = default;
    DiskManagerRadar(const DiskManagerRadar &) = delete;
    DiskManagerRadar &operator=(const DiskManagerRadar &) = delete;

    void WriteBehaviorEvent(const std::string &funcName, const std::string &bundleName, int32_t pid,
                            const std::string &time);
    void WriteAuditForBehavior(const std::string &funcName, int32_t stage, const std::string &status,
                               const VolumeReportInfo &info, int32_t ret);
    void WriteAuditForFault(const RadarParameter &param);
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_RADAR_H
