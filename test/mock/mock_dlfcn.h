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

#ifndef OHOS_DISK_MANAGER_MOCK_DLFCN_H
#define OHOS_DISK_MANAGER_MOCK_DLFCN_H

#include <cstdint>
#include <string>
#include <unordered_map>

extern "C" void *MockDlopen(const char *filename, int flag);
extern "C" void *MockDlsym(void *handle, const char *symbol);
extern "C" int MockDlclose(void *handle);
extern "C" char *MockDlerror();

extern "C" int32_t MockNotifyExternalVolumeFuseMount(int fuseFd, const std::string &volumeId,
                                                     const std::string &fsUuid);
extern "C" int32_t MockNotifyExternalVolumeFuseUmount(const std::string &volumeId);
extern "C" int32_t MockIsUsbFuseByTypeFunc(const std::string &fsType, bool &enabled);

namespace OHOS {
namespace DiskManager {

class MockDlfcnConfig {
public:
    static MockDlfcnConfig &GetInstance()
    {
        static MockDlfcnConfig *instance = new MockDlfcnConfig();
        return *instance;
    }

    void SetDlopenResult(void *result) { dlopenResult_ = result; }
    void *GetDlopenResult() { return dlopenResult_; }

    void SetDlsymResult(const std::string &symbol, uintptr_t funcAddr)
    {
        dlsymResults_[symbol] = funcAddr;
    }
    uintptr_t GetDlsymResult(const std::string &symbol)
    {
        auto it = dlsymResults_.find(symbol);
        if (it != dlsymResults_.end()) {
            return it->second;
        }
        return 0;
    }
    void ClearDlsymResults() { dlsymResults_.clear(); }

    void *GetDlsymResultVoid(const std::string &symbol)
    {
        uintptr_t addr = GetDlsymResult(symbol);
        if (addr == 0) {
            return nullptr;
        }
        return reinterpret_cast<void *>(addr);
    }

    void SetMountResult(int32_t result) { mountResult_ = result; }
    int32_t GetMountResult() { return mountResult_; }

    void SetUMountResult(int32_t result) { uMountResult_ = result; }
    int32_t GetUMountResult() { return uMountResult_; }

    void SetUsbFuseByTypeResult(int32_t result, bool enabled)
    {
        usbFuseByTypeResult_ = result;
        usbFuseByTypeEnabled_ = enabled;
    }
    int32_t GetUsbFuseByTypeResult() { return usbFuseByTypeResult_; }
    bool GetUsbFuseByTypeEnabled() { return usbFuseByTypeEnabled_; }

private:
    MockDlfcnConfig() = default;
    void *dlopenResult_ = nullptr;
    std::unordered_map<std::string, uintptr_t> dlsymResults_;
    int32_t mountResult_ = 0;
    int32_t uMountResult_ = 0;
    int32_t usbFuseByTypeResult_ = 0;
    bool usbFuseByTypeEnabled_ = true;
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_DLFCN_H