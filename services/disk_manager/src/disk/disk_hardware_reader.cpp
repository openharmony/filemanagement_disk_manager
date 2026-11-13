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

#include "disk_hardware_reader.h"
#include "disk_manager_hilog.h"
#include "disk_manager.h"
#include "block_info_table.h"
#include "disk.h"

#include <cerrno>
#include <climits>
#include <cstdint>
#include <dirent.h>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {

namespace {
constexpr const char *SYS_BLOCK_PATH = "/sys/block";
constexpr const char *NVME_PREFIX = "nvme";
constexpr const char *SD_PREFIX = "sd";
constexpr const char *DEVICE_MODEL_NODE = "/device/model";
constexpr const char *DEVICE_STATE_NODE = "/device/state";
constexpr const char *SIZE_NODE = "/size";
constexpr uint64_t SECTOR_SIZE = 512;
constexpr int32_t MEDIA_TYPE_SSD = 1;
constexpr int32_t MEDIA_TYPE_HDD = 2;
constexpr const char *SYSTEM_DISK_DEVNAME = "nvme0n1";

std::string TrimString(const std::string &s)
{
    auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}
} // namespace

std::string DiskHardwareReader::ReadSysfsNode(const std::string &path)
{
    if (path.empty() || path.size() >= PATH_MAX) {
        return "";
    }
    char realPath[PATH_MAX] = {0};
    if (realpath(path.c_str(), realPath) == nullptr) {
        LOGE("ReadSysfsNode: realpath failed, errno=%{public}d", errno);
        return "";
    }
    std::ifstream file(realPath);
    if (!file.is_open()) {
        return "";
    }
    std::string content;
    if (std::getline(file, content)) {
        file.close();
        return TrimString(content);
    }
    file.close();
    return "";
}

std::set<std::string> DiskHardwareReader::BuildWantedDeviceNames()
{
    std::set<std::string> wanted;
    // 系统盘：固定 nvme0n1
    wanted.insert(SYSTEM_DISK_DEVNAME);

    // 数据盘：DiskManager 已注册的 Disk 中，IsInternalDataDisk() 且 BlockInfoTable 能查到的
    std::vector<Disk> allDisks;
    DiskManager::GetInstance().GetAllDisks(allDisks);
    for (const auto &disk : allDisks) {
        if (!disk.IsInternalDataDisk()) {
            continue;
        }
        BlockInfo blockInfo {};
        if (!BlockInfoTable::GetInstance().TryCopyByDiskId(disk.GetDiskId(), blockInfo)) {
            continue;
        }
        if (!disk.GetDevName().empty()) {
            wanted.insert(disk.GetDevName());
        }
    }
    return wanted;
}

std::vector<LocalDiskInfo> DiskHardwareReader::GetLocalDiskInfos()
{
    std::vector<LocalDiskInfo> infos;
    const std::set<std::string> wanted = BuildWantedDeviceNames();
    DIR *dir = opendir(SYS_BLOCK_PATH);
    if (dir == nullptr) {
        LOGE("GetLocalDiskInfos: opendir %{public}s failed", SYS_BLOCK_PATH);
        return infos;
    }
    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string devName = entry->d_name;
        if (devName == "." || devName == "..") {
            continue;
        }
        bool isNvme = devName.find(NVME_PREFIX) == 0;
        bool isSd = devName.find(SD_PREFIX) == 0;
        if (!isNvme && !isSd) {
            continue;
        }
        // 仅保留白名单内的设备（系统盘 + 数据盘）
        if (wanted.find(devName) == wanted.end()) {
            continue;
        }
        std::string basePath = std::string(SYS_BLOCK_PATH) + "/" + devName;
        LocalDiskInfo info;
        info.SetSysPath(basePath);
        std::string model = ReadSysfsNode(basePath + DEVICE_MODEL_NODE);
        info.SetModel(model);
        info.SetDescription(model);
        info.SetState(ReadSysfsNode(basePath + DEVICE_STATE_NODE));
        std::string sizeStr = ReadSysfsNode(basePath + SIZE_NODE);
        if (!sizeStr.empty()) {
            unsigned long long sectors = 0;
            std::istringstream iss(sizeStr);
            iss >> sectors;
            if (sectors > 0 && sectors <= UINT64_MAX / SECTOR_SIZE) {
                info.SetSize(static_cast<int64_t>(sectors * SECTOR_SIZE));
            } else {
                LOGE("GetLocalDiskInfos: size overflow for %{public}s", devName.c_str());
                info.SetSize(0);
            }
        }
        if (isNvme) {
            info.SetMediaType(MEDIA_TYPE_SSD);
        } else {
            info.SetMediaType(MEDIA_TYPE_HDD);
        }
        infos.push_back(info);
        LOGI("GetLocalDiskInfos: found %{public}s, mediaType=%{public}d", devName.c_str(), info.GetMediaType());
    }
    closedir(dir);
    return infos;
}

} // namespace DiskManager
} // namespace OHOS
