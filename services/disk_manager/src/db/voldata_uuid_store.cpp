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

#include "voldata_uuid_store.h"

#include "disk_manager_errno.h"
#include "disk_manager_hilog.h"

#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <system_error>
#include <unistd.h>

namespace OHOS {
namespace DiskManager {
namespace {
constexpr const char *DISK_MANAGER_DATA_PATH = "/data/service/el1/public/disk_manager/";
constexpr const char *VOLDATA_UUID_MAPPING_JSON = "voldata_uuid_mapping.json";
constexpr const char *TMP_FILE_SUFFIX = ".tmp";
constexpr const char *VOLDATA_MOUNT_PREFIX = "/mnt/data/voldata/data";
constexpr uint32_t MAX_VOLDATA_SLOT_COUNT = 20;

bool IsSafeFsUuidLocal(const std::string &fsUuid)
{
    if (fsUuid.empty()) {
        return false;
    }
    return fsUuid.find("..") == std::string::npos && fsUuid.find('/') == std::string::npos;
}

bool ParseVoldataSlotFromMountPath(const std::string &mountPath, uint32_t &outSlot)
{
    const std::string prefix(VOLDATA_MOUNT_PREFIX);
    if (mountPath.size() <= prefix.size() || mountPath.compare(0, prefix.size(), prefix) != 0) {
        return false;
    }
    for (size_t i = prefix.size(); i < mountPath.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(mountPath[i]))) {
            return false;
        }
    }
    errno = 0;
    char *end = nullptr;
    const unsigned long parsed = std::strtoul(mountPath.c_str() + prefix.size(), &end, 10); // NOLINT(cert-str34-c)
    if (errno != 0 || end == mountPath.c_str() + prefix.size()) {
        return false;
    }
    if (parsed < 1UL || parsed > static_cast<unsigned long>(MAX_VOLDATA_SLOT_COUNT)) {
        return false;
    }
    outSlot = static_cast<uint32_t>(parsed);
    return true;
}
} // namespace

bool VoldataUuidEntry::FromJson(const nlohmann::json &jsonObject)
{
    if (!jsonObject.is_object()) {
        return false;
    }
    if (!jsonObject.contains("fsUuid") || !jsonObject["fsUuid"].is_string()) {
        return false;
    }
    if (!jsonObject.contains("mountPath") || !jsonObject["mountPath"].is_string()) {
        return false;
    }
    fsUuid = jsonObject["fsUuid"].get<std::string>();
    mountPath = jsonObject["mountPath"].get<std::string>();
    if (!ParseVoldataSlotFromMountPath(mountPath, slotIndex)) {
        return false;
    }
    return IsSafeFsUuidLocal(fsUuid);
}

nlohmann::json VoldataUuidEntry::ToJson() const
{
    return nlohmann::json {
        {"fsUuid", fsUuid},
        {"mountPath", mountPath},
        {"slotIndex", slotIndex},
    };
}

VoldataUuidStore &VoldataUuidStore::GetInstance()
{
    static VoldataUuidStore instance;
    return instance;
}

bool VoldataUuidStore::IsSafeFsUuid(const std::string &fsUuid)
{
    if (fsUuid.empty()) {
        return false;
    }
    return fsUuid.find("..") == std::string::npos && fsUuid.find('/') == std::string::npos;
}

std::string VoldataUuidStore::BuildMountPathForSlot(uint32_t slotIndex)
{
    return std::string(VOLDATA_MOUNT_PREFIX) + std::to_string(slotIndex);
}

int32_t VoldataUuidStore::Init()
{
    std::lock_guard<std::mutex> initLock(initMutex_);
    if (initialized_) {
        return DiskManagerErrNo::E_OK;
    }

    jsonFilePath_ = std::string(DISK_MANAGER_DATA_PATH) + VOLDATA_UUID_MAPPING_JSON;
    const int32_t loadRet = LoadFromFile();
    if (loadRet != DiskManagerErrNo::E_OK) {
        LOGE("VoldataUuidStore Init load failed ret=%{public}d", loadRet);
        return loadRet;
    }

    initialized_ = true;
    LOGI("VoldataUuidStore Init ok entries=%{public}zu", uuidMap_.size());
    return DiskManagerErrNo::E_OK;
}

int32_t VoldataUuidStore::UnInit()
{
    std::lock_guard<std::mutex> initLock(initMutex_);
    if (!initialized_) {
        return DiskManagerErrNo::E_OK;
    }

    std::unique_lock<std::shared_mutex> dataLock(dataMutex_);
    uuidMap_.clear();
    initialized_ = false;
    return DiskManagerErrNo::E_OK;
}

int32_t VoldataUuidStore::ResolveMountPath(const std::string &fsUuid, std::string &outMountPath, bool &outCreated)
{
    outCreated = false;
    if (!IsSafeFsUuid(fsUuid)) {
        LOGE("ResolveMountPath invalid fsUuid");
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    const int32_t initRet = Init();
    if (initRet != DiskManagerErrNo::E_OK) {
        return initRet;
    }

    std::unique_lock<std::shared_mutex> dataLock(dataMutex_);

    const auto existing = uuidMap_.find(fsUuid);
    if (existing != uuidMap_.end()) {
        outMountPath = existing->second.mountPath;
        LOGI("ResolveMountPath reuse uuid path=%{public}s", outMountPath.c_str());
        return DiskManagerErrNo::E_OK;
    }

    std::unordered_map<std::string, VoldataUuidEntry> backup = uuidMap_;
    const uint32_t slot = AllocateNextSlotLocked();
    VoldataUuidEntry entry;
    entry.fsUuid = fsUuid;
    entry.slotIndex = slot;
    entry.mountPath = BuildMountPathForSlot(slot);
    uuidMap_[fsUuid] = entry;

    const int32_t saveRet = SaveToFile();
    if (saveRet != DiskManagerErrNo::E_OK) {
        uuidMap_ = std::move(backup);
        LOGE("ResolveMountPath save failed ret=%{public}d", saveRet);
        return saveRet;
    }

    outMountPath = entry.mountPath;
    outCreated = true;
    LOGI("ResolveMountPath new uuid slot=%{public}u path=%{public}s", slot, outMountPath.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t VoldataUuidStore::RemoveByFsUuid(const std::string &fsUuid)
{
    if (!IsSafeFsUuid(fsUuid)) {
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    const int32_t initRet = Init();
    if (initRet != DiskManagerErrNo::E_OK) {
        return initRet;
    }

    std::unique_lock<std::shared_mutex> dataLock(dataMutex_);
    const auto it = uuidMap_.find(fsUuid);
    if (it == uuidMap_.end()) {
        return DiskManagerErrNo::E_OK;
    }

    std::unordered_map<std::string, VoldataUuidEntry> backup = uuidMap_;
    uuidMap_.erase(it);
    const int32_t saveRet = SaveToFile();
    if (saveRet != DiskManagerErrNo::E_OK) {
        uuidMap_ = std::move(backup);
        LOGE("RemoveByFsUuid save failed ret=%{public}d", saveRet);
        return saveRet;
    }

    LOGI("RemoveByFsUuid ok uuid=%{public}s", fsUuid.c_str());
    return DiskManagerErrNo::E_OK;
}

int32_t VoldataUuidStore::ReplaceFsUuid(const std::string &oldFsUuid, const std::string &newFsUuid)
{
    if (!IsSafeFsUuid(oldFsUuid) || !IsSafeFsUuid(newFsUuid)) {
        LOGE("ReplaceFsUuid invalid fsUuid");
        return DiskManagerErrNo::DISK_MGR_ERR;
    }
    if (oldFsUuid == newFsUuid) {
        return DiskManagerErrNo::E_OK;
    }

    const int32_t initRet = Init();
    if (initRet != DiskManagerErrNo::E_OK) {
        return initRet;
    }

    std::unique_lock<std::shared_mutex> dataLock(dataMutex_);
    const auto oldIt = uuidMap_.find(oldFsUuid);
    if (oldIt == uuidMap_.end()) {
        LOGI("ReplaceFsUuid no mapping for old uuid=%{public}s", oldFsUuid.c_str());
        return DiskManagerErrNo::E_OK;
    }
    if (uuidMap_.find(newFsUuid) != uuidMap_.end()) {
        LOGE("ReplaceFsUuid new uuid already mapped uuid=%{public}s", newFsUuid.c_str());
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    std::unordered_map<std::string, VoldataUuidEntry> backup = uuidMap_;
    VoldataUuidEntry entry = oldIt->second;
    entry.fsUuid = newFsUuid;
    uuidMap_.erase(oldIt);
    uuidMap_[newFsUuid] = entry;

    const int32_t saveRet = SaveToFile();
    if (saveRet != DiskManagerErrNo::E_OK) {
        uuidMap_ = std::move(backup);
        LOGE("ReplaceFsUuid save failed ret=%{public}d", saveRet);
        return saveRet;
    }

    LOGI("ReplaceFsUuid ok old=%{public}s new=%{public}s path=%{public}s slot=%{public}u", oldFsUuid.c_str(),
         newFsUuid.c_str(), entry.mountPath.c_str(), entry.slotIndex);
    return DiskManagerErrNo::E_OK;
}

bool VoldataUuidStore::TryGetMountPath(const std::string &fsUuid, std::string &outMountPath) const
{
    if (!IsSafeFsUuid(fsUuid)) {
        return false;
    }

    std::shared_lock<std::shared_mutex> dataLock(dataMutex_);
    const auto it = uuidMap_.find(fsUuid);
    if (it == uuidMap_.end()) {
        return false;
    }
    outMountPath = it->second.mountPath;
    return true;
}

void VoldataUuidStore::EvictSlotOneIfFullLocked()
{
    if (uuidMap_.size() < MAX_VOLDATA_SLOT_COUNT) {
        return;
    }
    for (auto it = uuidMap_.begin(); it != uuidMap_.end(); ++it) {
        if (it->second.slotIndex == 1U) {
            LOGI("EvictSlotOneIfFullLocked remove uuid=%{public}s path=%{public}s", it->first.c_str(),
                 it->second.mountPath.c_str());
            uuidMap_.erase(it);
            return;
        }
    }
    if (!uuidMap_.empty()) {
        LOGW("EvictSlotOneIfFullLocked no slot1 entry, remove arbitrary oldest");
        uuidMap_.erase(uuidMap_.begin());
    }
}

uint32_t VoldataUuidStore::FindMaxUsedSlotLocked() const
{
    uint32_t maxSlot = 0;
    for (const auto &pair : uuidMap_) {
        if (pair.second.slotIndex > maxSlot) {
            maxSlot = pair.second.slotIndex;
        }
    }
    return maxSlot;
}

uint32_t VoldataUuidStore::AllocateNextSlotLocked()
{
    uint32_t nextSlot = FindMaxUsedSlotLocked() + 1U;
    if (nextSlot > MAX_VOLDATA_SLOT_COUNT) {
        EvictSlotOneIfFullLocked();
        nextSlot = 1U;
    }
    return nextSlot;
}

int32_t VoldataUuidStore::LoadFromFile()
{
    uuidMap_.clear();

    std::error_code errorCode;
    if (!std::filesystem::exists(jsonFilePath_, errorCode)) {
        LOGI("VoldataUuidStore JSON not exists, create empty: %{public}s", jsonFilePath_.c_str());
        return SaveToFile();
    }

    std::ifstream file(jsonFilePath_);
    if (!file.is_open()) {
        LOGE("VoldataUuidStore open read failed: %{public}s", jsonFilePath_.c_str());
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    const std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (jsonStr.empty()) {
        return DiskManagerErrNo::E_OK;
    }

    if (!nlohmann::json::accept(jsonStr)) {
        LOGE("VoldataUuidStore invalid JSON: %{public}s", jsonFilePath_.c_str());
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    nlohmann::json root = nlohmann::json::parse(jsonStr, nullptr, false);
    if (root.is_discarded()) {
        LOGE("VoldataUuidStore parse discarded");
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    if (!root.is_array()) {
        LOGE("VoldataUuidStore root is not array");
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    for (const auto &item : root) {
        if (uuidMap_.size() >= MAX_VOLDATA_SLOT_COUNT) {
            LOGW("VoldataUuidStore load truncated at max=%{public}u", MAX_VOLDATA_SLOT_COUNT);
            break;
        }
        VoldataUuidEntry entry;
        if (!entry.FromJson(item)) {
            LOGW("VoldataUuidStore skip invalid item");
            continue;
        }
        uuidMap_[entry.fsUuid] = entry;
    }

    LOGI("VoldataUuidStore loaded %{public}zu entries", uuidMap_.size());
    return DiskManagerErrNo::E_OK;
}

int32_t VoldataUuidStore::SaveToFile()
{
    nlohmann::json jsonArray = nlohmann::json::array();
    for (const auto &pair : uuidMap_) {
        jsonArray.push_back(pair.second.ToJson());
    }
    return SaveJsonToFile(jsonFilePath_, jsonArray);
}

int32_t VoldataUuidStore::SaveJsonToFile(const std::string &filePath, const nlohmann::json &jsonData)
{
    const std::string jsonStr = jsonData.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);

    std::error_code errorCode;
    const std::filesystem::path dirPath = std::filesystem::path(filePath).parent_path();
    std::filesystem::create_directories(dirPath, errorCode);
    if (errorCode) {
        LOGE("VoldataUuidStore create_directories failed errno=%{public}d", errorCode.value());
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    const std::string tempFilePath = filePath + TMP_FILE_SUFFIX;
    std::ofstream tempFile(tempFilePath, std::ios::trunc);
    if (!tempFile.is_open()) {
        LOGE("VoldataUuidStore open temp failed: %{public}s", tempFilePath.c_str());
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    tempFile << jsonStr;
    tempFile.close();
    if (tempFile.fail()) {
        LOGE("VoldataUuidStore write temp failed");
        std::remove(tempFilePath.c_str());
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    if (rename(tempFilePath.c_str(), filePath.c_str()) != 0) {
        LOGE("VoldataUuidStore rename failed errno=%{public}d", errno);
        std::remove(tempFilePath.c_str());
        return DiskManagerErrNo::DISK_MGR_ERR;
    }

    return DiskManagerErrNo::E_OK;
}

} // namespace DiskManager
} // namespace OHOS
