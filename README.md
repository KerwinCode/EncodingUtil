# EncodingUtil: 一个轻量级的C++编码转换库 (A Lightweight C++ Encoding Conversion Library)

[![Language](https://img.shields.io/badge/Language-C++20-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-brightgreen.svg)](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-brightgreen.svg)
[![Standard](https://img.shields.io/badge/Standard-Header--Only-orange.svg)](https://img.shields.io/badge/Standard-Header--Only-orange.svg)

`EncodingUtil` 是一个跨平台的、仅需头文件的C++库，专注于 `UTF-8` 和 `GBK` 两种编码的检测与相互转换。它零依赖（在各平台均使用系统原生API），并为 `string_view` 和 `u8string` 提供了现代化的接口。

`EncodingUtil` is a cross-platform, header-only C++ library focused on the detection and mutual conversion between `UTF-8` and `GBK` encodings. It has zero external dependencies (using native system APIs on each platform) and provides modern interfaces for `string_view` and `u8string`.

## ✨ 特性 (Features)

*   **编码检测 (Encoding Detection)**:
    *   能够检测字节流是 `UTF-8`, `GBK`, `ASCII` 还是 `UNKNOWN`（未知/无效）编码。Detects if a byte stream is encoded in `UTF-8`, `GBK`, `ASCII`, or `UNKNOWN` (invalid).
    *   能够识别并处理不完整的字节序列。Capable of identifying and handling incomplete byte sequences correctly.
*   **编码转换 (Encoding Conversion)**:
    *   在 `UTF-8` 和 `GBK` 之间进行高效、可靠的相互转换。Performs efficient and reliable mutual conversions between `UTF-8` and `GBK`.
    *   **严格遵守GBK标准**: 当一个UTF-8字符（如Emoji "😂"）在GBK中无法表示时，会抛出 `std::runtime_error` 异常，而不是静默替换。**Strict GBK Compliance**: Throws a `std::runtime_error` exception when a UTF-8 character (like an Emoji "😂") cannot be represented in GBK, instead of silent substitution.
*   **现代化C++接口 (Modern C++ Interface)**:
    *   所有核心函数都接受 `std::string_view` 作为输入，避免不必要的内存拷贝。All core functions accept `std::string_view` as input to prevent unnecessary memory copies.
    *   在C++20环境下，自动提供对 `std::u8string` 和 `std::u8string_view` 的重载支持。Automatically provides overload support for `std::u8string` and `std::u8string_view` in a C++20 environment.
*   **跨平台与零依赖 (Cross-Platform & Zero-Dependency)**:
    *   在 **Windows** 上，使用原生的 `Win32 API` (`MultiByteToWideChar`)。On **Windows**, it uses the native `Win32 API` (`MultiByteToWideChar`).
    *   在 **Linux** 和 **macOS** 上，使用标准的 `iconv` 库 (POSIX原生)。On **Linux** and **macOS**, it uses the standard `iconv` library (native to POSIX).
*   **仅需头文件 (Header-Only)**:
    *   只需将 `include/encoding_util/encoding_util.hpp` 文件包含到您的项目中即可使用。Simply include the `include/encoding_util/encoding_util.hpp` file in your project to get started.

## 🚀 快速开始 (Quick Start)

### 1. 集成 (Integration)

将 `include/encoding_util/encoding_util.hpp` 文件复制到您的项目头文件目录中，并在需要的地方包含它。

Copy the `include/encoding_util/encoding_util.hpp` file into your project's include directory and include it where needed.

```cpp
#include "encoding_util/encoding_util.hpp"
#include <iostream>
```

### 2. 使用示例 (Usage Example)

这是一个展示核心功能的简单示例。

Here is a simple example demonstrating the core functionalities.

```cpp
#include <iostream>
#include <string>
#include <stdexcept>
#include "encoding_util/encoding_util.hpp"

// 一个辅助函数，用于打印编码类型
void print_encoding(encoding_util::Encoding enc) {
    switch (enc) {
        case encoding_util::Encoding::UTF8:   std::cout << "UTF-8";   break;
        case encoding_util::Encoding::GBK:    std::cout << "GBK";     break;
        case encoding_util::Encoding::ASCII:  std::cout << "ASCII";   break;
        case encoding_util::Encoding::UNKNOWN:std::cout << "Unknown"; break;
    }
}

int main() {
    // --- 1. 编码检测 (Encoding Detection) ---
    std::cout << "--- 1. Encoding Detection ---" << std::endl;

    const std::string utf8_str = "你好，世界"; // "你好，世界" 的 UTF-8 编码
    const std::string gbk_str = "\xC4\xE3\xBA\xC3\xA3\xAC\xCA\xC0\xBD\xe7"; // "你好，世界" 的 GBK 编码
    const std::string ascii_str = "Hello, World";

    std::cout << "'" << utf8_str << "' is detected as: ";
    print_encoding(encoding_util::detect_encoding(utf8_str));
    std::cout << std::endl;

    std::cout << "A GBK string is detected as: ";
    print_encoding(encoding_util::detect_encoding(gbk_str));
    std::cout << std::endl;

    // --- 2. 编码转换 (Encoding Conversion) ---
    std::cout << "\n--- 2. Encoding Conversion ---" << std::endl;

    try {
        // 从 GBK 转换为 UTF-8
        std::string converted_to_utf8 = encoding_util::gbk_to_utf8(gbk_str);
        std::cout << "GBK string converted to UTF-8: " << converted_to_utf8 << std::endl;

        // 从 UTF-8 转换为 GBK
        std::string converted_to_gbk = encoding_util::utf8_to_gbk(utf8_str);
        std::cout << "UTF-8 string converted to GBK successfully (bytes: " << converted_to_gbk.length() << ")." << std::endl;
        
        // 验证往返转换的无损性
        if (encoding_util::gbk_to_utf8(converted_to_gbk) == utf8_str) {
            std::cout << "Round-trip conversion is lossless." << std::endl;
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "Conversion Error: " << e.what() << std::endl;
    }

    // --- 3. 错误处理 (Error Handling) ---
    std::cout << "\n--- 3. Error Handling ---" << std::endl;

    const std::string str_with_emoji = "这个字符无法在GBK中表示: 😂";

    try {
        std::cout << "Attempting to convert a string with Emoji to GBK..." << std::endl;
        encoding_util::utf8_to_gbk(str_with_emoji);
    } catch (const std::runtime_error& e) {
        std::cerr << "Successfully caught expected exception: " << e.what() << std::endl;
    }

    return 0;
}
```

**预期输出 (Expected Output):**

```
--- 1. Encoding Detection ---
'你好，世界' is detected as: UTF-8
A GBK string is detected as: GBK

--- 2. Encoding Conversion ---
GBK string converted to UTF-8: 你好，世界
UTF-8 string converted to GBK successfully (bytes: 12).
Round-trip conversion is lossless.

--- 3. Error Handling ---
Attempting to convert a string with Emoji to GBK...
Successfully caught expected exception: WinAPI: 字符串包含目标编码(GBK)无法表示的字符。
```
*(注意: Windows和Linux上的异常信息文本可能略有不同。)*
*(Note: The exception message text may differ slightly between Windows and Linux.)*


## 🛠️ 构建与测试 (Building and Testing)

本项目使用 CMake (版本 >= 3.14) 和 GoogleTest 进行管理和测试。

This project is managed and tested using CMake (version >= 3.14) and GoogleTest.

```bash
# 1. 克隆仓库 (Clone the repository)
# git clone ...

# 2. 创建构建目录 (Create a build directory)
cmake -S . -B build

# 3. 构建项目 (Build the project)
cmake --build build

# 4. 运行单元测试 (Run unit tests)
cd build
ctest
```

## 📜 许可 (License)

本项目采用 MIT 许可。详情请见 `LICENSE` 文件。

This project is licensed under the MIT License. See the `LICENSE` file for details.
