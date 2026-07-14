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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_UTILS_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_UTILS_H

#include <climits>
#include <cinttypes>
#include <string>

#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {

constexpr size_t UEVENT_MAX_LEN = 2048;
constexpr size_t VOLUME_NAME_MAX_LEN = 64;

std::string GetAnonyString(const std::string &value);

bool IsFilePathInvalid(const std::string &filePath);
bool IsPureDigitsInRange(const std::string &s, size_t minLen, size_t maxLen);
bool IsMountPathValid(const std::string &mountPath);
bool IsVolumeIdValid(const std::string &volumeId);
bool IsDiskIdValid(const std::string &diskId);
bool IsUuidValid(const std::string &uuid);

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_UTILS_H
