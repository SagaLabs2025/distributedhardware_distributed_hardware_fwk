# 分布式硬件框架 TLV 自定义参数支持 - 实施总结

## 一、实施内容

### 1. 新增工具类

#### 1.1 Base64 编解码工具
- **文件位置**:
  - `common/utils/include/dh_base64_utils.h`
  - `common/utils/src/dh_base64_utils.cpp`

- **功能**:
  - `Encode()`: 将二进制数据编码为 Base64 字符串
  - `Decode()`: 将 Base64 字符串解码为二进制数据
  - `IsValid()`: 验证 Base64 字符串格式

#### 1.2 TLV 编解码工具
- **文件位置**:
  - `common/utils/include/dh_tlv_utils.h`
  - `common/utils/src/dh_tlv_utils.cpp`

- **功能**:
  - `EncodeToBinary()`: 将 TLV 列表编码为二进制数据
  - `DecodeFromBinary()`: 从二进制数据解码 TLV 列表
  - `EncodeSingleTLV()`: 编码单个 TLV 项
  - `FindTLVByType()`: 按类型查找 TLV 值
  - `GetEncodedLength()`: 获取编码后的长度
  - `ClearTLVList()`: 清空 TLV 列表

- **TLV 格式**:
  - Type: 2 字节 (Big-endian)
  - Length: 4 字节 (Big-endian)
  - Value: 可变长度 (最大 64KB)
  - 总长度限制: 128KB

### 2. 数据结构修改

#### 2.1 DHDescriptor 添加自定义参数字段
- **文件**: `common/utils/include/dhardware_descriptor.h`

```cpp
struct DHDescriptor {
    std::string id;
    DHType dhType;
    /**
     * @brief 业务自定义参数 (Base64 编码的 TLV 格式)
     * 框架层透传此参数，不解析
     * 硬件层负责解码并使用自定义参数
     */
    std::string customTLVParams;
};
```

#### 2.2 EnableParam 添加自定义参数字段
- **文件**: `common/utils/include/idistributed_hardware_source.h`

```cpp
struct EnableParam {
    std::string sourceVersion;
    std::string sourceAttrs;
    std::string sinkVersion;
    std::string sinkAttrs;
    std::string subtype;
    /**
     * @brief 业务自定义参数 (Base64 编码的 TLV 格式)
     * 框架层透传此参数，不解析
     * 硬件层负责解码并使用自定义参数
     */
    std::string customTLVParams;
};
```

### 3. 框架层透传实现

#### 3.1 ComponentManager::Enable 方法修改
- **文件**: `services/distributedhardwarefwkservice/include/componentmanager/component_manager.h`
- **修改**: 添加可选的 DHDescriptor 参数

```cpp
int32_t Enable(const std::string &networkId, const std::string &uuid, const std::string &dhId,
    const DHType dhType, bool isActive = false, const DHDescriptor *dhDescriptor = nullptr);
```

- **文件**: `services/distributedhardwarefwkservice/src/componentmanager/component_manager.cpp`
- **修改**: 在 GetEnableParam 后，透传 customTLVParams

```cpp
// 如果提供了 DHDescriptor，透传自定义 TLV 参数
if (dhDescriptor != nullptr && !dhDescriptor->customTLVParams.empty()) {
    param.customTLVParams = dhDescriptor->customTLVParams;
}
```

#### 3.2 RealEnableSource 调用修改
- **文件**: `services/distributedhardwarefwkservice/src/componentmanager/component_manager.cpp`
- **修改**: 调用 Enable 时传入 dhDescriptor

```cpp
ret = Enable(networkId, uuid, dhDescriptor.id, dhDescriptor.dhType, isActive, &dhDescriptor);
```

## 二、数据流

### 业务层 → 框架层 → 硬件层

```
业务层
  ├─ 构造 TLV 数据
  ├─ 编码为 Base64 字符串 (30KB → 40KB)
  └─ 赋值给 DHDescriptor.customTLVParams
        ↓
DistributedHardwareFwkKit::EnableSource(networkId, descriptors)
        ↓
ComponentManager::EnableSource(networkId, dhDescriptor)
        ↓
ComponentManager::Enable(..., &dhDescriptor)
  ├─ GetEnableParam(...) 获取基础参数
  ├─ dhDescriptor.customTLVParams → param.customTLVParams (透传，不解析)
        ↓
ComponentEnable::Enable(networkId, dhId, param, handler)
        ↓
IDistributedHardwareSource::RegisterDistributedHardware(networkId, dhId, param, callback)
        ↓
分布式相机/分布式音频
  ├─ Base64 解码
  ├─ TLV 解析
  └─ 使用自定义参数
```

## 三、使用示例

### 3.1 业务层编码自定义参数

```cpp
#include "dh_tlv_utils.h"
#include "dh_base64_utils.h"

// 1. 构造 float 数组 (30KB)
std::vector<float> floatArray(7500);
for (size_t i = 0; i < floatArray.size(); ++i) {
    floatArray[i] = static_cast<float>(i) * 0.1f;
}

// 2. 转为字节
std::vector<uint8_t> rawBytes(floatArray.size() * 4);
std::memcpy(rawBytes.data(), floatArray.data(), rawBytes.size());

// 3. 编码为 TLV
std::vector<DHTLVUtils::TLVItem> tlvParams;
tlvParams.emplace_back(0x0001, rawBytes);  // Type=0x0001

// 4. 编码为 TLV 二进制 (30KB + 6字节 ≈ 30KB)
std::vector<uint8_t> tlvBinary = DHTLVUtils::EncodeToBinary(tlvParams);

// 5. Base64 编码 (30KB → 40KB)
std::string base64String = DHBase64Utils::Encode(tlvBinary);

// 6. 赋值给 DHDescriptor
DHDescriptor desc;
desc.id = "camera_001";
desc.dhType = DHType::CAMERA;
desc.customTLVParams = base64String;

// 7. 调用框架
DistributedHardwareFwkKit::EnableSource(networkId, descriptors);
```

### 3.2 硬件层解码自定义参数

```cpp
#include "dh_tlv_utils.h"
#include "dh_base64_utils.h"

void DistributedCamera::OnRegisterResult(const std::string &networkId,
                                       const std::string &dhId,
                                       int32_t status,
                                       const EnableParam &param)
{
    // 1. Base64 解码 (40KB → 30KB)
    std::vector<uint8_t> tlvBinary = DHBase64Utils::Decode(param.customTLVParams);
    if (tlvBinary.empty()) {
        DHLOGE("Base64 decode failed");
        return;
    }

    // 2. TLV 解析
    std::vector<DHTLVUtils::TLVItem> tlvList;
    int32_t ret = DHTLVUtils::DecodeFromBinary(tlvBinary, tlvList);
    if (ret != 0) {
        DHLOGE("TLV decode failed: %{public}d", ret);
        return;
    }

    // 3. 提取 float 数组
    std::vector<uint8_t> floatBytes;
    if (DHTLVUtils::FindTLVByType(tlvList, 0x0001, floatBytes)) {
        std::vector<float> floatArray(floatBytes.size() / 4);
        std::memcpy(floatArray.data(), floatBytes.data(), floatBytes.size());

        // 使用 floatArray...
        DHLOGI("Custom params received, size = %{public}zu", floatArray.size());
    }
}
```

## 四、性能分析

| 数据类型 | 原始大小 | TLV 编码后 | Base64 编码后 | 总开销 |
|---------|---------|-----------|--------------|---------|
| 30KB float 数组 | 30KB | 30KB + 6B | 40KB | +33% |

**编码/解码耗时**:
- TLV 编码: O(n)，线性遍历
- TLV 解码: O(n)，线性遍历
- Base64 编码: O(n)，标准算法
- Base64 解码: O(n)，标准算法

**内存占用**:
- 编码时: 需要 ~70KB 临时内存 (原始 + TLV + Base64)
- 解码时: 需要 ~70KB 临时内存 (Base64 + TLV + 原始)

## 五、向后兼容性

- **DHDescriptor.customTLVParams**: 可选字段，空字符串表示无自定义参数
- **EnableParam.customTLVParams**: 可选字段，空字符串表示无自定义参数
- **ComponentManager::Enable()**: dhDescriptor 参数默认为 nullptr，不影响现有调用

## 六、错误处理

### TLV 解码错误码
- `-10001`: ERR_TLV_INVALID_LENGTH - TLV 长度无效
- `-10002`: ERR_TLV_VALUE_TOO_LARGE - TLV 值过大 (>64KB)
- `-10003`: ERR_TLV_DATA_EXCEEDED - 整个 TLV 数据过大 (>128KB)

### Base64 错误处理
- `Decode()`: 返回空 vector 表示解码失败
- `IsValid()`: 返回 false 表示格式无效

## 七、后续建议

1. **单元测试**: 添加 TLV 和 Base64 工具类的单元测试
2. **集成测试**: 测试端到端透传流程
3. **性能优化**: 如果需要频繁编码/解码，考虑使用 `std::vector<uint8_t>` 直接透传
4. **文档更新**: 更新 API 文档，说明自定义参数的使用方式
5. **类型定义**: 建议在 `device_type.h` 中定义常用的 TLV Type 常量

## 八、文件变更清单

### 新增文件
- `common/utils/include/dh_base64_utils.h`
- `common/utils/src/dh_base64_utils.cpp`
- `common/utils/include/dh_tlv_utils.h`
- `common/utils/src/dh_tlv_utils.cpp`

### 修改文件
- `common/utils/include/dhardware_descriptor.h` - 添加 customTLVParams
- `common/utils/include/idistributed_hardware_source.h` - 添加 customTLVParams
- `services/distributedhardwarefwkservice/include/componentmanager/component_manager.h` - 修改 Enable 签名
- `services/distributedhardwarefwkservice/src/componentmanager/component_manager.cpp` - 透传 customTLVParams

## 九、测试建议

### 9.1 单元测试
- Base64 编码/解码正确性
- TLV 编码/解码正确性
- 边界条件（空数据、最大长度、错误格式）
- 性能测试（30KB 数据编码/解码耗时）

### 9.2 集成测试
- 业务层 → 框架层 → 硬件层端到端测试
- float 数组透传验证
- 空参数处理
- 错误参数处理

---

**实施完成！** ✅
