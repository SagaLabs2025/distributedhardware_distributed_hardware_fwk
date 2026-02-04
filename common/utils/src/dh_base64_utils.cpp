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

#include "dh_base64_utils.h"
#include <algorithm>
#include <cstring>

namespace OHOS {
namespace DistributedHardware {

namespace {
    // Base64 字符表
    static const char BASE64_CHARS[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // 查找字符在 Base64 表中的索引
    inline uint8_t CharToValue(char c)
    {
        if (c >= 'A' && c <= 'Z') {
            return c - 'A';
        }
        if (c >= 'a' && c <= 'z') {
            return c - 'a' + 26;
        }
        if (c >= '0' && c <= '9') {
            return c - '0' + 52;
        }
        if (c == '+') {
            return 62;
        }
        if (c == '/') {
            return 63;
        }
        return 0xFF; // 无效字符
    }
} // anonymous namespace

std::string DHBase64Utils::Encode(const std::vector<uint8_t>& data)
{
    if (data.empty()) {
        return "";
    }

    std::string result;
    result.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    const size_t length = data.size();

    while (i < length) {
        uint32_t triple = static_cast<uint32_t>(data[i]) << 16;
        i++;

        if (i < length) {
            triple |= static_cast<uint32_t>(data[i]) << 8;
            i++;
        }

        if (i < length) {
            triple |= static_cast<uint32_t>(data[i]);
            i++;
        }

        result.push_back(BASE64_CHARS[(triple >> 18) & 0x3F]);
        result.push_back(BASE64_CHARS[(triple >> 12) & 0x3F]);

        if (i > length) {
            result.push_back('=');
        } else {
            result.push_back(BASE64_CHARS[(triple >> 6) & 0x3F]);
        }

        if (i > length + 1) {
            result.push_back('=');
            result.push_back('=');
        } else if (i > length) {
            result.push_back('=');
        } else {
            result.push_back(BASE64_CHARS[triple & 0x3F]);
        }
    }

    return result;
}

std::vector<uint8_t> DHBase64Utils::Decode(const std::string& encoded)
{
    if (encoded.empty()) {
        return {};
    }

    const size_t length = encoded.length();

    // Base64 编码长度必须是 4 的倍数
    if (length % 4 != 0) {
        return {};
    }

    std::vector<uint8_t> result;
    result.reserve((length / 4) * 3);

    for (size_t i = 0; i < length; i += 4) {
        uint32_t quad = 0;

        for (size_t j = 0; j < 4; ++j) {
            uint8_t val = CharToValue(encoded[i + j]);
            if (val == 0xFF && encoded[i + j] != '=') {
                // 无效字符
                return {};
            }
            quad = (quad << 6) | val;
        }

        result.push_back(static_cast<uint8_t>((quad >> 16) & 0xFF));

        if (encoded[i + 2] != '=') {
            result.push_back(static_cast<uint8_t>((quad >> 8) & 0xFF));
        }

        if (encoded[i + 3] != '=') {
            result.push_back(static_cast<uint8_t>(quad & 0xFF));
        }
    }

    return result;
}

bool DHBase64Utils::IsValid(const std::string& encoded)
{
    if (encoded.empty()) {
        return false;
    }

    const size_t length = encoded.length();

    // 长度必须是 4 的倍数
    if (length % 4 != 0) {
        return false;
    }

    // 验证每个字符
    for (char c : encoded) {
        if (c >= 'A' && c <= 'Z') {
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            continue;
        }
        if (c >= '0' && c <= '9') {
            continue;
        }
        if (c == '+' || c == '/' || c == '=') {
            continue;
        }
        // 无效字符
        return false;
    }

    return true;
}

} // namespace DistributedHardware
} // namespace OHOS
