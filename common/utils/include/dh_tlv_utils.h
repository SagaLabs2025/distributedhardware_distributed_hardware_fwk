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

#ifndef OHOS_DH_TLV_UTILS_H
#define OHOS_DH_TLV_UTILS_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace OHOS {
namespace DistributedHardware {

/**
 * @brief TLV 工具类
 * 支持 Type-Length-Value 格式的编码和解码
 * Type: 2 字节
 * Length: 4 字节
 * Value: 可变长度
 */
class DHTLVUtils {
public:
    /**
     * @brief TLV 数据项
     */
    struct TLVItem {
        uint16_t type;               // 2 字节类型
        std::vector<uint8_t> value;  // 值数据

        TLVItem() : type(0) {}
        TLVItem(uint16_t t, const std::vector<uint8_t>& v) : type(t), value(v) {}
        TLVItem(uint16_t t, std::vector<uint8_t>&& v) : type(t), value(std::move(v)) {}
    };

    /**
     * @brief 将 TLV 列表编码为二进制数据
     * @param tlvList TLV 列表
     * @return 编码后的二进制数据
     */
    static std::vector<uint8_t> EncodeToBinary(const std::vector<TLVItem>& tlvList);

    /**
     * @brief 从二进制数据解码 TLV 列表
     * @param binary 二进制数据
     * @param tlvList 输出的 TLV 列表
     * @return 0 成功，非 0 失败
     */
    static int32_t DecodeFromBinary(const std::vector<uint8_t>& binary,
                                   std::vector<TLVItem>& tlvList);

    /**
     * @brief 便捷方法：编码单个 TLV 项
     * @param type 类型
     * @param value 值数据
     * @return 编码后的二进制数据
     */
    static std::vector<uint8_t> EncodeSingleTLV(uint16_t type, const std::vector<uint8_t>& value);

    /**
     * @brief 便捷方法：按类型查找 TLV 值
     * @param tlvList TLV 列表
     * @param type 类型
     * @param value 输出的值
     * @return true 找到，false 未找到
     */
    static bool FindTLVByType(const std::vector<TLVItem>& tlvList,
                            uint16_t type, std::vector<uint8_t>& value);

    /**
     * @brief 获取编码后的二进制数据长度
     * @param tlvList TLV 列表
     * @return 字节长度
     */
    static size_t GetEncodedLength(const std::vector<TLVItem>& tlvList);

    /**
     * @brief 清空 TLV 列表
     * @param tlvList TLV 列表
     */
    static void ClearTLVList(std::vector<TLVItem>& tlvList);
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DH_TLV_UTILS_H
