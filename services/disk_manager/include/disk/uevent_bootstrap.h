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

#ifndef OHOS_DISK_MANAGER_DISK_UEVENT_BOOTSTRAP_H
#define OHOS_DISK_MANAGER_DISK_UEVENT_BOOTSTRAP_H

#include "disk/uevent_env_parser.h"
#include "disk_config.h"

#include <list>
#include <mutex>
#include <string>

namespace OHOS {
namespace DiskManager {

/**
 * 解析块设备（disk）uevent，并驱动外置盘初始化：创建设备节点、读分区表、更新状态、按需挂载。
 * 需 root 的操作经 StorageDaemonAdapter 调用 IStorageDaemon（与 storage_daemon Netlink 处理链对齐）。
 */
class UeventBootstrap {
public:
    static int32_t OnBlockDiskUevent(const std::string &rawUeventMsg);
    static void Init();

private:
    static int32_t HandleDiskRemove(const UeventEnv &env);
    /** 首次出现 / 重新枚举：发布磁盘事件（若本事件首次见到该 diskId）。 */
    static int32_t HandleDiskAdd(const UeventEnv &env);
    /** DISK_EJECT_REQUEST 走 Eject；否则刷新分区与卷状态，不重复发布「新盘」事件。 */
    static int32_t HandleDiskChange(const UeventEnv &env);
    static int32_t DiscoverPartitionsAndVolumes(const UeventEnv &env, bool publishNewDiskEvent);
    static std::vector<std::string> SplitLine(std::string &line, std::string &token);
    static bool ParasConfig();
    static uint32_t MatchConfig(const UeventEnv &env);

private:
    static std::list<DiskConfig> diskConfigList_;
    static std::mutex diskConfigListMutex_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_DISK_UEVENT_BOOTSTRAP_H
