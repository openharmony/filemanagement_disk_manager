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
#ifndef OHOS_FILEMANAGEMENT_DISK_MANAGER_EDM_ADAPTER_H
#define OHOS_FILEMANAGEMENT_DISK_MANAGER_EDM_ADAPTER_H

#include <cstdint>
#include <string>

#include "nocopyable.h"

namespace OHOS {
namespace DiskManager {

class EdmAdapter : public NoCopyable {
public:
    static EdmAdapter &GetInstance();

    bool IsEdmEnableOddBurn(const std::string &diskId, int32_t callerUserId);

private:
    EdmAdapter();
    ~EdmAdapter();

    bool IsExternalOddBurnAllowed(int32_t userId, const std::string &pid, const std::string &vid,
                                 const std::string &sn);
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_FILEMANAGEMENT_DISK_MANAGER_EDM_ADAPTER_H
