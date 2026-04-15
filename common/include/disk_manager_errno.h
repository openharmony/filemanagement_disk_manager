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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_ERRNO_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_ERRNO_H

#include <cstdint>

namespace OHOS {
namespace DiskManager {

// 与 storage_service 的 STORAGE_SERVICE_SYS_CAP_TAG(13600000) 错开；合入主线时请与平台错误码分配表对齐后固化。
constexpr int32_t DISK_MANAGER_SYS_CAP_TAG = 13610000;

// 本部件对外 int32_t 返回值语义与 IStorageManager 等保持一致时可与全局 E_OK(0) 混用。
enum DiskManagerErrNo : int32_t {
    E_OK = 0,
    DISK_MGR_ERR = -1,

    E_SA_IS_NULLPTR = 10,
    E_REMOTE_IS_NULLPTR = 11,
    E_SERVICE_IS_NULLPTR = 12,
    E_DEATH_RECIPIENT_IS_NULLPTR = 13,

    E_DISK_NOT_FOUND = 20,
    E_VOLUME_NOT_FOUND = 21,
    E_UEVENT_PARSE_FAILED = 22,
    E_DAEMON_IPC_FAILED = 23,
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_ERRNO_H
