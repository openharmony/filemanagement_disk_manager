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
#include <filesystem>
#include <fstream>

#include "voldata_uuid_store.h"
#include "disk_manager_errno.h"

namespace OHOS {
namespace DiskManager {

using namespace testing;
using namespace testing::ext;

namespace {
const std::string JSON_DIR = "/data/service/el1/public/disk_manager/";
const std::string JSON_FILE = JSON_DIR + "voldata_uuid_mapping.json";
const std::string JSON_TMP_FILE = JSON_FILE + ".tmp";

void RemoveTestFiles()
{
    std::error_code ec;
    std::filesystem::remove(JSON_FILE, ec);
    std::filesystem::remove(JSON_TMP_FILE, ec);
}

void WriteRawFile(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path, std::ios::trunc);
    ofs << content;
    ofs.close();
}
} // namespace

class VoldataUuidStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        std::filesystem::create_directories(JSON_DIR);
    }
    static void TearDownTestCase(void)
    {
        std::error_code ec;
        std::filesystem::remove_all(JSON_DIR, ec);
    }
    void SetUp() override
    {
        auto &store = VoldataUuidStore::GetInstance();
        store.UnInit();
        RemoveTestFiles();
        ASSERT_EQ(store.Init(), DiskManagerErrNo::E_OK);
    }
    void TearDown() override
    {
        VoldataUuidStore::GetInstance().UnInit();
        RemoveTestFiles();
    }
};

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_001, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j = "not_an_object";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_002, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["mountPath"] = "/mnt/data/voldata/data1";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_003, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "abc123";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_004, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = 42;
    j["mountPath"] = "/mnt/data/voldata/data1";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_005, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "abc123";
    j["mountPath"] = 123;
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_006, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "abc123";
    j["mountPath"] = "/wrong/path/data1";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_007, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "abc123";
    j["mountPath"] = "/mnt/data/voldata/data0";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_008, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "abc123";
    j["mountPath"] = "/mnt/data/voldata/data1a";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_009, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "bad..uuid";
    j["mountPath"] = "/mnt/data/voldata/data1";
    EXPECT_FALSE(entry.FromJson(j));
}

HWTEST_F(VoldataUuidStoreTest, FromJson_TestCase_010, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "valid-uuid-123";
    j["mountPath"] = "/mnt/data/voldata/data1";
    EXPECT_TRUE(entry.FromJson(j));
    EXPECT_EQ(entry.fsUuid, "valid-uuid-123");
    EXPECT_EQ(entry.mountPath, "/mnt/data/voldata/data1");
    EXPECT_EQ(entry.slotIndex, 1u);
}

HWTEST_F(VoldataUuidStoreTest, ToJson_TestCase_001, TestSize.Level0)
{
    VoldataUuidEntry entry;
    entry.fsUuid = "test-uuid";
    entry.mountPath = "/mnt/data/voldata/data5";
    entry.slotIndex = 5;
    nlohmann::json j = entry.ToJson();
    EXPECT_TRUE(j.is_object());
    EXPECT_EQ(j["fsUuid"].get<std::string>(), "test-uuid");
    EXPECT_EQ(j["mountPath"].get<std::string>(), "/mnt/data/voldata/data5");
    EXPECT_EQ(j["slotIndex"].get<uint32_t>(), 5u);
}

HWTEST_F(VoldataUuidStoreTest, Init_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.Init(), DiskManagerErrNo::E_OK);
}

HWTEST_F(VoldataUuidStoreTest, Init_TestCase_002, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    store.UnInit();
    RemoveTestFiles();
    EXPECT_EQ(store.Init(), DiskManagerErrNo::E_OK);
    EXPECT_TRUE(std::filesystem::exists(JSON_FILE));
}

HWTEST_F(VoldataUuidStoreTest, Init_TestCase_003, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string mountPath;
    bool created = false;
    EXPECT_EQ(store.ResolveMountPath("uuid-persist", mountPath, created), DiskManagerErrNo::E_OK);
    store.UnInit();
    EXPECT_EQ(store.Init(), DiskManagerErrNo::E_OK);
    std::string loadedPath;
    EXPECT_TRUE(store.TryGetMountPath("uuid-persist", loadedPath));
    EXPECT_EQ(loadedPath, mountPath);
}

HWTEST_F(VoldataUuidStoreTest, UnInit_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    store.UnInit();
    EXPECT_EQ(store.UnInit(), DiskManagerErrNo::E_OK);
}

HWTEST_F(VoldataUuidStoreTest, UnInit_TestCase_002, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.UnInit(), DiskManagerErrNo::E_OK);
    std::string path;
    EXPECT_FALSE(store.TryGetMountPath("any-uuid", path));
}

HWTEST_F(VoldataUuidStoreTest, ResolveMountPath_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path;
    bool created = false;
    EXPECT_EQ(store.ResolveMountPath("", path, created), DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, ResolveMountPath_TestCase_002, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path;
    bool created = false;
    EXPECT_EQ(store.ResolveMountPath("bad..uuid", path, created), DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, ResolveMountPath_TestCase_003, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path;
    bool created = false;
    EXPECT_EQ(store.ResolveMountPath("bad/uuid", path, created), DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, ResolveMountPath_TestCase_004, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path1;
    bool created1 = false;
    EXPECT_EQ(store.ResolveMountPath("uuid-reuse", path1, created1), DiskManagerErrNo::E_OK);
    EXPECT_TRUE(created1);
    std::string path2;
    bool created2 = false;
    EXPECT_EQ(store.ResolveMountPath("uuid-reuse", path2, created2), DiskManagerErrNo::E_OK);
    EXPECT_FALSE(created2);
    EXPECT_EQ(path2, path1);
}

HWTEST_F(VoldataUuidStoreTest, ResolveMountPath_TestCase_005, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path;
    bool created = false;
    EXPECT_EQ(store.ResolveMountPath("uuid-new", path, created), DiskManagerErrNo::E_OK);
    EXPECT_TRUE(created);
    EXPECT_EQ(path, "/mnt/data/voldata/data1");
}

HWTEST_F(VoldataUuidStoreTest, RemoveByFsUuid_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.RemoveByFsUuid(""), DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, RemoveByFsUuid_TestCase_002, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.RemoveByFsUuid("bad..uuid"), DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, RemoveByFsUuid_TestCase_003, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.RemoveByFsUuid("not-in-map-uuid"), DiskManagerErrNo::E_OK);
}

HWTEST_F(VoldataUuidStoreTest, RemoveByFsUuid_TestCase_004, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path;
    bool created = false;
    store.ResolveMountPath("uuid-remove", path, created);
    EXPECT_EQ(store.RemoveByFsUuid("uuid-remove"), DiskManagerErrNo::E_OK);
    std::string checkPath;
    EXPECT_FALSE(store.TryGetMountPath("uuid-remove", checkPath));
}

HWTEST_F(VoldataUuidStoreTest, ReplaceFsUuid_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.ReplaceFsUuid("", "new-uuid"), DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, ReplaceFsUuid_TestCase_002, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.ReplaceFsUuid("old-uuid", "bad..uuid"), DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, ReplaceFsUuid_TestCase_003, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.ReplaceFsUuid("same-uuid", "same-uuid"), DiskManagerErrNo::E_OK);
}

HWTEST_F(VoldataUuidStoreTest, ReplaceFsUuid_TestCase_004, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    EXPECT_EQ(store.ReplaceFsUuid("old-not-in-map", "new-uuid"), DiskManagerErrNo::E_OK);
}

HWTEST_F(VoldataUuidStoreTest, ReplaceFsUuid_TestCase_005, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path1, path2;
    bool c1 = false, c2 = false;
    store.ResolveMountPath("uuid-rp-old", path1, c1);
    store.ResolveMountPath("uuid-rp-new-exists", path2, c2);
    EXPECT_EQ(store.ReplaceFsUuid("uuid-rp-old", "uuid-rp-new-exists"),
              DiskManagerErrNo::DISK_MGR_ERR);
}

HWTEST_F(VoldataUuidStoreTest, ReplaceFsUuid_TestCase_006, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string originalPath;
    bool created = false;
    store.ResolveMountPath("uuid-rp-src", originalPath, created);
    EXPECT_EQ(store.ReplaceFsUuid("uuid-rp-src", "uuid-rp-dst"), DiskManagerErrNo::E_OK);
    std::string checkOld;
    EXPECT_FALSE(store.TryGetMountPath("uuid-rp-src", checkOld));
    std::string newPath;
    EXPECT_TRUE(store.TryGetMountPath("uuid-rp-dst", newPath));
    EXPECT_EQ(newPath, originalPath);
}

HWTEST_F(VoldataUuidStoreTest, TryGetMountPath_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path;
    EXPECT_FALSE(store.TryGetMountPath("", path));
}

HWTEST_F(VoldataUuidStoreTest, TryGetMountPath_TestCase_002, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string path;
    EXPECT_FALSE(store.TryGetMountPath("not-in-map", path));
}

HWTEST_F(VoldataUuidStoreTest, TryGetMountPath_TestCase_003, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string resolvePath;
    bool created = false;
    store.ResolveMountPath("uuid-lookup", resolvePath, created);
    std::string foundPath;
    EXPECT_TRUE(store.TryGetMountPath("uuid-lookup", foundPath));
    EXPECT_EQ(foundPath, resolvePath);
}

HWTEST_F(VoldataUuidStoreTest, SlotAllocation_Sequential_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string p1, p2, p3;
    bool c1 = false, c2 = false, c3 = false;
    store.ResolveMountPath("slot-a", p1, c1);
    store.ResolveMountPath("slot-b", p2, c2);
    store.ResolveMountPath("slot-c", p3, c3);
    EXPECT_EQ(p1, "/mnt/data/voldata/data1");
    EXPECT_EQ(p2, "/mnt/data/voldata/data2");
    EXPECT_EQ(p3, "/mnt/data/voldata/data3");
}

HWTEST_F(VoldataUuidStoreTest, SlotAllocation_ReuseFreed_TestCase_001, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    std::string p1, p2;
    bool c1 = false, c2 = false;
    store.ResolveMountPath("reuse-a", p1, c1);
    store.ResolveMountPath("reuse-b", p2, c2);
    EXPECT_EQ(p1, "/mnt/data/voldata/data1");
    store.RemoveByFsUuid("reuse-a");
    std::string p3;
    bool c3 = false;
    store.ResolveMountPath("reuse-c", p3, c3);
    EXPECT_EQ(p3, "/mnt/data/voldata/data1");
}

HWTEST_F(VoldataUuidStoreTest, SlotAllocation_HighSlot_TestCase_001, TestSize.Level0)
{
    VoldataUuidEntry entry;
    nlohmann::json j;
    j["fsUuid"] = "high-uuid";
    j["mountPath"] = "/mnt/data/voldata/data999";
    EXPECT_TRUE(entry.FromJson(j));
    EXPECT_EQ(entry.slotIndex, 999u);
}

HWTEST_F(VoldataUuidStoreTest, LoadFromFile_TestCase_001, TestSize.Level0)
{
    std::ifstream ifs(JSON_FILE);
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();
    nlohmann::json root = nlohmann::json::parse(content);
    EXPECT_TRUE(root.is_array());
    EXPECT_EQ(root.size(), 0u);
}

HWTEST_F(VoldataUuidStoreTest, LoadFromFile_TestCase_002, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    store.UnInit();
    RemoveTestFiles();
    nlohmann::json arr = nlohmann::json::array();
    nlohmann::json obj1;
    obj1["fsUuid"] = "pre-uuid-1";
    obj1["mountPath"] = "/mnt/data/voldata/data1";
    obj1["slotIndex"] = 1;
    nlohmann::json obj2;
    obj2["fsUuid"] = "pre-uuid-2";
    obj2["mountPath"] = "/mnt/data/voldata/data2";
    obj2["slotIndex"] = 2;
    arr.push_back(obj1);
    arr.push_back(obj2);
    WriteRawFile(JSON_FILE, arr.dump());
    EXPECT_EQ(store.Init(), DiskManagerErrNo::E_OK);
    std::string path1, path2;
    EXPECT_TRUE(store.TryGetMountPath("pre-uuid-1", path1));
    EXPECT_EQ(path1, "/mnt/data/voldata/data1");
    EXPECT_TRUE(store.TryGetMountPath("pre-uuid-2", path2));
    EXPECT_EQ(path2, "/mnt/data/voldata/data2");
}

HWTEST_F(VoldataUuidStoreTest, LoadFromFile_TestCase_003, TestSize.Level0)
{
    auto &store = VoldataUuidStore::GetInstance();
    store.UnInit();
    RemoveTestFiles();
    WriteRawFile(JSON_FILE, "not valid json {{{");
    EXPECT_EQ(store.Init(), DiskManagerErrNo::DISK_MGR_ERR);
}

} // namespace DiskManager
} // namespace OHOS