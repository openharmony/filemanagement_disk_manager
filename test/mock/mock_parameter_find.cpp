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

#include "mock_parameter_find.h"
#include <cstring>
#include <securec.h>

int g_mockFindParameterResult = 1;
uint32_t g_mockGetParameterValueResult = 5;
char g_mockParameterValue[MOCK_PARAMETER_VALUE_MAX_LEN] = "false";

extern "C" {
int MockFindParameter(const char *key)
{
    return g_mockFindParameterResult;
}

uint32_t MockGetParameterValue(int handle, char *value, uint32_t len)
{
    if (value == nullptr || len == 0) {
        return 0;
    }
    uint32_t copyLen = strlen(g_mockParameterValue);
    if (copyLen >= len) {
        copyLen = len - 1;
    }
    (void)memcpy_s(value, len, g_mockParameterValue, copyLen);
    value[copyLen] = '\0';
    return g_mockGetParameterValueResult;
}
}