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

#include "pc_encryption_adapter.h"

#include <dlfcn.h>
#include <mutex>
#include <thread>
#include <vector>

#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {

namespace {
using FuncQueryEncStatus = int32_t (*)(const std::string &, int32_t &);
using FuncNotifyMounted = int32_t (*)(const std::string &, const std::string &, const std::string &);
constexpr const char *PC_ENCRYPTION_LIB_PATH = "/system/lib64/libpc_encryption_ext_volume_user_api.z.so";
constexpr const char *PC_ENC_QUERY_FUNC_NAME = "PC_ENC_EXT_QueryVolEncryptionStatus";
constexpr const char *PC_ENC_NOTIFY_MOUNTED_FUNC_NAME = "PC_ENC_EXT_ConfigureVolEncryptionPolicyOnMounted";
} // namespace

PcEncryptionAdapter &PcEncryptionAdapter::GetInstance()
{
    static PcEncryptionAdapter instance;
    return instance;
}

PcEncryptionAdapter::PcEncryptionAdapter()
{
    Init();
    LOGI("PcEncryptionAdapter created");
}

PcEncryptionAdapter::~PcEncryptionAdapter()
{
    std::vector<std::thread> toJoin;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        toJoin.swap(workers_);
    }
    for (auto &t : toJoin) {
        if (t.joinable()) {
            t.join();
        }
    }
    UnInit();
    LOGI("PcEncryptionAdapter destroyed");
}

void PcEncryptionAdapter::Init()
{
    handler_ = dlopen(PC_ENCRYPTION_LIB_PATH, RTLD_LAZY);
    if (handler_ == nullptr) {
        LOGE("PC encryption extension library not loaded: %{public}s", dlerror());
    }
}

void PcEncryptionAdapter::UnInit()
{
    if (handler_ != nullptr) {
        dlclose(handler_);
        handler_ = nullptr;
    }
}

bool PcEncryptionAdapter::QueryEncryptionStatus(const std::string &volPath, int32_t &encStatus)
{
    LOGI("QueryEncryptionStatus enter volPath=%{public}s", volPath.c_str());
    if (handler_ == nullptr) {
        LOGE("QueryEncryptionStatus: handler is nullptr");
        return false;
    }
    FuncQueryEncStatus func = reinterpret_cast<FuncQueryEncStatus>(
        dlsym(handler_, PC_ENC_QUERY_FUNC_NAME));
    if (func == nullptr) {
        LOGE("QueryEncryptionStatus: dlsym %{public}s failed, error: %{public}s",
             PC_ENC_QUERY_FUNC_NAME, dlerror());
        return false;
    }
    int32_t ret = func(volPath, encStatus);
    if (ret != 0) {
        LOGE("QueryEncryptionStatus: %{public}s returned %{public}d for path %{public}s",
             PC_ENC_QUERY_FUNC_NAME, ret, volPath.c_str());
        return false;
    }
    LOGI("QueryEncryptionStatus success encStatus=%{public}d", encStatus);
    return true;
}

void PcEncryptionAdapter::NotifyVolumeMounted(const std::string &diskId,
                                              const std::string &volumeId,
                                              const std::string &volPath)
{
    LOGI("NotifyVolumeMounted enter diskId=%{public}s volumeId=%{public}s volPath=%{public}s",
         diskId.c_str(), volumeId.c_str(), volPath.c_str());
    {
        std::lock_guard<std::mutex> lk(mutex_);
        workers_.emplace_back([this, diskId, volumeId, volPath]() {
            if (handler_ == nullptr) {
                LOGE("NotifyVolumeMounted: handler is nullptr");
                return;
            }
            FuncNotifyMounted func = reinterpret_cast<FuncNotifyMounted>(
                dlsym(handler_, PC_ENC_NOTIFY_MOUNTED_FUNC_NAME));
            if (func == nullptr) {
                LOGE("NotifyVolumeMounted: dlsym %{public}s failed, error: %{public}s",
                     PC_ENC_NOTIFY_MOUNTED_FUNC_NAME, dlerror());
                return;
            }
            int32_t ret = func(diskId, volumeId, volPath);
            LOGI("NotifyVolumeMounted: %{public}s returned %{public}d, diskId: %{public}s, volumeId: %{public}s",
                 PC_ENC_NOTIFY_MOUNTED_FUNC_NAME, ret, diskId.c_str(), volumeId.c_str());
        });
    }
    LOGI("NotifyVolumeMounted: async task started");
}

} // namespace DiskManager
} // namespace OHOS