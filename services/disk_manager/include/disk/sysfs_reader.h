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
 
#ifndef OHOS_DISK_MANAGER_SYSFS_READER_H
#define OHOS_DISK_MANAGER_SYSFS_READER_H
 
#include <string>
 
namespace OHOS {
namespace DiskManager {
 
/**
 * USB设备sysfs属性信息，通过直读sysfs获取。
 * 非USB设备所有字段为空串。
 */
struct UsbSysfsInfo {
    std::string vid;           // idVendor，如 "0781"；非USB设备为空串
    std::string pid;           // idProduct，如 "5581"；非USB设备为空串
    std::string serialNumber;  // serial；无serial节点时为空串
    std::string busnum;        // busnum；非USB设备为空串
    std::string devnum;        // devnum；非USB设备为空串
};
 
/**
 * 从sysfs路径读取USB设备属性。
 *
 * 遍历策略：从block设备sysfs路径向上遍历，对每层：
 *   - subsystem: readlink {path}/subsystem 符号链接取basename（与systemd sd-device一致）
 *   - devtype:   读uevent文件解析 DEVTYPE= 行（DEVTYPE无独立属性文件，只能从uevent获取）
 * 以 subsystem=="usb" && devtype=="usb_device" 为精确判据确认USB设备节点，
 * 然后在该层级读取 idVendor/idProduct/serial/busnum/devnum。
 *
 * 权限：sysfs USB属性文件权限为0444，disk_manager进程可直接读取。
 */
class SysfsReader {
public:
    /**
     * 从uevent的sysPath读取USB设备信息。
     * @param sysPath  uevent中的env.sysPath，如 "/sys/devices/.../block/sda"
     * @param devName  uevent中的env.devName，如 "sda"
     * @return UsbSysfsInfo 读取到的USB设备属性；非USB设备返回全空结构体
     */
    static UsbSysfsInfo ReadUsbInfo(const std::string &sysPath, const std::string &devName);
 
private:
    /**
     * 从sysPath剥离 "/block/{devName}" 后缀，返回SCSI target层级路径。
     * 例如: "/sys/.../0:0:0:0/block/sda" → "/sys/.../0:0:0:0"
     * fallback: 若后缀不匹配，取dirname
     */
    static std::string StripBlockSuffix(const std::string &sysPath, const std::string &devName);
 
    /**
     * 读取单个sysfs属性文件内容（去除尾部空白/换行）。
     * 文件不存在或无法打开时返回空串。
     */
    static std::string ReadSysfsFile(const std::string &path);
 
    /**
     * 读取subsystem符号链接，返回最后一层目录名。
     * 与systemd sd-device.c:readlink_value()一致。
     * subsystem在sysfs中是符号链接：
     *   /sys/.../2-1/subsystem -> ../../bus/usb
     * 取最后一层 "usb" 作为subsystem值。
     *
     * @param sysfsDir  sysfs设备目录路径
     * @return subsystem名（如"usb"、"block"、"scsi"）；失败返回空串
     */
    static std::string ReadSubsystemSymlink(const std::string &sysfsDir);
 
    /**
     * 从uevent文件中解析指定字段的值。
     * 与systemd sd-device.c:device_read_uevent_file()一致。
     *
     * @param sysfsDir  sysfs设备目录路径
     * @param field     字段名，如 "DEVTYPE"
     * @return 字段值；不存在或读取失败返回空串
     */
    static std::string ReadUeventField(const std::string &sysfsDir, const std::string &field);
 
    /**
     * 检查目录是否有uevent文件（判断是否为合法设备节点）。
     * 与systemd sd-device.c:device_set_syspath()中的验证逻辑一致。
     */
    static bool HasUeventFile(const std::string &sysfsDir);
 
    /**
     * 从startPath向上遍历sysfs目录，查找 subsystem=="usb" && devtype=="usb_device"
     * 的节点，确认后在该节点读取 idVendor/idProduct/serial/busnum/devnum。
     */
    static UsbSysfsInfo WalkUpAndReadUsbAttrs(const std::string &startPath);
 
    static constexpr const char *BLOCK_DIR_PREFIX = "/block/";
};
 
} // namespace DiskManager
} // namespace OHOS
 
#endif // OHOS_DISK_MANAGER_SYSFS_READER_H