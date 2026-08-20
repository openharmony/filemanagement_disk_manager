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

#include "crypt_param.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

class CryptParamTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(CryptParamTest, DefaultConstructor_TestCase_001, TestSize.Level0)
{
    CryptParam cp;
    EXPECT_EQ(cp.GetPassPhrase(), "");
    EXPECT_EQ(cp.GetType(), "");
    EXPECT_EQ(cp.GetCipher(), "");
    EXPECT_EQ(cp.GetKeySize(), 0);
    EXPECT_EQ(cp.GetHash(), "");
}

HWTEST_F(CryptParamTest, ParameterizedConstructor_TestCase_001, TestSize.Level0)
{
    CryptParam cp("secret123", "luks", "aes", 256, "sha256");
    EXPECT_EQ(cp.GetPassPhrase(), "secret123");
    EXPECT_EQ(cp.GetType(), "luks");
    EXPECT_EQ(cp.GetCipher(), "aes");
    EXPECT_EQ(cp.GetKeySize(), 256);
    EXPECT_EQ(cp.GetHash(), "sha256");
}

HWTEST_F(CryptParamTest, SetPassPhrase_TestCase_001, TestSize.Level0)
{
    CryptParam cp;
    cp.SetPassPhrase("new-secret");
    EXPECT_EQ(cp.GetPassPhrase(), "new-secret");
}

HWTEST_F(CryptParamTest, SetType_TestCase_001, TestSize.Level0)
{
    CryptParam cp;
    cp.SetType("luks2");
    EXPECT_EQ(cp.GetType(), "luks2");
}

HWTEST_F(CryptParamTest, SetCipher_TestCase_001, TestSize.Level0)
{
    CryptParam cp;
    cp.SetCipher("aes-xts-plain64");
    EXPECT_EQ(cp.GetCipher(), "aes-xts-plain64");
}

HWTEST_F(CryptParamTest, SetKeySize_TestCase_001, TestSize.Level0)
{
    CryptParam cp;
    cp.SetKeySize(512);
    EXPECT_EQ(cp.GetKeySize(), 512);
}

HWTEST_F(CryptParamTest, SetHash_TestCase_001, TestSize.Level0)
{
    CryptParam cp;
    cp.SetHash("sha512");
    EXPECT_EQ(cp.GetHash(), "sha512");
}

HWTEST_F(CryptParamTest, Marshalling_Success_TestCase_001, TestSize.Level0)
{
    CryptParam cp("secret123", "luks", "aes", 256, "sha256");
    Parcel parcel;
    EXPECT_TRUE(cp.Marshalling(parcel));
}

HWTEST_F(CryptParamTest, Unmarshalling_Success_TestCase_001, TestSize.Level0)
{
    CryptParam cp("secret123", "luks", "aes", 256, "sha256");
    Parcel parcel;
    EXPECT_TRUE(cp.Marshalling(parcel));
    CryptParam *result = CryptParam::Unmarshalling(parcel);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetPassPhrase(), "secret123");
    EXPECT_EQ(result->GetType(), "luks");
    EXPECT_EQ(result->GetCipher(), "aes");
    EXPECT_EQ(result->GetKeySize(), 256);
    EXPECT_EQ(result->GetHash(), "sha256");
    delete result;
}

HWTEST_F(CryptParamTest, Unmarshalling_OversizedPassPhrase_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString(std::string(4097, 'a'));
    parcel.WriteString("luks");
    parcel.WriteString("aes");
    parcel.WriteInt32(256);
    parcel.WriteString("sha256");
    CryptParam *result = CryptParam::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(CryptParamTest, Unmarshalling_OversizedType_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("secret123");
    parcel.WriteString(std::string(4097, 'b'));
    parcel.WriteString("aes");
    parcel.WriteInt32(256);
    parcel.WriteString("sha256");
    CryptParam *result = CryptParam::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(CryptParamTest, Unmarshalling_OversizedCipher_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("secret123");
    parcel.WriteString("luks");
    parcel.WriteString(std::string(4097, 'c'));
    parcel.WriteInt32(256);
    parcel.WriteString("sha256");
    CryptParam *result = CryptParam::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(CryptParamTest, Unmarshalling_OversizedHash_TestCase_001, TestSize.Level0)
{
    Parcel parcel;
    parcel.WriteString("secret123");
    parcel.WriteString("luks");
    parcel.WriteString("aes");
    parcel.WriteInt32(256);
    parcel.WriteString(std::string(4097, 'd'));
    CryptParam *result = CryptParam::Unmarshalling(parcel);
    EXPECT_EQ(result, nullptr);
}

} // namespace DiskManager
} // namespace OHOS
