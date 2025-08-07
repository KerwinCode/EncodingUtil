# EncodingUtil: 一个轻量级的 C++编码转换库 / A Lightweight C++ Encoding Conversion Library

[![Language](https://img.shields.io/badge/Language-C++20-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-brightgreen.svg)](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-brightgreen.svg)
[![Standard](https://img.shields.io/badge/Standard-Header--Only-orange.svg)](https://img.shields.io/badge/Standard-Header--Only-orange.svg)

`EncodingUtil` 是一个跨平台的、仅需头文件的 C++库，专注于 `UTF-8` 和 `GBK` 两种编码的检测与相互转换。它零依赖（在各平台均使用系统原生 API），并为 `string_view` 和 `u8string` 提供了现代化的接口。

`EncodingUtil` is a cross-platform, header-only C++ library focused on the detection and mutual conversion between `UTF-8` and `GBK` encodings. It has zero external dependencies (using native system APIs on each platform) and provides modern interfaces for `string_view` and `u8string`.

## ✨ 特性 / Features

### 🔍 编码检测 / Encoding Detection

- 检测字节流编码类型： `UTF-8`, `GBK`, `ASCII` 或 `UNKNOWN`（未知/无效）  
  Detects encoding types: `UTF-8`, `GBK`, `ASCII`, or `UNKNOWN` (invalid).
- 识别并正确处理不完整的字节序列  
  Correctly identifies and handles incomplete byte sequences.

### 🔄 编码转换 / Encoding Conversion

- 在 `UTF-8` 和 `GBK` 间高效可靠地相互转换  
  Efficient and reliable mutual conversions between `UTF-8` and `GBK`.
- **严格遵守 GBK 标准**：无法转换的字符（如 Emoji "😂"）抛出 `std::runtime_error`，而非静默替换  
  **Strict GBK Compliance**: Throws `std::runtime_error` for unconvertible characters (e.g., Emoji "😂"), no silent substitution.

### 💻 现代 C++ 接口 / Modern C++ Interface

- 所有函数接受 `std::string_view` 输入，避免内存拷贝  
  All functions accept `std::string_view` to prevent memory copies.
- C++20 环境下自动支持 `std::u8string` 和 `std::u8string_view`  
  Automatic support for `std::u8string` and `std::u8string_view` in C++20.

### 🌐 跨平台与零依赖 / Cross-Platform & Zero-Dependency

- **Windows**：原生 `Win32 API` (`MultiByteToWideChar`)  
  Uses native `Win32 API` (`MultiByteToWideChar`).
- **Linux/macOS**：标准 `iconv` 库 (POSIX 原生)  
  Uses standard `iconv` library (native to POSIX).

### 📦 仅需头文件 / Header-Only

- 包含 `include/encoding_util/encoding_util.hpp` 即可使用  
  Simply include `include/encoding_util/encoding_util.hpp` to start.

## 🚀 快速开始 / Quick Start

### 1️⃣ 集成 / Integration

```bash
# 复制头文件到项目目录 / Copy header to project
cp include/encoding_util/encoding_util.hpp your_project/include/
```

```cpp
#include "encoding_util/encoding_util.hpp"
```

### 2️⃣ 使用示例 / Usage Example

```cpp
#include <iostream>
#include <stdexcept>
#include <string>
#include "encoding_util/encoding_util.hpp"


int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);  // 设置控制台输出为 UTF-8 编码
#endif

    const std::string gbk_str = "\xC4\xE3\xBA\xC3\xA3\xAC\xCA\xC0\xBD\xe7";  // "你好，世界" 的 GBK 编码
    const std::string utf8_str = "你好，世界";

    try {
        // --- 1. 使用智能转换确保得到 UTF-8 ---
        // to_utf8 会自动检测编码，仅在需要时转换
        std::cout << "--- 1. Smart Conversion to UTF-8 ---" << std::endl;

        std::string utf8_result_1 = encoding_util::to_utf8(gbk_str);  // 从GBK转换
        std::cout << "Converted from GBK: " << utf8_result_1 << std::endl;

        std::string utf8_result_2 = encoding_util::to_utf8(utf8_str);  // 本身就是UTF-8，无转换开销
        std::cout << "Already UTF-8: " << utf8_result_2 << std::endl;

        // --- 2. 编码判断 ---
        std::cout << "\n--- 2. Encoding Checks ---" << std::endl;
        if (encoding_util::is_gbk(gbk_str)) {
            std::cout << "The first string is confirmed to be GBK." << std::endl;
        }
        if (encoding_util::is_utf8(utf8_str)) {
            std::cout << "The second string is confirmed to be UTF-8." << std::endl;
        }

        // --- 3. 智能转换为 GBK 并处理错误 ---
        std::cout << "\n--- 3. Smart Conversion to GBK & Error Handling ---" << std::endl;
        std::string gbk_result = encoding_util::to_gbk(utf8_str);
        std::cout << "UTF-8 string converted to GBK successfully." << std::endl;

        // 尝试转换一个包含无法表示字符的字符串
        const std::string str_with_emoji = "这个字符无法在GBK中表示: 😂";
        std::cout << "Attempting to convert a string with Emoji to GBK..." << std::endl;
        encoding_util::to_gbk(str_with_emoji);

    } catch (const std::runtime_error& e) {
        std::cerr << "Successfully caught expected exception: " << e.what() << std::endl;
    }

    return 0;
}
```

**预期输出 (Expected Output):**

```text
--- 1. Smart Conversion to UTF-8 ---
Converted from GBK: 你好，世界
Already UTF-8: 你好，世界

--- 2. Encoding Checks ---
The first string is confirmed to be GBK.
The second string is confirmed to be UTF-8.

--- 3. Smart Conversion to GBK & Error Handling ---
UTF-8 string converted to GBK successfully.
Attempting to convert a string with Emoji to GBK...
Successfully caught expected exception: 转换失败：字符串中包含无法在目标编码(GBK)中表示的字符。
```

*注意: Windows和Linux上的异常信息文本可能略有不同。*

*Note: The exception message text may differ slightly between Windows and Linux.*

## 🛠️ 构建与测试 / Building and Testing

```bash
# 克隆仓库 / Clone repository
git clone https://github.com/KerwinCode/EncodingUtil.git

# 生成构建系统 / Generate build system
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 编译项目 / Compile project
cmake --build build

# 运行测试 / Run tests
cd build && ctest --verbose
```

## 📜 许可 / License

本项目采用 **MIT 许可证**，详见 LICENSE 文件。

This project is licensed under the **MIT License**, see LICENSE for details.
