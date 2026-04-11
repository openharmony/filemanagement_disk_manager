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

#ifndef OHOS_STORAGEDAEMON_STORAGE_DAEMON_PROXY_H
#define OHOS_STORAGEDAEMON_STORAGE_DAEMON_PROXY_H

#include "iremote_proxy.h"
#include "istorage_daemon.h"

namespace OHOS {
namespace StorageDaemon {

class StorageDaemonProxy : public IRemoteProxy<IStorageDaemon> {
public:
    explicit StorageDaemonProxy(const sptr<IRemoteObject> &impl);
    ~StorageDaemonProxy() = default;

    ErrCode Mount(const std::string &volId, uint32_t flags) override;
    ErrCode UMount(const std::string &volId) override;
    ErrCode Check(const std::string &volId) override;
    ErrCode Format(const std::string &volId, const std::string &fsType) override;
    ErrCode Partition(const std::string &diskId, int32_t type) override;
    ErrCode SetVolumeDescription(const std::string &volId, const std::string &description) override;
    ErrCode QueryUsbIsInUse(const std::string &diskPath, bool &isInUse) override;
    ErrCode TryToFix(const std::string &volId, uint32_t flags) override;
    ErrCode MountUsbFuse(const std::string &volumeId, std::string &fsUuid, int &fuseFd) override;

private:
    static inline BrokerDelegator<StorageDaemonProxy> delegator_;
};

} // namespace StorageDaemon
} // namespace OHOS

#endif // OHOS_STORAGEDAEMON_STORAGE_DAEMON_PROXY_H
