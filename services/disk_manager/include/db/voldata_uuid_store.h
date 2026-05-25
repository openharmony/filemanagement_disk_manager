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

#ifndef OHOS_DISK_MANAGER_DB_VOLDATA_UUID_STORE_H
#define OHOS_DISK_MANAGER_DB_VOLDATA_UUID_STORE_H

#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"

namespace OHOS {
namespace DiskManager {

/** PC 数据盘场景：卷 fsUuid 与 /mnt/data/voldata/dataX 挂载路径的持久化映射（最多 20 条）。 */
struct VoldataUuidEntry {
    std::string fsUuid;
    std::string mountPath;
    uint32_t slotIndex = 0;

    bool FromJson(const nlohmann::json &jsonObject);
    nlohmann::json ToJson() const;
};

class VoldataUuidStore final {
public:
    static VoldataUuidStore &GetInstance();

    int32_t Init();
    int32_t UnInit();

    /**
     * 查询已有 dataX 路径；不存在则按当前最大 slot+1 分配（满 20 时淘汰 data1 后复用 data1），
     * 仅新建时落盘。outCreated 表示本次是否新写入持久化文件。
     */
    int32_t ResolveMountPath(const std::string &fsUuid, std::string &outMountPath, bool &outCreated);

    int32_t RemoveByFsUuid(const std::string &fsUuid);

    /**
     * 格式化后 fsUuid 变化时，保留原 dataX 槽位并更新映射键。
     * 若 oldFsUuid 无映射则 no-op；若 newFsUuid 已存在则失败。
     */
    int32_t ReplaceFsUuid(const std::string &oldFsUuid, const std::string &newFsUuid);

    bool TryGetMountPath(const std::string &fsUuid, std::string &outMountPath) const;

private:
    VoldataUuidStore() = default;
    ~VoldataUuidStore() = default;
    VoldataUuidStore(const VoldataUuidStore &) = delete;
    VoldataUuidStore &operator=(const VoldataUuidStore &) = delete;

    static bool IsSafeFsUuid(const std::string &fsUuid);
    static std::string BuildMountPathForSlot(uint32_t slotIndex);

    int32_t LoadFromFile();
    int32_t SaveToFile();
    int32_t SaveJsonToFile(const std::string &filePath, const nlohmann::json &jsonData);

    void EvictSlotOneIfFullLocked();
    uint32_t FindMaxUsedSlotLocked() const;
    uint32_t AllocateNextSlotLocked();

    mutable std::mutex initMutex_;
    mutable std::shared_mutex dataMutex_;
    std::unordered_map<std::string, VoldataUuidEntry> uuidMap_;
    std::string jsonFilePath_;
    bool initialized_ = false;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DB_VOLDATA_UUID_STORE_H
