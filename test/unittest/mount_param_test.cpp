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

#include <gtest/gtest.h>

#include "mount_param.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class MountParamTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(MountParamTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    MountParam mp;
    EXPECT_FALSE(mp.GetReadOnly());
}

HWTEST_F(MountParamTest, ParameterizedConstructor_TestCase_001, TestSize.Level0)
{
    MountParam mp(true);
    EXPECT_TRUE(mp.GetReadOnly());
}

HWTEST_F(MountParamTest, ParameterizedConstructor_TestCase_002, TestSize.Level0)
{
    MountParam mp(false);
    EXPECT_FALSE(mp.GetReadOnly());
}

HWTEST_F(MountParamTest, SetReadOnly_TestCase_001, TestSize.Level0)
{
    MountParam mp;
    mp.SetReadOnly(true);
    EXPECT_TRUE(mp.GetReadOnly());
}

HWTEST_F(MountParamTest, SetReadOnly_TestCase_002, TestSize.Level0)
{
    MountParam mp(true);
    mp.SetReadOnly(false);
    EXPECT_FALSE(mp.GetReadOnly());
}

HWTEST_F(MountParamTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    MountParam mp(true);
    Parcel parcel;
    EXPECT_TRUE(mp.Marshalling(parcel));
}

HWTEST_F(MountParamTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    MountParam mp(true);
    Parcel parcel;
    EXPECT_TRUE(mp.Marshalling(parcel));
    MountParam *result = MountParam::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->GetReadOnly());
    delete result;
}

HWTEST_F(MountParamTest, Unmarshalling_Success_TestCase_002, TestSize.Level0)
{
    MountParam mp(false);
    Parcel parcel;
    EXPECT_TRUE(mp.Marshalling(parcel));
    MountParam *result = MountParam::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->GetReadOnly());
    delete result;
}

} // namespace DiskManager
} // namespace OHOS
