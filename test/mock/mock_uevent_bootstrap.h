#ifndef OHOS_DISK_MANAGER_MOCK_UEVENT_BOOTSTRAP_H
#define OHOS_DISK_MANAGER_MOCK_UEVENT_BOOTSTRAP_H

#include <cstdint>
#include <string>

#include <gmock/gmock.h>

#include "uevent_env_parser.h"

namespace OHOS {
namespace DiskManager {

class MockUeventBootstrap {
public:
    static MockUeventBootstrap mockInstance_;
    static MockUeventBootstrap &GetInstance() { return mockInstance_; }

    static int32_t OnBlockDiskUevent(const std::string &rawUeventMsg)
    {
        return mockInstance_.OnBlockDiskUeventImpl(rawUeventMsg);
    }
    static void Init() { mockInstance_.InitImpl(); }
    static uint32_t MatchConfig(const UeventEnv &env) { return mockInstance_.MatchConfigImpl(env); }
    static int32_t RediscoverDiskVolumes(const std::string &diskId)
    {
        return mockInstance_.RediscoverDiskVolumesImpl(diskId);
    }

    MOCK_METHOD(int32_t, OnBlockDiskUeventImpl, (const std::string &rawUeventMsg));
    MOCK_METHOD(void, InitImpl, ());
    MOCK_METHOD(uint32_t, MatchConfigImpl, (const UeventEnv &env));
    MOCK_METHOD(int32_t, RediscoverDiskVolumesImpl, (const std::string &diskId));
};

} // namespace DiskManager
} // namespace OHOS

#endif // OHOS_DISK_MANAGER_MOCK_UEVENT_BOOTSTRAP_H