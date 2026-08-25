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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_DFX_TYPES_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_DFX_TYPES_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace DiskManager {

constexpr int32_t DFX_STAGE_MOUNT = 41;
constexpr int32_t DFX_STAGE_UNMOUNT = 42;
constexpr int32_t DFX_STAGE_FORMAT = 43;
constexpr int32_t DFX_STAGE_SET_VOLUME_DESCRIPTION = 44;
constexpr int32_t DFX_STAGE_UEVENT_PARSE = 45;
constexpr int32_t DFX_STAGE_GET_PARTITION_TABLE = 46;
constexpr int32_t DFX_STAGE_CREATE_PARTITION = 47;
constexpr int32_t DFX_STAGE_DELETE_PARTITION = 48;
constexpr int32_t DFX_STAGE_FORMAT_PARTITION = 49;
constexpr int32_t DFX_STAGE_BIND_BLOCK_LOOP_DEV = 50;
constexpr int32_t DFX_STAGE_CREATE_DM_CRYPT_VOLUME = 51;
constexpr int32_t DFX_STAGE_DESTROY_DM_CRYPT_VOLUME = 52;
constexpr int32_t DFX_STAGE_UNBIND_BLOCK_LOOP_DEV = 53;
// Must fit tools[] dump from volume op diag (cmd/ret/exitCode/output).
constexpr size_t DFX_TRUNCATE_MAX_LEN = 2048;

enum class VolumeOpType : int32_t {
    MOUNT = 0,
    UNMOUNT = 1,
    FORMAT = 2,
    SET_VOLUME_DESCRIPTION = 3,
    OTHER = 4,
    GET_PARTITION_TABLE = 5,
    CREATE_PARTITION = 6,
    DELETE_PARTITION = 7,
    FORMAT_PARTITION = 8,
    BIND_BLOCK_LOOP_DEV = 9,
    CREATE_DM_CRYPT_VOLUME = 10,
    DESTROY_DM_CRYPT_VOLUME = 11,
    UNBIND_BLOCK_LOOP_DEV = 12,
};

enum class AutoMountSkipReason : int32_t {
    AUTO_MOUNT_DISABLED = 0,
    MISSING_FS_TYPE,
    MISSING_UUID,
    MISSING_FS_TYPE_AND_UUID,
};

AutoMountSkipReason ResolveAutoMountSkipReason(bool autoMountEnabled, const std::string &type,
                                               const std::string &uuid);
const char *AutoMountSkipReasonToString(AutoMountSkipReason reason);

struct AutoMountSkipContext {
    std::string volId;
    std::string diskId;
    std::string volDevPath;
    std::string type;
    std::string uuid;
    bool autoMountEnabled = true;
};

struct VolumeReportInfo {
    std::string volumeId;
    std::string diskId;
    std::string devPath;
    std::string fsType;
    std::string fsUuid;
    std::string extra;

    VolumeReportInfo &WithVolumeId(const std::string &id);
    VolumeReportInfo &WithDiskId(const std::string &id);
    VolumeReportInfo &WithDevPath(const std::string &path);
    VolumeReportInfo &WithFsType(const std::string &type);
    void MergeFrom(const VolumeReportInfo &other);
    std::string ToExtraData() const;
};

VolumeReportInfo BuildAutoMountSkipReportInfo(const AutoMountSkipContext &ctx);

struct OpDiagReport {
    bool valid = false;
    int32_t ret = 0;
    std::string funcName;
    int32_t bizStage = 0;
    VolumeOpType opType = VolumeOpType::OTHER;
    VolumeReportInfo info;
};

std::string DfxTruncate(const std::string &text, size_t maxLen = DFX_TRUNCATE_MAX_LEN);
VolumeReportInfo ParseOpDiagText(const std::string &opDiag);
OpDiagReport ParseOpDiagReport(const std::string &opDiag);

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_DFX_TYPES_H
