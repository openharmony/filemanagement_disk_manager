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

#ifndef OHOS_DISK_MANAGER_DB_BLOCK_INFO_TABLE_H
#define OHOS_DISK_MANAGER_DB_BLOCK_INFO_TABLE_H

#include "storage_spec_models.h"

#include <mutex>
#include <string>
#include <unordered_map>

namespace OHOS {
namespace DiskManager {

/**
 * BlockInfoTable：从 StorageDaemon RPC 拉取 BlockInfo，按 diskId 缓存在内存，不落盘。
 */
class BlockInfoTable {
public:
    static BlockInfoTable &GetInstance();

    int32_t ReloadFromDaemon(bool isDataDisk = true, const std::string &devName = "", const std::string &diskId = "");

    bool TryCopyByDiskId(const std::string &diskId, BlockInfo &outBlockInfo) const;

    static std::string ToJsonStringWithExtras(
        const BlockInfo &blockInfo,
        const std::unordered_map<std::string, std::string> &extraKeyValues = {});

    BlockInfoTable(const BlockInfoTable &) = delete;
    BlockInfoTable &operator=(const BlockInfoTable &) = delete;

private:
    BlockInfoTable() = default;
    ~BlockInfoTable() = default;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, BlockInfo> blockInfoMap_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DB_BLOCK_INFO_TABLE_H
