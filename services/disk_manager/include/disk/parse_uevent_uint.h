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

#ifndef OHOS_DISK_MANAGER_DISK_PARSE_UEVENT_UINT_H
#define OHOS_DISK_MANAGER_DISK_PARSE_UEVENT_UINT_H

#include <charconv>
#include <cstdint>
#include <string>
#include <system_error>

namespace OHOS {
namespace DiskManager {
inline bool ParseUeventUint(const std::string &text, unsigned int &out)
{
    if (text.empty()) {
        return false;
    }
    unsigned int value = 0;
    const char *first = text.data();
    const char *last = first + text.size();
    auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc() || result.ptr != last) {
        return false;
    }
    out = value;
    return true;
}
} // namespace DiskManager
} // namespace OHOS
#endif // OHOS_DISK_MANAGER_DISK_PARSE_UEVENT_UINT_H
