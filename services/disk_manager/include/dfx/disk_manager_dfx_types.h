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
constexpr size_t DFX_TRUNCATE_MAX_LEN = 256;

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

std::string DfxTruncate(const std::string &text, size_t maxLen = DFX_TRUNCATE_MAX_LEN);

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_DFX_TYPES_H
