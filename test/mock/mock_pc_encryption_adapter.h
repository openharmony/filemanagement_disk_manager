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

#ifndef OHOS_DISK_MANAGER_MOCK_PC_ENCRYPTION_ADAPTER_H
#define OHOS_DISK_MANAGER_MOCK_PC_ENCRYPTION_ADAPTER_H

#include <string>

#include <gmock/gmock.h>

namespace OHOS {
namespace DiskManager {

class MockPcEncryptionAdapter {
public:
    static MockPcEncryptionAdapter &GetInstance();

    MOCK_METHOD(bool, QueryEncryptionStatus, (const std::string &volPath, int32_t &encStatus));
    MOCK_METHOD(void, NotifyVolumeMounted,
        (const std::string &diskId, const std::string &volumeId, const std::string &volPath));
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_PC_ENCRYPTION_ADAPTER_H