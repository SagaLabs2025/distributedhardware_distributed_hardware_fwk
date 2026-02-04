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

#include "dh_tlv_utils.h"
#include <cstring>

namespace OHOS {
namespace DistributedHardware {

namespace {
    // TLV 字段长度定义
    constexpr size_t TYPE_BYTES = 2;      // Type 字段字节数
    constexpr size_t LENGTH_BYTES = 4;      // Length 字段字节数
    constexpr size_t HEADER_BYTES = TYPE_BYTES + LENGTH_BYTES;

    // 单个 TLV 项最大值长度 (64KB)
    constexpr uint32_t MAX_TLV_VALUE_LENGTH = 64 * 1024;

    // 整个 TLV 数据最大长度 (128KB)
    constexpr size_t MAX_TLV_DATA_LENGTH = 128 * 1024;

    // TLV 解码错误码
    constexpr int32_t ERR_TLV_INVALID_LENGTH = -10001;
    constexpr int32_t ERR_TLV_VALUE_TOO_LARGE = -10002;
    constexpr int32_t ERR_TLV_DATA_EXCEEDED = -10003;
} // anonymous namespace

std::vector<uint8_t> DHTLVUtils::EncodeSingleTLV(uint16_t type, const std::vector<uint8_t>& value)
{
    std::vector<TLVItem> tlvList;
    tlvList.emplace_back(type, value);
    return EncodeToBinary(tlvList);
}

std::vector<uint8_t> DHTLVUtils::EncodeToBinary(const std::vector<TLVItem>& tlvList)
{
    std::vector<uint8_t> result;

    // 预分配空间
    size_t totalSize = GetEncodedLength(tlvList);
    if (totalSize == 0) {
        return {};
    }

    result.reserve(totalSize);

    for (const auto& item : tlvList) {
        // 编码 Type (2 字节, Big-endian)
        result.push_back(static_cast<uint8_t>((item.type >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(item.type & 0xFF));

        // 编码 Length (4 字节, Big-endian)
        uint32_t length = static_cast<uint32_t>(item.value.size());
        result.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
        result.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
        result.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
        result.push_back(static_cast<uint8_t>(length & 0xFF));

        // 编码 Value
        result.insert(result.end(), item.value.begin(), item.value.end());
    }

    return result;
}

int32_t DHTLVUtils::DecodeFromBinary(const std::vector<uint8_t>& binary,
                                     std::vector<TLVItem>& tlvList)
{
    tlvList.clear();

    if (binary.empty()) {
        return 0; // 空数据是有效的
    }

    if (binary.size() > MAX_TLV_DATA_LENGTH) {
        return ERR_TLV_DATA_EXCEEDED;
    }

    size_t pos = 0;
    const size_t totalSize = binary.size();

    while (pos + HEADER_BYTES <= totalSize) {
        TLVItem item;

        // 解码 Type (2 字节, Big-endian)
        item.type = (static_cast<uint16_t>(binary[pos]) << 8) |
                   static_cast<uint16_t>(binary[pos + 1]);
        pos += TYPE_BYTES;

        // 解码 Length (4 字节, Big-endian)
        uint32_t length =
            (static_cast<uint32_t>(binary[pos]) << 24) |
            (static_cast<uint32_t>(binary[pos + 1]) << 16) |
            (static_cast<uint32_t>(binary[pos + 2]) << 8) |
            (static_cast<uint32_t>(binary[pos + 3]));
        pos += LENGTH_BYTES;

        // 检查长度是否合法
        if (pos + length > totalSize) {
            return ERR_TLV_INVALID_LENGTH;
        }

        if (length > MAX_TLV_VALUE_LENGTH) {
            return ERR_TLV_VALUE_TOO_LARGE;
        }

        // 解码 Value
        if (length > 0) {
            item.value.resize(length);
            std::memcpy(item.value.data(), binary.data() + pos, length);
            pos += length;
        }

        tlvList.push_back(std::move(item));
    }

    return 0;
}

bool DHTLVUtils::FindTLVByType(const std::vector<TLVItem>& tlvList,
                                uint16_t type, std::vector<uint8_t>& value)
{
    for (const auto& item : tlvList) {
        if (item.type == type) {
            value = item.value;
            return true;
        }
    }
    return false;
}

size_t DHTLVUtils::GetEncodedLength(const std::vector<TLVItem>& tlvList)
{
    size_t totalLen = 0;
    for (const auto& item : tlvList) {
        totalLen += HEADER_BYTES + item.value.size();
    }
    return totalLen;
}

void DHTLVUtils::ClearTLVList(std::vector<TLVItem>& tlvList)
{
    tlvList.clear();
}

} // namespace DistributedHardware
} // namespace OHOS
