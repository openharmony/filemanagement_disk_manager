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

#include "partitiontableparser_fuzzer.h"

#include "partition_table_parser.h"

#include <string>
#include <vector>

namespace OHOS {
using namespace DiskManager;

bool PartitionTableParserFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    std::string rawDump(reinterpret_cast<const char *>(data), size);
    std::string diskId("disk-1-1");
    std::string tableType;
    std::vector<PartitionRecord> parts;

    PartitionTableParser::ParseSgdiskDump(rawDump, diskId, tableType, parts);

    std::string hexStr(reinterpret_cast<const char *>(data), std::min(size, size_t(8)));
    PartitionTableParser::IsMbrTypeSupportedForVolume(hexStr);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::PartitionTableParserFuzzTest(data, size);
    return 0;
}
