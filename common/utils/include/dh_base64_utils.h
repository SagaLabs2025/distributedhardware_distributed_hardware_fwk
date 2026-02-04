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

#ifndef OHOS_DH_BASE64_UTILS_H
#define OHOS_DH_BASE64_UTILS_H

#include <string>
#include <vector>

namespace OHOS {
namespace DistributedHardware {

/**
 * @brief Base64 编解码工具类
 * 用于将二进制数据编码为 Base64 字符串，或将 Base64 字符串解码为二进制数据
 */
class DHBase64Utils {
public:
    /**
     * @brief 将二进制数据编码为 Base64 字符串
     * @param data 二进制数据
     * @return Base64 编码后的字符串
     */
    static std::string Encode(const std::vector<uint8_t>& data);

    /**
     * @brief 将 Base64 字符串解码为二进制数据
     * @param encoded Base64 编码的字符串
     * @return 解码后的二进制数据，解码失败返回空 vector
     */
    static std::vector<uint8_t> Decode(const std::string& encoded);

    /**
     * @brief 验证 Base64 字符串格式是否有效
     * @param encoded Base64 编码的字符串
     * @return true 有效，false 无效
     */
    static bool IsValid(const std::string& encoded);
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DH_BASE64_UTILS_H
