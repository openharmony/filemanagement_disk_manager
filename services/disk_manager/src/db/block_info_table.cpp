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

#include "block_info_table.h"

#include "adapter/storage_daemon_adapter.h"
#include "disk_manager_hilog.h"
#include "errors.h"

#include <algorithm>
#include <cctype>
#include <cstdint>

#include <nlohmann/json.hpp>

namespace OHOS {
namespace DiskManager {

namespace {

using json = nlohmann::json;

constexpr size_t BLOCK_INFO_MAX_COUNT = 20U;
constexpr const char *BLOCK_INFO_SCAN_PAYLOAD_TYPE = "data";

bool IsAsciiSpace(char asciiChar)
{
    return std::isspace(static_cast<unsigned char>(asciiChar)) != 0;
}

std::string TrimAsciiSpaces(std::string text)
{
    const auto beginIt = std::find_if_not(text.begin(), text.end(), IsAsciiSpace);
    const auto endIt = std::find_if_not(text.rbegin(), text.rend(), IsAsciiSpace).base();
    if (beginIt >= endIt) {
        return "";
    }
    return std::string(beginIt, endIt);
}

std::string ReadStringOrEmpty(const json &jsonObject, const char *key)
{
    if (!jsonObject.contains(key) || jsonObject[key].is_null() || !jsonObject[key].is_string()) {
        return "";
    }
    return TrimAsciiSpaces(jsonObject[key].get<std::string>());
}

uint64_t ReadUInt64OrZero(const json &jsonObject, const char *key)
{
    if (!jsonObject.contains(key) || jsonObject[key].is_null()) {
        return UINT64_C(0);
    }
    if (jsonObject[key].is_number_unsigned()) {
        return jsonObject[key].get<uint64_t>();
    }
    if (jsonObject[key].is_number_integer()) {
        const auto value = jsonObject[key].get<long long>();
        return value >= 0 ? static_cast<uint64_t>(value) : UINT64_C(0);
    }
    return UINT64_C(0);
}

uint32_t ReadUInt32OrZero(const json &jsonObject, const char *key)
{
    const uint64_t value = ReadUInt64OrZero(jsonObject, key);
    return value <= static_cast<uint64_t>(UINT32_MAX) ? static_cast<uint32_t>(value) : UINT32_C(0);
}

bool ReadBoolLikeOrFalse(const json &jsonObject, const char *key)
{
    if (!jsonObject.contains(key) || jsonObject[key].is_null()) {
        return false;
    }
    if (jsonObject[key].is_boolean()) {
        return jsonObject[key].get<bool>();
    }
    if (jsonObject[key].is_number_integer()) {
        return jsonObject[key].get<int>() != 0;
    }
    return false;
}

bool FillBlockInfo(const json &jsonObject, BlockInfo &blockInfo)
{
    if (!jsonObject.is_object()) {
        return false;
    }
    blockInfo.diskId = ReadStringOrEmpty(jsonObject, "diskId");
    blockInfo.sizeBytes = ReadUInt64OrZero(jsonObject, "sizeBytes");
    blockInfo.vendor = ReadStringOrEmpty(jsonObject, "vendor");
    blockInfo.model = ReadStringOrEmpty(jsonObject, "model");
    blockInfo.interfaceType = ReadStringOrEmpty(jsonObject, "interfaceType");
    blockInfo.rpm = ReadUInt32OrZero(jsonObject, "rpm");
    blockInfo.state = ReadStringOrEmpty(jsonObject, "state");
    blockInfo.mediaType = ReadStringOrEmpty(jsonObject, "mediaType");
    blockInfo.removable = ReadBoolLikeOrFalse(jsonObject, "removable");
    blockInfo.serialNumber = ReadStringOrEmpty(jsonObject, "serialNumber");
    blockInfo.pciePath = ReadStringOrEmpty(jsonObject, "pciePath");
    blockInfo.location = ReadStringOrEmpty(jsonObject, "location");
    blockInfo.usedBytes = ReadUInt64OrZero(jsonObject, "usedBytes");
    blockInfo.availableBytes = ReadUInt64OrZero(jsonObject, "availableBytes");
    blockInfo.devicePath = ReadStringOrEmpty(jsonObject, "devicePath");
    blockInfo.port = ReadStringOrEmpty(jsonObject, "port");
    return !blockInfo.diskId.empty();
}

json BlockInfoToJsonObject(const BlockInfo &blockInfo)
{
    return json {
        {"sizeBytes", blockInfo.sizeBytes},
        {"vendor", blockInfo.vendor},
        {"model", blockInfo.model},
        {"interfaceType", blockInfo.interfaceType},
        {"rpm", blockInfo.rpm},
        {"state", blockInfo.state},
        {"mediaType", blockInfo.mediaType},
        {"removable", blockInfo.removable},
        {"serialNumber", blockInfo.serialNumber},
        {"pciePath", blockInfo.pciePath},
        {"location", blockInfo.location},
        {"diskId", blockInfo.diskId},
        {"usedBytes", blockInfo.usedBytes},
        {"availableBytes", blockInfo.availableBytes},
        {"devicePath", blockInfo.devicePath},
        {"port", blockInfo.port},
    };
}

void UpsertObjectsIntoMap(std::unordered_map<std::string, BlockInfo> &blockInfoMap, const json &rootJson)
{
    const auto ingestObject = [&](const json &jsonObject) {
        BlockInfo parsedBlockInfo {};
        if (!FillBlockInfo(jsonObject, parsedBlockInfo)) {
            return;
        }
        blockInfoMap.insert_or_assign(parsedBlockInfo.diskId, std::move(parsedBlockInfo));
    };
    const auto walkArray = [&](const json &jsonArray) {
        const size_t maxIngestCount = std::min(jsonArray.size(), BLOCK_INFO_MAX_COUNT);
        if (jsonArray.size() > BLOCK_INFO_MAX_COUNT) {
            LOGW("BlockInfoTable ingest truncated len=%{public}zu max=%{public}zu", jsonArray.size(),
                 static_cast<size_t>(BLOCK_INFO_MAX_COUNT));
        }
        for (size_t index = 0; index < maxIngestCount; ++index) {
            const auto &jsonElement = jsonArray[index];
            if (jsonElement.is_object()) {
                ingestObject(jsonElement);
            }
        }
    };

    if (rootJson.is_null()) {
        return;
    }
    if (rootJson.is_array()) {
        walkArray(rootJson);
        return;
    }
    if (!rootJson.is_object()) {
        return;
    }
    if (rootJson.contains("blocks") && rootJson["blocks"].is_array()) {
        walkArray(rootJson["blocks"]);
        return;
    }
    ingestObject(rootJson);
}

} // namespace

BlockInfoTable &BlockInfoTable::GetInstance()
{
    static BlockInfoTable tableInstance;
    return tableInstance;
}

int32_t BlockInfoTable::ReloadFromDaemon()
{
    LOGI("BlockInfoTable::ReloadFromDaemon enter");
    std::string blockInfosJsonString;
    const int32_t errCode = StorageDaemonAdapter::GetInstance().GetBlockInfoByType(
        std::string(BLOCK_INFO_SCAN_PAYLOAD_TYPE), blockInfosJsonString);
    if (errCode != ERR_OK) {
        LOGW("BlockInfoTable ReloadFromDaemon RPC err=%{public}d", errCode);
        return errCode;
    }
    std::unordered_map<std::string, BlockInfo> nextBlockInfoMap {};
    if (!blockInfosJsonString.empty()) {
        json rootJson = json::parse(blockInfosJsonString, nullptr, false);
        if (rootJson.is_discarded()) {
            LOGW("BlockInfoTable ReloadFromDaemon JSON parse failed len=%{public}zu", blockInfosJsonString.size());
            return ERR_INVALID_DATA;
        }
        UpsertObjectsIntoMap(nextBlockInfoMap, rootJson);
    }
    const size_t entryCount = nextBlockInfoMap.size();
    {
        std::lock_guard<std::mutex> lockGuard(mutex_);
        blockInfoMap_ = std::move(nextBlockInfoMap);
    }
    LOGI("BlockInfoTable ReloadFromDaemon entries=%{public}zu", entryCount);
    return ERR_OK;
}

bool BlockInfoTable::TryCopyByDiskId(const std::string &diskId, BlockInfo &outBlockInfo) const
{
    LOGI("BlockInfoTable::TryCopyByDiskId enter diskId=%{public}s", diskId.c_str());
    std::lock_guard<std::mutex> lockGuard(mutex_);
    auto iter = blockInfoMap_.find(diskId);
    if (iter == blockInfoMap_.end()) {
        return false;
    }
    outBlockInfo = iter->second;
    return true;
}

std::string BlockInfoTable::ToJsonStringWithExtras(
    const BlockInfo &blockInfo,
    const std::unordered_map<std::string, std::string> &extraKeyValues)
{
    LOGI("BlockInfoTable::ToJsonStringWithExtras enter diskId=%{public}s extraCount=%{public}zu",
         blockInfo.diskId.c_str(), extraKeyValues.size());
    json jsonObject = BlockInfoToJsonObject(blockInfo);
    for (const auto &[key, value] : extraKeyValues) {
        jsonObject[key] = value;
    }
    return jsonObject.dump();
}

int32_t BlockInfoTable::ReadExtDiskInfoFromDaemon(const std::string &devName, BlockInfo &info)
{
    LOGI("BlockInfoTable::ReadExtDiskInfoFromDaemon enter");
    std::string jsonString;
    const int32_t errCode = StorageDaemonAdapter::GetInstance().GetBlockInfoByType(devName, jsonString, info.diskId);
    if (errCode != ERR_OK) {
        LOGW("BlockInfoTable ReadExtDiskInfoFromDaemon RPC err=%{public}d", errCode);
        return errCode;
    }
    std::unordered_map<std::string, BlockInfo> nextBlockInfoMap {};
    if (!jsonString.empty()) {
        json rootJson = json::parse(jsonString, nullptr, false);
        if (rootJson.is_discarded()) {
            LOGW("BlockInfoTable ReadExtDiskInfoFromDaemon JSON parse failed len=%{public}zu", jsonString.size());
            return ERR_INVALID_DATA;
        }
        UpsertObjectsIntoMap(nextBlockInfoMap, rootJson);
    }
    if (!nextBlockInfoMap.empty()) {
        info = nextBlockInfoMap[info.diskId];
    }
    LOGI("BlockInfoTable ReadExtDiskInfoFromDaemon success.");
    return ERR_OK;
}
} // namespace DiskManager
} // namespace OHOS
