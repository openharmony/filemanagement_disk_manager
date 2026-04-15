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

#include "disk/uevent_env_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string_view>

namespace OHOS {
namespace DiskManager {

namespace {

constexpr uint32_t KeyHash(std::string_view sv)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < sv.size(); ++i) {
        h ^= static_cast<uint32_t>(static_cast<unsigned char>(sv[i]));
        h *= 16777619u;
    }
    return h;
}

inline uint32_t KeyHashRuntime(const std::string &key)
{
    return KeyHash(std::string_view(key.data(), key.size()));
}

} // namespace

bool UeventEnv::IsBlockDiskEvent() const
{
    return subsystem == "block" && devType == "disk";
}

void UeventEnvParser::ToLower(std::string &s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

namespace {

void ApplyEnvValue(UeventEnv &out, uint32_t keyHash, const std::string &val)
{
    switch (keyHash) {
        case KeyHash("action"):
            out.action = val;
            UeventEnvParser::ToLower(out.action);
            break;
        case KeyHash("subsystem"):
            out.subsystem = val;
            UeventEnvParser::ToLower(out.subsystem);
            break;
        case KeyHash("devtype"):
            out.devType = val;
            UeventEnvParser::ToLower(out.devType);
            break;
        case KeyHash("major"):
            out.major = static_cast<unsigned int>(strtoul(val.c_str(), nullptr, 10));
            break;
        case KeyHash("minor"):
            out.minor = static_cast<unsigned int>(strtoul(val.c_str(), nullptr, 10));
            break;
        case KeyHash("devpath"):
            out.devPath = val;
            out.sysPath = "/sys" + val;
            break;
        case KeyHash("devname"):
            out.devName = val;
            break;
        case KeyHash("disk_eject_request"):
            out.ejectRequest = (val == "1");
            break;
        default:
            break;
    }
}

} // namespace

bool UeventEnvParser::Parse(const std::string &raw, UeventEnv &out)
{
    out = UeventEnv{};
    std::stringstream ss(raw);
    std::string line;
    while (std::getline(ss, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = line.substr(0, pos);
        const std::string val = line.substr(pos + 1);
        ToLower(key);
        ApplyEnvValue(out, KeyHashRuntime(key), val);
    }
    if (out.action.empty() || out.subsystem.empty() || out.devType.empty()) {
        return false;
    }
    return true;
}

} // namespace DiskManager
} // namespace OHOS
