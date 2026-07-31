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

#include "blockinfo_fuzzer.h"

#include "block_info.h"

#include <string>
#include <vector>

namespace OHOS {
using namespace DiskManager;

namespace {
constexpr size_t MIN_DATA_SIZE = sizeof(uint64_t) + sizeof(uint32_t);
constexpr size_t VENDOR_MAX_LEN = 32;
} // namespace

void FuzzBlockInfoToJson(const uint8_t *data, size_t size)
{
    BlockInfo info;
    info.sizeBytes = *(reinterpret_cast<const uint64_t *>(data));
    info.vendor = std::string(reinterpret_cast<const char *>(data + sizeof(uint64_t)),
        std::min(size - sizeof(uint64_t), VENDOR_MAX_LEN));
    info.model = "test-model";
    info.interfaceType = "USB";
    info.rpm = 0;
    info.removable = true;
    info.serialNumber = "SN12345";
    info.diskId = "disk-1-1";
    info.devicePath = "/dev/block/sda";
    info.port = "1-1";
    info.devnum = "1";
    info.busnum = "1";
    info.devNode = "/dev/sda";
    info.scsiBusNum = "0";
    info.fwVersion = "1.0";
    info.ToJson();
}

void FuzzBlockInfoSerializeVector(const uint8_t *data, size_t size)
{
    BlockInfo info;
    info.sizeBytes = *(reinterpret_cast<const uint64_t *>(data));
    info.vendor = "test-vendor";
    info.diskId = "disk-1-1";
    info.devicePath = "/dev/block/sda";

    std::vector<BlockInfo> infos;
    infos.push_back(info);
    infos.push_back(info);
    BlockInfo::SerializeVector(infos);
}

bool BlockInfoFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr || size < MIN_DATA_SIZE) {
        return false;
    }

    FuzzBlockInfoToJson(data, size);
    FuzzBlockInfoSerializeVector(data, size);

    return true;
}
} // namespace OHOS

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::BlockInfoFuzzTest(data, size);
    return 0;
}
