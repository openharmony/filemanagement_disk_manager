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

#include "hi_audit.h"

#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

#include "disk_manager_hilog.h"

namespace OHOS {
namespace DiskManager {
namespace {
struct HiAuditConfig {
    std::string logPath;
    std::string logName;
    uint32_t logSize;
    uint32_t fileSize;
    uint32_t fileCount;
};

const HiAuditConfig HIAUDIT_CONFIG = {
    "/data/log/hiaudit/disk_manager/", "disk_manager", 2 * 1024, 3 * 1024 * 1024, 10};
constexpr int8_t MILLISECONDS_LENGTH = 3;
constexpr int64_t SEC_TO_MILLISEC = 1000;
constexpr int MAX_TIME_BUFF = 64;
const std::string HIAUDIT_LOG_NAME = HIAUDIT_CONFIG.logPath + HIAUDIT_CONFIG.logName + "_audit.csv";
} // namespace

HiAudit::HiAudit()
{
    Init();
}

HiAudit::~HiAudit()
{
    if (writeFd_ >= 0) {
        fdsan_close_with_tag(writeFd_, LOG_DOMAIN);
        writeFd_ = -1;
    }
}

HiAudit &HiAudit::GetInstance()
{
    static HiAudit instance;
    return instance;
}

void HiAudit::Init()
{
    if (access(HIAUDIT_CONFIG.logPath.c_str(), F_OK) != 0) {
        int32_t ret = mkdir(HIAUDIT_CONFIG.logPath.c_str(), S_IRWXU | S_IRWXG | S_IXOTH);
        if (ret != 0) {
            LOGE("Failed to create directory %{public}s.", HIAUDIT_CONFIG.logPath.c_str());
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    writeFd_ = open(HIAUDIT_LOG_NAME.c_str(), O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
    if (writeFd_ < 0) {
        LOGE("writeFd_ open error, errno: %{public}d.", errno);
    } else {
        fdsan_exchange_owner_tag(writeFd_, 0, LOG_DOMAIN);
    }
    struct stat st;
    writeLogSize_ = stat(HIAUDIT_LOG_NAME.c_str(), &st) ? 0 : static_cast<uint64_t>(st.st_size);
    LOGI("writeLogSize: %{public}u", writeLogSize_.load());
}

uint64_t HiAudit::GetMilliseconds()
{
    auto now = std::chrono::system_clock::now();
    auto millisecs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return millisecs.count();
}

std::string HiAudit::GetFormattedTimestamp(time_t timeStamp, const std::string &format)
{
    auto seconds = timeStamp / SEC_TO_MILLISEC;
    char date[MAX_TIME_BUFF] = {0};
    struct tm result {};
    if (localtime_r(&seconds, &result) != nullptr) {
        strftime(date, MAX_TIME_BUFF, format.c_str(), &result);
    }
    return std::string(date);
}

std::string HiAudit::GetFormattedTimestampEndWithMilli()
{
    uint64_t milliSeconds = GetMilliseconds();
    std::string formattedTimeStamp = GetFormattedTimestamp(milliSeconds, "%Y%m%d%H%M%S");
    std::stringstream ss;
    ss << formattedTimeStamp;
    uint64_t milliseconds = milliSeconds % SEC_TO_MILLISEC;
    ss << std::setfill('0') << std::setw(MILLISECONDS_LENGTH) << milliseconds;
    return ss.str();
}

void HiAudit::Write(const AuditLog &auditLog)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (writeLogSize_ == 0) {
        WriteToFile(auditLog.TitleString());
    }
    std::string writeLog =
        GetFormattedTimestampEndWithMilli() + ", " + HIAUDIT_CONFIG.logName + ", NO, " + auditLog.ToString();
    LOGD("write %{public}s.", writeLog.c_str());
    if (writeLog.length() > HIAUDIT_CONFIG.logSize) {
        writeLog = writeLog.substr(0, HIAUDIT_CONFIG.logSize);
    }
    writeLog = writeLog + "\n";
    WriteToFile(writeLog);
}

void HiAudit::GetWriteFilePath()
{
    if (writeLogSize_ < HIAUDIT_CONFIG.fileSize) {
        return;
    }
    if (writeFd_ >= 0) {
        LOGW("file content full, close fd.");
        fdsan_close_with_tag(writeFd_, LOG_DOMAIN);
        writeFd_ = -1;
    }
    RotateAuditLog();
    CleanOldAuditFile();
    writeFd_ = open(HIAUDIT_LOG_NAME.c_str(), O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);
    if (writeFd_ < 0) {
        LOGE("writeFd_ open error, errno: %{public}d.", errno);
    } else {
        fdsan_exchange_owner_tag(writeFd_, 0, LOG_DOMAIN);
    }
    writeLogSize_ = 0;
}

void HiAudit::RotateAuditLog()
{
    std::string rotateName =
        HIAUDIT_CONFIG.logPath + HIAUDIT_CONFIG.logName + "_audit_" + GetFormattedTimestampEndWithMilli() + ".csv";
    if (std::rename(HIAUDIT_LOG_NAME.c_str(), rotateName.c_str()) != 0) {
        LOGW("rename audit log file failed, errno: %{public}d.", errno);
    }
}

void HiAudit::CleanOldAuditFile()
{
    uint32_t rotateFileCount = 0;
    std::string oldestAuditFile;
    DIR *dir = opendir(HIAUDIT_CONFIG.logPath.c_str());
    if (dir == nullptr) {
        LOGE("failed open dir, errno: %{public}d.", errno);
        return;
    }
    const std::string csvTag = ".csv";
    const std::string currentLogName = HIAUDIT_CONFIG.logName + "_audit.csv";
    struct dirent *ptr = nullptr;
    while ((ptr = readdir(dir)) != nullptr) {
        std::string subName = std::string(ptr->d_name);
        if (subName.find(HIAUDIT_CONFIG.logName) != 0 || subName == currentLogName ||
            subName.size() < csvTag.size() ||
            subName.compare(subName.size() - csvTag.size(), csvTag.size(), csvTag) != 0) {
            continue;
        }
        rotateFileCount = rotateFileCount + 1;
        std::string fullPath = HIAUDIT_CONFIG.logPath + subName;
        if (oldestAuditFile.empty()) {
            oldestAuditFile = fullPath;
            continue;
        }
        struct stat st {};
        struct stat oldestSt {};
        if (stat(fullPath.c_str(), &st) != 0 || stat(oldestAuditFile.c_str(), &oldestSt) != 0) {
            continue;
        }
        if (st.st_mtime < oldestSt.st_mtime) {
            oldestAuditFile = fullPath;
        }
    }
    closedir(dir);
    if (rotateFileCount > HIAUDIT_CONFIG.fileCount && !oldestAuditFile.empty()) {
        if (remove(oldestAuditFile.c_str()) != 0) {
            LOGW("remove old audit file failed, errno: %{public}d.", errno);
        }
    }
}

void HiAudit::WriteToFile(const std::string &content)
{
    GetWriteFilePath();
    if (writeFd_ < 0) {
        return;
    }
    size_t len = content.length();
    ssize_t ret = write(writeFd_, content.c_str(), len);
    if (ret < 0 || static_cast<size_t>(ret) != len) {
        LOGE("write failed, len: %{public}zu, ret: %{public}zd, errno: %{public}d.", len, ret, errno);
    }
    size_t written = (ret > 0) ? static_cast<size_t>(ret) : 0;
    writeLogSize_ = writeLogSize_ + written;
}
} // namespace DiskManager
} // namespace OHOS
