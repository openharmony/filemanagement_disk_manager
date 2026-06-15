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

#include "mock_dlfcn.h"

extern "C" void *MockDlopen(const char *filename, int flag)
{
    return OHOS::DiskManager::MockDlfcnConfig::GetInstance().GetDlopenResult();
}

extern "C" void *MockDlsym(void *handle, const char *symbol)
{
    return OHOS::DiskManager::MockDlfcnConfig::GetInstance().GetDlsymResultVoid(std::string(symbol));
}

extern "C" int MockDlclose(void *handle)
{
    return 0;
}

extern "C" char *MockDlerror()
{
    return const_cast<char *>("mock dlfcn error");
}

extern "C" int32_t MockNotifyExternalVolumeFuseMount(int fuseFd, const std::string &volumeId, const std::string &fsUuid)
{
    return OHOS::DiskManager::MockDlfcnConfig::GetInstance().GetMountResult();
}

extern "C" int32_t MockNotifyExternalVolumeFuseUmount(const std::string &volumeId)
{
    return OHOS::DiskManager::MockDlfcnConfig::GetInstance().GetUMountResult();
}

extern "C" int32_t MockIsUsbFuseByTypeFunc(const std::string &fsType, bool &enabled)
{
    enabled = OHOS::DiskManager::MockDlfcnConfig::GetInstance().GetUsbFuseByTypeEnabled();
    return OHOS::DiskManager::MockDlfcnConfig::GetInstance().GetUsbFuseByTypeResult();
}