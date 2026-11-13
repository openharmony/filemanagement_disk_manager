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

#ifndef OHOS_DISK_MANAGER_SMART_READER_H
#define OHOS_DISK_MANAGER_SMART_READER_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace DiskManager {

/**
 * @brief 磁盘序列号读取工具。
 *
 * NVMe 序列号通过 sysfs /device/serial 读取（委托 DiskHardwareReader::ReadSysfsNode）。
 * HDD 序列号由 Provider 从 BlockInfoTable 获取（storage_daemon 已填充）。
 */
class SmartReader {
public:
    /**
     * 获取磁盘序列号。
     * NVMe: 读 sysfs /sys/block/{devName}/device/serial
     * HDD: 返回 "Unknown"（HDD 序列号由 Provider 从 BlockInfoTable 获取）
     * @param devName 设备名，如 nvme0n1、sda
     * @param isNvme 是否为 NVMe 设备
     * @return 序列号；读取失败返回 "Unknown"
     */
    static std::string GetSerialNumber(const std::string &devName, bool isNvme);

private:
    SmartReader() = delete;
    ~SmartReader() = delete;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_SMART_READER_H
