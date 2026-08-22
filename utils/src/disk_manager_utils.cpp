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

#include <cstdint>
#include <random>
#include <vector>

#include "disk_manager_utils.h"

namespace OHOS {
namespace DiskManager {

constexpr size_t INT32_MIN_ID_LENGTH = 3;
constexpr size_t INT32_SHORT_ID_LENGTH = 20;
constexpr size_t INT32_PLAINTEXT_LENGTH = 4;

constexpr const char *PATH_INVALID_FLAG1 = "../";
constexpr const char *PATH_INVALID_FLAG2 = "/..";
constexpr int32_t PATH_INVALID_FLAG_LEN = 3;
constexpr size_t DOUBLE_DOT_LEN = 2;
constexpr size_t ID_SEG_MIN_DIGITS = 1;
constexpr size_t ID_SEG_MAX_DIGITS = 4;
constexpr char FILE_SEPARATOR_CHAR = '/';
constexpr size_t VOL_PREFIX_LEN = 4;
constexpr size_t DISK_PREFIX_LEN = 5;
constexpr size_t UUID_MAX_LEN = 128;
constexpr size_t UUID_DEDUP_SUFFIX_MAX_DIGITS = 4;

constexpr size_t UUID_BYTES = 16;
constexpr int BITS_PER_BYTE = 8;
constexpr size_t BYTES_PER_UINT64 = 8;
constexpr size_t UUID_VERSION_BYTE_INDEX = 6;
constexpr uint8_t UUID_VERSION_MASK = 0x0F;
constexpr uint8_t UUID_VERSION_4 = 0x40;
constexpr size_t UUID_VARIANT_BYTE_INDEX = 8;
constexpr uint8_t UUID_VARIANT_MASK = 0x3F;
constexpr uint8_t UUID_VARIANT_RFC4122 = 0x80;
constexpr size_t UUID_FORMATTED_LEN = 36;

constexpr size_t UUID_FIELD_COUNT = 5;
constexpr size_t UUID_FIELD_TIME_LOW_LEN = 4;
constexpr size_t UUID_FIELD_TIME_MID_LEN = 2;
constexpr size_t UUID_FIELD_TIME_HI_LEN = 2;
constexpr size_t UUID_FIELD_CLOCK_SEQ_LEN = 2;
constexpr size_t UUID_FIELD_NODE_LEN = 6;
constexpr size_t UUID_FIELD_LENGTHS[UUID_FIELD_COUNT] = {
    UUID_FIELD_TIME_LOW_LEN, UUID_FIELD_TIME_MID_LEN, UUID_FIELD_TIME_HI_LEN,
    UUID_FIELD_CLOCK_SEQ_LEN, UUID_FIELD_NODE_LEN
};
constexpr int HEX_NIBBLE_SHIFT = 4;
constexpr uint8_t HEX_NIBBLE_MASK = 0x0F;
constexpr char HEX_CHARS[] = "0123456789abcdef";

constexpr char BACKSLASH_CHAR = '\\';
constexpr const char *URL_ENCODED_SLASH_LOW = "%2f";
constexpr const char *URL_ENCODED_SLASH_UP = "%2F";
constexpr const char *URL_ENCODED_BACKSLASH_LOW = "%5c";
constexpr const char *URL_ENCODED_BACKSLASH_UP = "%5C";
constexpr const char *URL_ENCODED_DOT_LOW = "%2e";
constexpr const char *URL_ENCODED_DOT_UP = "%2E";
constexpr const char *DOUBLE_URL_ENCODED_DOT_LOW = "%252e";
constexpr const char *DOUBLE_URL_ENCODED_DOT_UP = "%252E";
constexpr const char *DOUBLE_URL_ENCODED_SLASH_LOW = "%252f";
constexpr const char *DOUBLE_URL_ENCODED_SLASH_UP = "%252F";
constexpr const char *DOUBLE_URL_ENCODED_BACKSLASH_LOW = "%255c";
constexpr const char *DOUBLE_URL_ENCODED_BACKSLASH_UP = "%255C";
constexpr const char *NULL_BYTE_STR = "%00";

std::string GetAnonyString(const std::string &value)
{
    std::string res;
    std::string tmpStr("******");
    size_t strLen = value.length();
    if (strLen < INT32_MIN_ID_LENGTH) {
        return tmpStr;
    }

    if (strLen <= INT32_SHORT_ID_LENGTH) {
        res += value[0];
        res += tmpStr;
        res += value[strLen - 1];
    } else {
        res.append(value, 0, INT32_PLAINTEXT_LENGTH);
        res += tmpStr;
        res.append(value, strLen - INT32_PLAINTEXT_LENGTH, INT32_PLAINTEXT_LENGTH);
    }

    return res;
}

bool IsFilePathInvalid(const std::string &filePath)
{
    if (filePath.empty()) {
        LOGE("Relative path is not allowed, filePath is empty");
        return true;
    }
    size_t pos = filePath.find(PATH_INVALID_FLAG1);
    while (pos != std::string::npos) {
        if (pos == 0 || filePath[pos - 1] == FILE_SEPARATOR_CHAR) {
            LOGE("Relative path is not allowed, path contain ../");
            return true;
        }
        pos = filePath.find(PATH_INVALID_FLAG1, pos + PATH_INVALID_FLAG_LEN);
    }
    pos = filePath.rfind(PATH_INVALID_FLAG2);
    if ((pos != std::string::npos) && (filePath.size() - pos == PATH_INVALID_FLAG_LEN)) {
        LOGE("Relative path is not allowed, path tail is /..");
        return true;
    }

    pos = filePath.find(BACKSLASH_CHAR);
    while (pos != std::string::npos) {
        if (pos >= DOUBLE_DOT_LEN && filePath[pos - 1] == '.' && filePath[pos - DOUBLE_DOT_LEN] == '.') {
            LOGE("Relative path is not allowed, path contain ..\\");
            return true;
        }
        pos = filePath.find(BACKSLASH_CHAR, pos + 1);
    }

    static const std::vector<std::string> urlTraversalPatterns = {
        URL_ENCODED_SLASH_LOW, URL_ENCODED_SLASH_UP,
        URL_ENCODED_BACKSLASH_LOW, URL_ENCODED_BACKSLASH_UP,
        URL_ENCODED_DOT_LOW, URL_ENCODED_DOT_UP,
        DOUBLE_URL_ENCODED_DOT_LOW, DOUBLE_URL_ENCODED_DOT_UP,
        DOUBLE_URL_ENCODED_SLASH_LOW, DOUBLE_URL_ENCODED_SLASH_UP,
        DOUBLE_URL_ENCODED_BACKSLASH_LOW, DOUBLE_URL_ENCODED_BACKSLASH_UP,
    };
    for (const auto &pattern : urlTraversalPatterns) {
        if (filePath.find(pattern) != std::string::npos) {
            LOGE("Relative path is not allowed, path contain encoded traversal %{public}s", pattern.c_str());
            return true;
        }
    }

    if (filePath.find(NULL_BYTE_STR) != std::string::npos || filePath.find('\0') != std::string::npos) {
        LOGE("Relative path is not allowed, path contains null byte");
        return true;
    }

    return false;
}

bool IsPureDigitsInRange(const std::string &s, size_t minLen, size_t maxLen)
{
    if (s.size() < minLen || s.size() > maxLen) {
        LOGE("IsPureDigitsInRange: length %{public}zu not in range [%{public}zu, %{public}zu]",
             s.size(), minLen, maxLen);
        return false;
    }
    for (char c : s) {
        if (c < '0' || c > '9') {
            LOGE("IsPureDigitsInRange: contains non-digit char %{public}c", c);
            return false;
        }
    }
    return true;
}

bool IsMountPathValid(const std::string &mountPath)
{
    if (IsFilePathInvalid(mountPath)) {
        LOGE("IsMountPathValid: mountPath path traversal detected");
        return false;
    }
    if (mountPath.find("/mnt/data") != 0) {
        LOGE("IsMountPathValid: mountPath prefix is not /mnt/data");
        return false;
    }
    return true;
}

bool IsVolumeIdValid(const std::string &volumeId)
{
    if (IsFilePathInvalid(volumeId)) {
        LOGE("IsVolumeIdValid: volumeId path traversal detected");
        return false;
    }
    if (volumeId.find("mtp-") == 0 || volumeId.find("ptp-") == 0) {
        LOGI("IsVolumeIdValid: volume is mtp or ptp");
        return true;
    }
    if (volumeId.find("vol-") != 0) {
        LOGE("IsVolumeIdValid: volumeId prefix is not vol-");
        return false;
    }
    size_t firstDash = volumeId.find('-', VOL_PREFIX_LEN);
    if (firstDash == std::string::npos) {
        LOGE("IsVolumeIdValid: volumeId missing second dash");
        return false;
    }
    if (volumeId.find('-', firstDash + 1) != std::string::npos) {
        LOGE("IsVolumeIdValid: volumeId has extra dash");
        return false;
    }
    std::string seg1 = volumeId.substr(VOL_PREFIX_LEN, firstDash - VOL_PREFIX_LEN);
    std::string seg2 = volumeId.substr(firstDash + 1);
    if (!IsPureDigitsInRange(seg1, ID_SEG_MIN_DIGITS, ID_SEG_MAX_DIGITS) ||
        !IsPureDigitsInRange(seg2, ID_SEG_MIN_DIGITS, ID_SEG_MAX_DIGITS)) {
        LOGE("IsVolumeIdValid: volumeId segments not 1-4 digit numbers");
        return false;
    }
    return true;
}

bool IsDiskIdValid(const std::string &diskId)
{
    if (IsFilePathInvalid(diskId)) {
        LOGE("IsDiskIdValid: diskId path traversal detected");
        return false;
    }
    if (diskId.find("disk-") != 0) {
        LOGE("IsDiskIdValid: diskId prefix is not disk-");
        return false;
    }
    size_t firstDash = diskId.find('-', DISK_PREFIX_LEN);
    if (firstDash == std::string::npos) {
        LOGE("IsDiskIdValid: diskId missing second dash");
        return false;
    }
    if (diskId.find('-', firstDash + 1) != std::string::npos) {
        LOGE("IsDiskIdValid: diskId has extra dash");
        return false;
    }
    std::string seg1 = diskId.substr(DISK_PREFIX_LEN, firstDash - DISK_PREFIX_LEN);
    std::string seg2 = diskId.substr(firstDash + 1);
    if (!IsPureDigitsInRange(seg1, ID_SEG_MIN_DIGITS, ID_SEG_MAX_DIGITS) ||
        !IsPureDigitsInRange(seg2, ID_SEG_MIN_DIGITS, ID_SEG_MAX_DIGITS)) {
        LOGE("IsDiskIdValid: diskId segments not 1-4 digit numbers");
        return false;
    }
    return true;
}

bool IsUuidValid(const std::string &uuid)
{
    if (IsFilePathInvalid(uuid)) {
        LOGE("IsUuidValid: uuid path traversal detected");
        return false;
    }
    size_t sep = uuid.rfind(UUID_SEQ_SEPARATOR);
    if (sep != std::string::npos) {
        std::string base = uuid.substr(0, sep);
        std::string suffix = uuid.substr(sep + 1);
        if (IsPureDigitsInRange(suffix, 1, UUID_DEDUP_SUFFIX_MAX_DIGITS) && base.size() <= UUID_MAX_LEN) {
            return true;
        }
        LOGE("IsUuidValid: invalid resolved uuid, len=%{public}zu", uuid.size());
        return false;
    }
    if (uuid.size() > UUID_MAX_LEN) {
        LOGE("IsUuidValid: uuid length exceeds %{public}zu limit, actual=%{public}zu",
             static_cast<size_t>(UUID_MAX_LEN), uuid.size());
        return false;
    }
    return true;
}

std::string GenerateRandomUuid()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t a = dist(gen);
    uint64_t b = dist(gen);
    uint8_t buf[UUID_BYTES];
    for (size_t i = 0; i < BYTES_PER_UINT64; ++i) {
        buf[i] = static_cast<uint8_t>(a >> (i * BITS_PER_BYTE));
        buf[i + BYTES_PER_UINT64] = static_cast<uint8_t>(b >> (i * BITS_PER_BYTE));
    }
    buf[UUID_VERSION_BYTE_INDEX] = (buf[UUID_VERSION_BYTE_INDEX] & UUID_VERSION_MASK) | UUID_VERSION_4;
    buf[UUID_VARIANT_BYTE_INDEX] = (buf[UUID_VARIANT_BYTE_INDEX] & UUID_VARIANT_MASK) | UUID_VARIANT_RFC4122;
    char out[UUID_FORMATTED_LEN + 1] = {0};
    size_t pos = 0;
    size_t byteIdx = 0;
    for (size_t field = 0; field < UUID_FIELD_COUNT; ++field) {
        for (size_t byteInField = 0; byteInField < UUID_FIELD_LENGTHS[field]; ++byteInField) {
            out[pos++] = HEX_CHARS[buf[byteIdx] >> HEX_NIBBLE_SHIFT];
            out[pos++] = HEX_CHARS[buf[byteIdx] & HEX_NIBBLE_MASK];
            ++byteIdx;
        }
        if (field < UUID_FIELD_COUNT - 1) {
            out[pos++] = '-';
        }
    }
    return std::string(out);
}

} // namespace DiskManager
} // namespace OHOS
