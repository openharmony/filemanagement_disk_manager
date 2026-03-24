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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_HILOG_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_HILOG_H

#include "hilog/log.h"

#ifndef LOG_DOMAIN
#define LOG_DOMAIN 0xD004400
#endif
#ifndef DISK_MANAGER_LOG_TAG
#define DISK_MANAGER_LOG_TAG "DiskManager"
#endif

#define LOGI(fmt, ...)                                                                               \
    ((void)HILOG_IMPL(LOG_CORE, LOG_INFO, LOG_DOMAIN, DISK_MANAGER_LOG_TAG,                         \
                      "[%{public}s:%{public}d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define LOGW(fmt, ...)                                                                               \
    ((void)HILOG_IMPL(LOG_CORE, LOG_WARN, LOG_DOMAIN, DISK_MANAGER_LOG_TAG,                         \
                      "[%{public}s:%{public}d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define LOGE(fmt, ...)                                                                               \
    ((void)HILOG_IMPL(LOG_CORE, LOG_ERROR, LOG_DOMAIN, DISK_MANAGER_LOG_TAG,                        \
                      "[%{public}s:%{public}d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define LOGF(fmt, ...)                                                                               \
    ((void)HILOG_IMPL(LOG_CORE, LOG_FATAL, LOG_DOMAIN, DISK_MANAGER_LOG_TAG,                        \
                      "[%{public}s:%{public}d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define LOGD(fmt, ...)                                                                               \
    ((void)HILOG_IMPL(LOG_CORE, LOG_DEBUG, LOG_DOMAIN, DISK_MANAGER_LOG_TAG,                        \
                      "[%{public}s:%{public}d] " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_HILOG_H
