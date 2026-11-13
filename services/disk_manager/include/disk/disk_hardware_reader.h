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

#ifndef OHOS_DISK_MANAGER_DISK_HARDWARE_READER_H
#define OHOS_DISK_MANAGER_DISK_HARDWARE_READER_H

#include "local_disk_info.h"

#include <set>
#include <string>
#include <vector>

namespace OHOS {
namespace DiskManager {

/**
 * @brief 本地磁盘硬件信息读取工具，通过直读 sysfs 获取磁盘基础属性。
 *
 * 遍历 /sys/block 目录，读取 nvme* 和 sd* 前缀设备的 model、state、size 等属性。
 * 仅返回本地磁盘（含系统盘 nvme0n1），不返回外置存储设备。
 */
class DiskHardwareReader {
public:
    /**
     * 读取单个 sysfs 属性文件内容（去除首尾空白）。
     * 文件不存在或无法打开时返回空串。
     * @param path sysfs 文件路径，如 /sys/block/nvme0n1/device/model
     * @return 文件内容（已 trim）；失败返回空串
     */
    static std::string ReadSysfsNode(const std::string &path);

    /**
     * 扫描 /sys/block 目录，获取所有本地磁盘的硬件信息。
     * 保留 nvme* 和 sd* 前缀设备（含系统盘 nvme0n1）。
     * @return 本地磁盘硬件信息列表
     */
    static std::vector<LocalDiskInfo> GetLocalDiskInfos();

    /**
     * 构建需要采集的设备名白名单：系统盘(nvme0n1) + 数据盘(DiskManager 已注册且
     * IsInternalDataDisk() 为 true 且 BlockInfoTable 能查到的盘)。
     * @return 设备名集合（如 {"nvme0n1", "sda"}）
     */
    static std::set<std::string> BuildWantedDeviceNames();

private:
    DiskHardwareReader() = delete;
    ~DiskHardwareReader() = delete;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_HARDWARE_READER_H
