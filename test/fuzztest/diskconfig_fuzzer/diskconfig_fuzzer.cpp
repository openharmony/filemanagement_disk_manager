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

#include "diskconfig_fuzzer.h"

#include "disk_config.h"
#include "fuzzer/FuzzedDataProvider.h"

#include <string>

namespace OHOS {
using namespace DiskManager;

namespace {
// 限制 pattern 长度：fnmatch 对 "*a*a*a*..." 类模式存在指数回溯，超长输入会拖垮变异效率
constexpr size_t MAX_PATTERN_LEN = 256;
} // namespace

bool DiskConfigFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);

    std::string configPattern = fdp.ConsumeRandomLengthString(MAX_PATTERN_LEN);
    std::string label = fdp.ConsumeRandomLengthString(MAX_PATTERN_LEN);
    int32_t flag = fdp.ConsumeIntegral<int32_t>();

    DiskConfig config(configPattern, label, flag);
    (void)config.GetFlag();

    // 设备侧 sysPath 与配置模式做 fnmatch 匹配（含 * ? [ ] \ 等特殊字符组合）
    std::string devicePattern = fdp.ConsumeRandomLengthString(MAX_PATTERN_LEN);
    (void)config.IsMatch(devicePattern);

    // 交换 pattern / device 角色再验证一轮，覆盖双向匹配路径
    DiskConfig reversedConfig(devicePattern, label, flag);
    (void)reversedConfig.IsMatch(configPattern);
    (void)reversedConfig.GetFlag();

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DiskConfigFuzzTest(data, size);
    return 0;
}
