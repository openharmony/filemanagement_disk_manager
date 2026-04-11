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

#ifndef DISK_MANAGER_NAPI_UTILS_H
#define DISK_MANAGER_NAPI_UTILS_H

#include <cstdint>

#include "disk_manager_hilog.h"
#include "napi/native_api.h"

#define FILEMGMT_CALL_BASE(theCall, retVal)              \
    do {                                                   \
        if ((theCall) != napi_ok) {                        \
            LOGE("napi call failed, theCall: %{public}s", #theCall); \
            return retVal;                                 \
        }                                                  \
    } while (0)
#define FILEMGMT_CALL(theCall) FILEMGMT_CALL_BASE(theCall, nullptr)

namespace OHOS {
namespace DiskManager {
bool IsSystemApp();
int32_t Convert2JsErrNum(int32_t errNum);
} // namespace DiskManager
} // namespace OHOS

#endif // DISK_MANAGER_NAPI_UTILS_H
