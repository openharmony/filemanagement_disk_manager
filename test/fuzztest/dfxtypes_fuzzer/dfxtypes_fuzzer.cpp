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

#include "dfxtypes_fuzzer.h"

#include "disk_manager_dfx_types.h"
#include "fuzzer/FuzzedDataProvider.h"

#include <string>

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr size_t MAX_STRING_LEN = 256;
// 与 disk_manager 常量对齐：opDiag 报文上限、DFX 截断上限
constexpr size_t OP_DIAG_MAX_LEN = 8192;
constexpr size_t TRUNCATE_MAX_LEN = 2048;
// 覆盖合法枚举值 + 非法值，命中 AutoMountSkipReasonToString 的 default 分支
constexpr uint8_t SKIP_REASON_RANGE = 8;
} // namespace

bool DfxTypesFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size == 0) {
        return false;
    }

    FuzzedDataProvider fdp(data, size);

    // 1. opDiag 原始报文解析（storage_daemon 经 ReportVolumeOpDiag 上报的 JSON 文本）
    std::string opDiag = fdp.ConsumeRandomLengthString(OP_DIAG_MAX_LEN);
    VolumeReportInfo info = ParseOpDiagText(opDiag);
    (void)info.ToExtraData();
    OpDiagReport report = ParseOpDiagReport(opDiag);
    (void)report.info.ToExtraData();

    // 2. DfxTruncate 边界（maxLen 覆盖 0 / 小于 / 等于 / 大于文本长度）
    std::string text = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    size_t maxLen = static_cast<size_t>(fdp.ConsumeIntegral<uint32_t>()) % (TRUNCATE_MAX_LEN + 1);
    (void)DfxTruncate(text, maxLen);
    (void)DfxTruncate(opDiag);

    // 3. 自动挂载跳过原因推导 + 枚举转字符串（含非法枚举值）
    std::string type = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string uuid = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    bool autoMountEnabled = fdp.ConsumeBool();
    AutoMountSkipReason reason = ResolveAutoMountSkipReason(autoMountEnabled, type, uuid);
    (void)AutoMountSkipReasonToString(reason);
    AutoMountSkipReason fuzzReason =
        static_cast<AutoMountSkipReason>(fdp.ConsumeIntegral<uint8_t>() % SKIP_REASON_RANGE);
    (void)AutoMountSkipReasonToString(fuzzReason);

    // 4. 跳过上下文构造报告信息
    AutoMountSkipContext ctx;
    ctx.volId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    ctx.diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    ctx.volDevPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    ctx.type = type;
    ctx.uuid = uuid;
    ctx.autoMountEnabled = autoMountEnabled;
    VolumeReportInfo skipInfo = BuildAutoMountSkipReportInfo(ctx);
    (void)skipInfo.ToExtraData();

    // 5. 链式 With + MergeFrom 合并语义（空字段不覆盖、extra 追加）
    VolumeReportInfo merged;
    std::string volumeId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string diskId = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string devPath = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string fsType = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string fsUuid = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    std::string extra = fdp.ConsumeRandomLengthString(MAX_STRING_LEN);
    merged.WithVolumeId(volumeId).WithDiskId(diskId).WithDevPath(devPath).WithFsType(fsType);
    merged.fsUuid = fsUuid;
    merged.extra = extra;
    merged.MergeFrom(info);
    merged.MergeFrom(skipInfo);
    (void)merged.ToExtraData();

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::DfxTypesFuzzTest(data, size);
    return 0;
}
