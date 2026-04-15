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

#ifndef OHOS_DISK_MANAGER_DISK_UEVENT_ENV_PARSER_H
#define OHOS_DISK_MANAGER_DISK_UEVENT_ENV_PARSER_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace DiskManager {

struct UeventEnv {
    std::string action;
    std::string subsystem;
    std::string devType;
    unsigned int major = 0;
    unsigned int minor = 0;
    std::string devPath;
    std::string sysPath;
    /** DEVNAME（如 sda、mmcblk0），与内核 / storage_daemon NetlinkData 一致 */
    std::string devName;
    /** DISK_EJECT_REQUEST（小写键 disk_eject_request）：内核块层值为 "1" 时表示弹出请求。 */
    bool ejectRequest = false;

    bool IsBlockDiskEvent() const;
};

class UeventEnvParser {
public:
    /**
     * 解析 storage_daemon 转发的 uevent 文本。
     * 推荐格式：与内核 netlink 一致的 "KEY=value" 序列，可用 '\\n' 串联（避免 IDL String 无法携带 \\0）。
     */
    static bool Parse(const std::string &raw, UeventEnv &out);
    static void ToLower(std::string &s);
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_UEVENT_ENV_PARSER_H
