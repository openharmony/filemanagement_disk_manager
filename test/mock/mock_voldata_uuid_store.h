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

#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_MOCK_VOLDATA_UUID_STORE_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_MOCK_VOLDATA_UUID_STORE_H

#include <gmock/gmock.h>
#include <string>
#include <cstdint>

namespace OHOS {
namespace DiskManager {

class MockVoldataUuidStore {
public:
    static MockVoldataUuidStore &GetInstance();

    MOCK_METHOD(int32_t, Init, ());
    MOCK_METHOD(int32_t, UnInit, ());
    MOCK_METHOD(int32_t, ResolveMountPath, (const std::string &fsUuid, std::string &outMountPath, bool &outCreated));
    MOCK_METHOD(int32_t, RemoveByFsUuid, (const std::string &fsUuid));
    MOCK_METHOD(int32_t, ReplaceFsUuid, (const std::string &oldFsUuid, const std::string &newFsUuid));
    MOCK_METHOD2(TryGetMountPath, bool(const std::string &fsUuid, std::string &outMountPath));

    static MockVoldataUuidStore *mockInstance_;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_MOCK_VOLDATA_UUID_STORE_H