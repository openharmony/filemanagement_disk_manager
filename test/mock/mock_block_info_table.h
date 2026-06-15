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

#ifndef OHOS_DISK_MANAGER_MOCK_BLOCK_INFO_TABLE_H
#define OHOS_DISK_MANAGER_MOCK_BLOCK_INFO_TABLE_H

#include "block_info.h"

#include <cstdint>
#include <string>
#include <unordered_map>

#include <gmock/gmock.h>

namespace OHOS {
namespace DiskManager {

using ExtraKeyValues = std::unordered_map<std::string, std::string>;

class BlockInfoTable {
public:
    static BlockInfoTable &GetInstance();

    MOCK_METHOD(int32_t, ReloadFromDaemon, ());
    MOCK_METHOD(bool, TryCopyByDiskId, (const std::string &diskId, BlockInfo &outBlockInfo), (const));
    MOCK_METHOD(int32_t, ReadExtDiskInfoFromDaemon, (const std::string &devName, BlockInfo &info));
    MOCK_METHOD(std::string, ToJsonStringWithExtrasImpl,
        (const BlockInfo &blockInfo, const ExtraKeyValues &extraKeyValues), (const));

    static BlockInfoTable *mockInstance_;

    static std::string ToJsonStringWithExtras(const BlockInfo &blockInfo, const ExtraKeyValues &extraKeyValues = {});
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_BLOCK_INFO_TABLE_H