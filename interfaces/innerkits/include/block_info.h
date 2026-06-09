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

#ifndef OHOS_DISK_MANAGER_BLOCK_INFO_H
#define OHOS_DISK_MANAGER_BLOCK_INFO_H

#include "parcel.h"

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {
using json = nlohmann::json;

/**
 * 块设备详细信息（进程内/解析用）。
 */
struct BlockInfo {
    uint64_t sizeBytes;
    std::string vendor;
    std::string model;
    std::string interfaceType;
    uint32_t rpm;
    bool removable;
    std::string serialNumber;
    std::string diskId;
    std::string devicePath;
    std::string port;
    std::string devnum;
    std::string busnum;
    std::string devNode;
    std::string scsiBusNum;
    std::string fwVersion;
    nlohmann::json ODD_INFO;

    json ToJson() const
    {
        return json{{"sizeBytes", sizeBytes},
                    {"vendor", vendor},
                    {"model", model},
                    {"devnum", devnum},
                    {"busnum", busnum},
                    {"devNode", devNode},
                    {"scsiBusNum", scsiBusNum},
                    {"fwVersion", fwVersion},
                    {"ODD_INFO", ODD_INFO},
                    {"interfaceType", interfaceType},
                    {"rpm", rpm},
                    {"removable", removable},
                    {"serialNumber", serialNumber},
                    {"diskId", diskId},
                    {"devicePath", devicePath},
                    {"port", port}};
    }

    static std::string SerializeVector(const std::vector<BlockInfo> &infos)
    {
        json j = json::array();
        for (const auto &info : infos) {
            j.push_back(info.ToJson());
        }
        return j.dump();
    }
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_BLOCK_INFO_H