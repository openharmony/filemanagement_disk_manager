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

#include "blockinfotable_fuzzer.h"

#include "block_info.h"
#include "block_info_table.h"
#include "fuzzer/FuzzedDataProvider.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr size_t MAX_STRING_LEN = 256;
// 容器规模受控，防止内存爆炸与变异预算稀释
constexpr uint8_t MAX_EXTRA_COUNT = 8;
constexpr uint8_t MAX_VECTOR_COUNT = 8;
} // namespace

static void FillBlockInfo(BlockInfo &info, FuzzedDataProvider &fdp)
{
    info.sizeBytes = fdp.ConsumeIntegral<uint64_t>();
    info.rpm = fdp.ConsumeIntegral<uint32_t>();
    info.rotational = fdp.ConsumeIntegral<int32_t>();
    info.removable = fdp.ConsumeBool();
    info.vendor = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.model = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.interfaceType = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.serialNumber = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.devicePath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.port = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.devnum = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.busnum = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.devNode = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.scsiBusNum = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    info.fwVersion = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    // 生产路径 ODD_INFO 仅为 null 或合法 JSON；discarded 值不经赋值，避免构造生产不可达状态
    nlohmann::json oddInfo = nlohmann::json::parse(fdp.ConsumeRandomLengthString(MAX_STRING_LEN), nullptr, false);
    if (!oddInfo.is_discarded()) {
        info.ODD_INFO = oddInfo;
    }
}

bool BlockInfoTableFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);

    // 1. ToJsonStringWithExtras：BlockInfo + 受控数量 extras 键值对（含重复 key / 非法 UTF-8）
    BlockInfo info;
    FillBlockInfo(info, fdp);
    std::unordered_map<std::string, std::string> extras;
    uint8_t extraCount = fdp.ConsumeIntegral<uint8_t>() % MAX_EXTRA_COUNT;
    for (uint8_t i = 0; i < extraCount; ++i) {
        std::string key = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        std::string value = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
        extras[key] = value;
    }
    (void)BlockInfoTable::ToJsonStringWithExtras(info, extras);
    (void)BlockInfoTable::ToJsonStringWithExtras(info);

    // 2. 批量序列化（数组成员规模受控）
    std::vector<BlockInfo> infos;
    uint8_t vectorCount = fdp.ConsumeIntegral<uint8_t>() % MAX_VECTOR_COUNT;
    for (uint8_t i = 0; i < vectorCount; ++i) {
        BlockInfo item;
        FillBlockInfo(item, fdp);
        infos.push_back(item);
    }
    (void)BlockInfo::SerializeVector(infos);

    // 3. 空表只读查询（不触发 daemon IPC，未命中返回 false）
    std::string diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    BlockInfo outInfo;
    (void)BlockInfoTable::GetInstance().TryCopyByDiskId(diskId, outInfo);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::BlockInfoTableFuzzTest(data, size);
    return 0;
}
