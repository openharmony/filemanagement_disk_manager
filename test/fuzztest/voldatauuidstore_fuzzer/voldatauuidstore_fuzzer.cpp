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

#include "voldatauuidstore_fuzzer.h"

#include "fuzzer/FuzzedDataProvider.h"
#include "voldata_uuid_store.h"

#include <string>

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr size_t MAX_STRING_LEN = 4096;
// slot 合法区间为 [1, 1000]，取模 2000 覆盖越界值（0 与 >1000）
constexpr uint32_t SLOT_RANGE = 2000;
} // namespace

bool VoldataUuidStoreFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);

    // 1. 模拟持久化文件内容：fuzz 文本 -> JSON -> FromJson 严格校验
    //    （fsUuid 合法性、mountPath 前缀 /mnt/data/voldata/dataN、slot 范围）
    std::string rawJson = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    nlohmann::json js = nlohmann::json::parse(rawJson, nullptr, false);
    VoldataUuidEntry entry;
    (void)entry.FromJson(js);

    // 2. ToJson 序列化 -> 再 FromJson 往返校验
    nlohmann::json outJson = entry.ToJson();
    VoldataUuidEntry roundTrip;
    (void)roundTrip.FromJson(outJson);
    (void)roundTrip.ToJson();

    // 3. 用 fuzz 字段直接填充 entry 后序列化（含非法 UTF-8 / 越界 slot）
    VoldataUuidEntry manual;
    manual.fsUuid = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    manual.mountPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    manual.slotIndex = fdp.ConsumeIntegral<uint32_t>() % SLOT_RANGE;
    nlohmann::json manualJson = manual.ToJson();
    VoldataUuidEntry manualRoundTrip;
    (void)manualRoundTrip.FromJson(manualJson);

    // 4. 未初始化单例上的只读查询（不触发 Init/文件 IO，空表返回 false）
    std::string fsUuid = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string mountPath;
    (void)VoldataUuidStore::GetInstance().TryGetMountPath(fsUuid, mountPath);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::VoldataUuidStoreFuzzTest(data, size);
    return 0;
}
