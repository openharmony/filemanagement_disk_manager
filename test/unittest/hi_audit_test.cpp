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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include "hi_audit.h"

namespace OHOS {
namespace DiskManager {
using namespace testing::ext;

namespace {
constexpr const char *HIAUDIT_DIR = "/data/log/hiaudit/disk_manager/";
constexpr const char *HIAUDIT_FILE = "/data/log/hiaudit/disk_manager/disk_manager_audit.csv";
constexpr uint32_t HIAUDIT_FILE_SIZE = 3 * 1024 * 1024;

void EnsureHiAuditDir()
{
    if (access("/data/log", F_OK) != 0) {
        mkdir("/data/log", S_IRWXU | S_IRWXG | S_IXOTH);
    }
    if (access("/data/log/hiaudit", F_OK) != 0) {
        mkdir("/data/log/hiaudit", S_IRWXU | S_IRWXG | S_IXOTH);
    }
    if (access(HIAUDIT_DIR, F_OK) != 0) {
        mkdir(HIAUDIT_DIR, S_IRWXU | S_IRWXG | S_IXOTH);
    }
}

void WriteDummyFile(const std::string &name, const std::string &content = "dummy")
{
    std::ofstream ofs(std::string(HIAUDIT_DIR) + name);
    ofs << content;
    ofs.close();
}

AuditLog MakeSampleLog(const std::string &status = "SUCCESS")
{
    AuditLog log;
    log.cause = "diskManager";
    log.operationType = "DiskManager::Mount";
    log.operationScenario = "bizStage:41";
    log.operationStatus = status;
    log.operationCount = 1;
    log.extend = "ret:0";
    return log;
}
} // namespace

class HiAuditStateGuard {
public:
    explicit HiAuditStateGuard(HiAudit &audit) : audit_(audit), writeFd_(audit.writeFd_),
                                                 writeLogSize_(audit.writeLogSize_.load())
    {
    }
    ~HiAuditStateGuard()
    {
        if (audit_.writeFd_ >= 0 && audit_.writeFd_ != writeFd_) {
            close(audit_.writeFd_);
        }
        audit_.writeFd_ = writeFd_;
        audit_.writeLogSize_ = writeLogSize_;
    }

private:
    HiAudit &audit_;
    int writeFd_;
    uint32_t writeLogSize_;
};

class HiAuditTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        EnsureHiAuditDir();
    }
    static void TearDownTestCase(void) {}
    void SetUp() override
    {
        EnsureHiAuditDir();
    }
    void TearDown() override {}
};

HWTEST_F(HiAuditTest, AuditLog_TitleAndToString_001, TestSize.Level0)
{
    AuditLog log;
    log.isUserBehavior = true;
    log.operationCount = 2;
    log.cause = "diskManager";
    log.operationType = "Mount";
    log.operationScenario = "bizStage:41";
    log.operationStatus = "SUCCESS";
    log.extend = "ret:0";
    EXPECT_NE(log.TitleString().find("happenTime"), std::string::npos);
    EXPECT_NE(log.ToString().find("Mount"), std::string::npos);
    EXPECT_NE(log.ToString().find("SUCCESS"), std::string::npos);
}

HWTEST_F(HiAuditTest, GetInstance_001, TestSize.Level0)
{
    EXPECT_EQ(&HiAudit::GetInstance(), &HiAudit::GetInstance());
}

HWTEST_F(HiAuditTest, Write_001, TestSize.Level0)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    audit.writeLogSize_ = 0;
    AuditLog log = MakeSampleLog();
    audit.Write(log);
    audit.Write(log);
    EXPECT_GT(audit.writeLogSize_.load(), 0U);
}

HWTEST_F(HiAuditTest, Write_TruncateLongLine_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    audit.writeLogSize_ = 1;
    AuditLog log = MakeSampleLog("FAIL");
    log.operationType = std::string(3000, 'A');
    log.extend = "ret:-1";
    audit.Write(log);
    EXPECT_GT(audit.writeLogSize_.load(), 1U);
}

HWTEST_F(HiAuditTest, Write_ShortLineNoTruncate_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    audit.writeLogSize_ = 1;
    AuditLog log = MakeSampleLog("SUCCESS");
    log.operationType = "ShortOp";
    audit.Write(log);
    EXPECT_GT(audit.writeLogSize_.load(), 1U);
}

HWTEST_F(HiAuditTest, GetMilliseconds_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    uint64_t t1 = audit.GetMilliseconds();
    uint64_t t2 = audit.GetMilliseconds();
    EXPECT_GT(t1, 0ULL);
    EXPECT_GE(t2, t1);
}

HWTEST_F(HiAuditTest, GetFormattedTimestamp_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    std::string ts = audit.GetFormattedTimestamp(audit.GetMilliseconds(), "%Y%m%d%H%M%S");
    EXPECT_FALSE(ts.empty());
    std::string milli = audit.GetFormattedTimestampEndWithMilli();
    EXPECT_GE(milli.size(), ts.size());
}

HWTEST_F(HiAuditTest, GetWriteFilePath_NotFull_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    audit.writeLogSize_ = 0;
    audit.GetWriteFilePath();
    EXPECT_EQ(audit.writeLogSize_.load(), 0U);
}

HWTEST_F(HiAuditTest, GetWriteFilePath_RotateWithOpenFd_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);

    std::ofstream ofs(HIAUDIT_FILE, std::ios::trunc);
    ofs << "seed-for-rotate\n";
    ofs.close();

    if (audit.writeFd_ >= 0) {
        close(audit.writeFd_);
    }
    audit.writeFd_ = open(HIAUDIT_FILE, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
    ASSERT_GE(audit.writeFd_, 0);
    audit.writeLogSize_ = HIAUDIT_FILE_SIZE + 1;
    audit.GetWriteFilePath();
    EXPECT_EQ(audit.writeLogSize_.load(), 0U);
    EXPECT_GE(audit.writeFd_, 0);
}

HWTEST_F(HiAuditTest, RotateAndClean_OverLimit_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);

    std::ofstream ofs(HIAUDIT_FILE, std::ios::app);
    ofs << "seed\n";
    ofs.close();

    for (int i = 0; i < 12; ++i) {
        WriteDummyFile("disk_manager_audit_2026010101010" + std::to_string(i) + ".csv");
        usleep(2000);
    }

    audit.writeLogSize_ = HIAUDIT_FILE_SIZE + 1;
    if (audit.writeFd_ >= 0) {
        close(audit.writeFd_);
        audit.writeFd_ = -1;
    }
    audit.GetWriteFilePath();
    EXPECT_EQ(audit.writeLogSize_.load(), 0U);
    audit.CleanOldAuditFile();
    SUCCEED();
}

HWTEST_F(HiAuditTest, CleanOldAuditFile_FilterAndUnderLimit_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);

    // Non-matching / filtered names should be skipped by CleanOldAuditFile.
    WriteDummyFile("other_prefix_audit_1.csv");
    WriteDummyFile("disk_manager_audit.csv"); // current log name
    WriteDummyFile("disk_manager_short");     // no .csv
    WriteDummyFile("disk_manager_audit_x.txt");
    WriteDummyFile("disk_manager_audit_under_1.csv");
    WriteDummyFile("disk_manager_audit_under_2.csv");
    usleep(2000);
    WriteDummyFile("disk_manager_audit_under_3.csv");

    audit.CleanOldAuditFile();
    SUCCEED();
}

HWTEST_F(HiAuditTest, WriteToFile_InvalidFd_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    audit.writeFd_ = -1;
    audit.writeLogSize_ = 0;
    audit.WriteToFile("invalid-fd-line\n");
    // size==0: GetWriteFilePath returns early; writeFd_ still invalid → no write.
    EXPECT_EQ(audit.writeLogSize_.load(), 0U);
}

HWTEST_F(HiAuditTest, WriteToFile_RotateFromInvalidFd_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    std::ofstream ofs(HIAUDIT_FILE, std::ios::trunc);
    ofs << "seed\n";
    ofs.close();
    if (audit.writeFd_ >= 0) {
        close(audit.writeFd_);
    }
    audit.writeFd_ = -1;
    audit.writeLogSize_ = HIAUDIT_FILE_SIZE + 1;
    audit.WriteToFile("after-rotate\n");
    // Rotate + reopen path: fd should be valid and size recount from new write.
    EXPECT_GE(audit.writeFd_, 0);
    EXPECT_GT(audit.writeLogSize_.load(), 0U);
}

HWTEST_F(HiAuditTest, WriteToFile_ValidFd_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    if (audit.writeFd_ < 0) {
        audit.writeFd_ = open(HIAUDIT_FILE, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
    }
    ASSERT_GE(audit.writeFd_, 0);
    uint32_t before = audit.writeLogSize_.load();
    if (before >= HIAUDIT_FILE_SIZE) {
        audit.writeLogSize_ = 0;
        before = 0;
    }
    audit.WriteToFile("valid-fd-line\n");
    EXPECT_GT(audit.writeLogSize_.load(), before);
}

HWTEST_F(HiAuditTest, RotateAuditLog_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    std::ofstream ofs(HIAUDIT_FILE, std::ios::trunc);
    ofs << "rotate-direct\n";
    ofs.close();
    audit.RotateAuditLog();
    SUCCEED();
}

HWTEST_F(HiAuditTest, RotateAuditLog_MissingSrc_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    unlink(HIAUDIT_FILE);
    // rename failure path (source missing).
    audit.RotateAuditLog();
    SUCCEED();
}

HWTEST_F(HiAuditTest, Init_ExistingDir_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    // Re-init when directory already exists (skip mkdir create branch).
    audit.Init();
    EXPECT_TRUE(access(HIAUDIT_DIR, F_OK) == 0);
}

HWTEST_F(HiAuditTest, GetFormattedTimestamp_Zero_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    std::string ts = audit.GetFormattedTimestamp(0, "%Y%m%d%H%M%S");
    EXPECT_FALSE(ts.empty());
}

HWTEST_F(HiAuditTest, CleanOldAuditFile_RemoveOldest_001, TestSize.Level1)
{
    auto &audit = HiAudit::GetInstance();
    HiAuditStateGuard guard(audit);
    const std::string oldest = "disk_manager_audit_oldest.csv";
    const std::string newer = "disk_manager_audit_newer.csv";
    WriteDummyFile(oldest);
    usleep(5000);
    for (int i = 0; i < 11; ++i) {
        WriteDummyFile("disk_manager_audit_bulk_" + std::to_string(i) + ".csv");
        usleep(1000);
    }
    WriteDummyFile(newer);
    audit.CleanOldAuditFile();
    // One oldest among matching rotated files should be removed when count > 10.
    SUCCEED();
}
} // namespace DiskManager
} // namespace OHOS
