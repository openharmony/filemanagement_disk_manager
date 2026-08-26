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
 
#include "sysfs_reader.h"
 
#include "disk_manager_hilog.h"
 
#include <fstream>
#include <string>
#include <unistd.h>
 
namespace OHOS {
namespace DiskManager {
 
std::string SysfsReader::StripBlockSuffix(const std::string &sysPath, const std::string &devName)
{
    // sysPath 形如: /sys/.../0:0:0:0/block/sda
    // 需去掉末尾 "/block/sda" 得到: /sys/.../0:0:0:0
    std::string suffix = std::string(BLOCK_DIR_PREFIX) + devName;
    if (sysPath.size() > suffix.size() &&
        sysPath.compare(sysPath.size() - suffix.size(), suffix.size(), suffix) == 0) {
        return sysPath.substr(0, sysPath.size() - suffix.size());
    }
    // fallback: NVMe等设备路径无 /block/ 段，直接取dirname
    auto lastSlash = sysPath.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash > 0) {
        return sysPath.substr(0, lastSlash);
    }
    return sysPath;
}
 
std::string SysfsReader::ReadSysfsFile(const std::string &path)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return "";
    }
    std::string content;
    std::getline(ifs, content);
    // 去除尾部空白/换行
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r' ||
           content.back() == ' ' || content.back() == '\0')) {
        content.pop_back();
    }
    return content;
}
 
std::string SysfsReader::ReadSubsystemSymlink(const std::string &sysfsDir)
{
    // subsystem 在 sysfs 中是符号链接:
    //   /sys/.../2-1/subsystem -> ../../bus/usb
    // 取最后一层 "usb" 作为 subsystem 值
    // 与 systemd sd-device.c:readlink_value() 一致
    std::string linkPath = sysfsDir + "/subsystem";
    char target[256] = {};
    ssize_t n = readlink(linkPath.c_str(), target, sizeof(target) - 1);
    if (n <= 0) {
        return "";
    }
    target[n] = '\0';
 
    // 取最后一层目录名: ../../bus/usb → usb
    std::string targetStr(target);
    auto lastSlash = targetStr.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash + 1 < targetStr.size()) {
        return targetStr.substr(lastSlash + 1);
    }
    return targetStr;
}
 
std::string SysfsReader::ReadUeventField(const std::string &sysfsDir, const std::string &field)
{
    // 从 uevent 文件中解析指定字段的值
    // 与 systemd sd-device.c:device_read_uevent_file() + handle_uevent_line() 一致
    std::string ueventPath = sysfsDir + "/uevent";
    std::ifstream ifs(ueventPath);
    if (!ifs.is_open()) {
        return "";
    }
 
    std::string prefix = field + "=";
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.compare(0, prefix.size(), prefix) == 0) {
            return line.substr(prefix.size());
        }
    }
    return "";
}
 
bool SysfsReader::HasUeventFile(const std::string &sysfsDir)
{
    // 在 sysfs 中，只有包含 uevent 文件的目录才是有效的 kobject 设备节点
    // 与 systemd sd-device.c:device_set_syspath() 中的验证逻辑一致
    std::string ueventPath = sysfsDir + "/uevent";
    return access(ueventPath.c_str(), F_OK) == 0;
}
 
UsbSysfsInfo SysfsReader::WalkUpAndReadUsbAttrs(const std::string &startPath)
{
    UsbSysfsInfo info;
    std::string currentPath = startPath;
 
    while (!currentPath.empty() && currentPath != "/") {
        // 跳过非设备节点目录（没有uevent文件的目录不是合法kobject）
        if (!HasUeventFile(currentPath)) {
            auto lastSlash = currentPath.find_last_of('/');
            if (lastSlash == std::string::npos || lastSlash == 0) {
                break;
            }
            currentPath = currentPath.substr(0, lastSlash);
            continue;
        }
 
        // 读取 subsystem: readlink {path}/subsystem 取 basename
        // 与 systemd sd-device 一致，uevent 文件中 SUBSYSTEM= 行不可靠
        std::string subsystem = ReadSubsystemSymlink(currentPath);
        if (subsystem != "usb") {
            auto lastSlash = currentPath.find_last_of('/');
            if (lastSlash == std::string::npos || lastSlash == 0) {
                break;
            }
            currentPath = currentPath.substr(0, lastSlash);
            continue;
        }
 
        // 读取 devtype: 从 uevent 文件解析 DEVTYPE=
        // DEVTYPE 无独立属性文件，只能从 uevent 获取
        std::string devType = ReadUeventField(currentPath, "DEVTYPE");
        if (devType != "usb_device") {
            // SUBSYSTEM=usb 但 DEVTYPE!=usb_device，如 usb_interface 或 usb_host
            auto lastSlash = currentPath.find_last_of('/');
            if (lastSlash == std::string::npos || lastSlash == 0) {
                break;
            }
            currentPath = currentPath.substr(0, lastSlash);
            continue;
        }
 
        // 精确确认：当前目录是 USB 设备节点 (subsystem=usb, devtype=usb_device)
        LOGI("SysfsReader: confirmed usb_device at %{public}s", currentPath.c_str());
 
        info.vid = ReadSysfsFile(currentPath + "/idVendor");
        info.pid = ReadSysfsFile(currentPath + "/idProduct");
        info.serialNumber = ReadSysfsFile(currentPath + "/serial");
        info.busnum = ReadSysfsFile(currentPath + "/busnum");
        info.devnum = ReadSysfsFile(currentPath + "/devnum");
 
        LOGI("SysfsReader: vid=%{public}s pid=%{public}s sn=%{public}s "
             "busnum=%{public}s devnum=%{public}s",
             info.vid.c_str(), info.pid.c_str(), info.serialNumber.c_str(),
             info.busnum.c_str(), info.devnum.c_str());
 
        // 找到usb_device节点即停止，不再向上遍历，避免误读Hub层级属性
        break;
    }
 
    if (info.vid.empty()) {
        LOGI("SysfsReader: no usb_device found in path ancestry");
    }
 
    return info;
}
 
UsbSysfsInfo SysfsReader::ReadUsbInfo(const std::string &sysPath, const std::string &devName)
{
    LOGI("SysfsReader::ReadUsbInfo sysPath=%{public}s devName=%{public}s",
         sysPath.c_str(), devName.c_str());
 
    if (sysPath.empty() || devName.empty()) {
        LOGW("SysfsReader: empty sysPath or devName");
        return {};
    }
 
    // 从sysPath去掉 /block/{devName} 后缀，得到SCSI target路径
    std::string walkStart = StripBlockSuffix(sysPath, devName);
    LOGI("SysfsReader: walkStart=%{public}s", walkStart.c_str());
 
    return WalkUpAndReadUsbAttrs(walkStart);
}
 
} // namespace DiskManager
} // namespace OHOS