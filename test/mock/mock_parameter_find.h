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

#ifndef OHOS_DISK_MANAGER_MOCK_PARAMETER_FIND_H
#define OHOS_DISK_MANAGER_MOCK_PARAMETER_FIND_H

#include <cstdint>

constexpr uint32_t MOCK_PARAMETER_VALUE_MAX_LEN = 32;

extern int g_mockFindParameterResult;
extern uint32_t g_mockGetParameterValueResult;
extern char g_mockParameterValue[MOCK_PARAMETER_VALUE_MAX_LEN];

extern "C" {
int MockFindParameter(const char *key);
uint32_t MockGetParameterValue(int handle, char *value, uint32_t len);
}

#endif // OHOS_DISK_MANAGER_MOCK_PARAMETER_FIND_H