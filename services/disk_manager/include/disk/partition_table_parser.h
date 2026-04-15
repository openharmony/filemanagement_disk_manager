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

#ifndef OHOS_DISK_MANAGER_DISK_PARTITION_TABLE_PARSER_H
#define OHOS_DISK_MANAGER_DISK_PARTITION_TABLE_PARSER_H

#include "disk/storage_spec_models.h"

#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {

class PartitionTableParser {
public:
    /**
     * 解析 storage_daemon ReadPartitionTable 返回的原始文本（与 sgdisk --ohos-dump 风格兼容）。
     * 输出填充 PartitionRecord.partitionNumber / partitionType（MBR 时为十六进制类型串）等。
     */
    static bool ParseSgdiskDump(const std::string &rawDump,
                                const std::string &diskId,
                                std::string &tableTypeOut,
                                std::vector<PartitionRecord> &partsOut);

    /** 与 storage_service DiskInfo::CreateMBRVolume 白名单对齐的简化判断。 */
    static bool IsMbrTypeSupportedForVolume(const std::string &mbrTypeHex);

private:
    static std::string Trim(const std::string &s);
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_PARTITION_TABLE_PARSER_H
