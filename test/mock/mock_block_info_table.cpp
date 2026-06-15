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

#include "mock_block_info_table.h"

namespace OHOS {
namespace DiskManager {

BlockInfoTable *BlockInfoTable::mockInstance_ = nullptr;

BlockInfoTable &BlockInfoTable::GetInstance()
{
    if (mockInstance_ == nullptr) {
        mockInstance_ = new BlockInfoTable();
    }
    return *mockInstance_;
}

std::string BlockInfoTable::ToJsonStringWithExtras(const BlockInfo &blockInfo, const ExtraKeyValues &extraKeyValues)
{
    return GetInstance().ToJsonStringWithExtrasImpl(blockInfo, extraKeyValues);
}

} // namespace DiskManager
} // namespace OHOS