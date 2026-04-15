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

#include "disk/partition_table_parser.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <string>

namespace OHOS {
namespace DiskManager {

namespace {

void SplitSpace(const std::string &line, std::vector<std::string> &out)
{
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) {
        out.push_back(tok);
    }
}

} // namespace

std::string PartitionTableParser::Trim(const std::string &s)
{
    size_t a = 0;
    while (a < s.size() && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) {
        ++a;
    }
    size_t b = s.size();
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) {
        --b;
    }
    return s.substr(a, b - a);
}

bool PartitionTableParser::IsMbrTypeSupportedForVolume(const std::string &mbrTypeHex)
{
    std::string h = mbrTypeHex;
    if (h.size() >= 2 && (h[0] == '0' && (h[1] == 'x' || h[1] == 'X'))) {
        h = h.substr(2);
    }
    unsigned long v = strtoul(h.c_str(), nullptr, 16);
    switch (v) {
        case 0x06:
        case 0x07:
        case 0x0b:
        case 0x0c:
        case 0x0e:
        case 0x1b:
        case 0x83:
            return true;
        default:
            return false;
    }
}

bool PartitionTableParser::ParseSgdiskDump(const std::string &rawDump,
                                           const std::string &diskId,
                                           std::string &tableTypeOut,
                                           std::vector<PartitionRecord> &partsOut)
{
    tableTypeOut.clear();
    partsOut.clear();
    std::string table;
    std::stringstream ss(rawDump);
    std::string line;
    while (std::getline(ss, line)) {
        line = Trim(line);
        if (line.empty()) {
            continue;
        }
        std::vector<std::string> tok;
        SplitSpace(line, tok);
        if (tok.empty()) {
            continue;
        }
        if (tok[0] == "DISK" && tok.size() >= 2) {
            table = tok[1];
            for (auto &c : table) {
                c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            }
            tableTypeOut = table;
            continue;
        }
        if (tok[0] != "PART" || tok.size() < 2) {
            continue;
        }
        const uint32_t partNum = static_cast<uint32_t>(strtoul(tok[1].c_str(), nullptr, 10));
        if (partNum < 1) {
            continue;
        }
        PartitionRecord rec;
        rec.diskId = diskId;
        rec.partitionNumber = partNum;
        if (table == "mbr" && tok.size() >= 3) {
            rec.partitionType = "mbr";
            rec.fsTypeRaw = tok[2];
            if (!IsMbrTypeSupportedForVolume(rec.fsTypeRaw)) {
                continue;
            }
        } else if (table == "gpt") {
            rec.partitionType = "gpt";
        } else {
            continue;
        }
        partsOut.push_back(rec);
    }
    return !tableTypeOut.empty();
}

} // namespace DiskManager
} // namespace OHOS
