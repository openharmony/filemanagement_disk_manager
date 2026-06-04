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

#ifndef OHOS_DISK_MANAGER_MOCK_USB_FUSE_ADAPTER_H
#define OHOS_DISK_MANAGER_MOCK_USB_FUSE_ADAPTER_H

#include <cstdint>
#include <string>

#include <gmock/gmock.h>

namespace OHOS {
namespace DiskManager {

class MockUsbFuseAdapter {
public:
    static MockUsbFuseAdapter &GetInstance();

    MOCK_METHOD(int32_t, NotifyUsbFuseMount, (int fuseFd, const std::string &volumeId, const std::string &fsUuid));
    MOCK_METHOD(int32_t, NotifyUsbFuseUmount, (const std::string &volumeId));
    MOCK_METHOD(bool, IsUsbFuseByType, (const std::string &fsType));
    MOCK_METHOD(bool, IsUsbFuseEnabledForFsType, (const std::string &fsType));

    static MockUsbFuseAdapter mockInstance_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_USB_FUSE_ADAPTER_H