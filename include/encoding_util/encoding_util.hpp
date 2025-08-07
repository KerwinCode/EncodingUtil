#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// 平台相关的头文件
#ifdef _WIN32
    #include <windows.h>
#else
    #include <errno.h>
    #include <iconv.h>
#endif

namespace encoding_util {

/**
 * @enum Encoding
 * @brief 表示字符编码类型的枚举。
 */
enum class Encoding {
    UNKNOWN,  // 未知编码
    ASCII,    // 纯ASCII
    GBK,      // GBK编码
    UTF8      // UTF-8编码
};

namespace detail {

enum class Utf8Status { VALID, INVALID_SEQUENCE, INCOMPLETE_SEQUENCE };

/**
 * @brief (内部实现) 检查一个字节序列是否是合法的GBK编码。
 * @details
 * GBK编码规则如下：
 * - 单字节: 范围 `0x00` - `0x7F`。 (二进制: `0xxxxxxx`)
 * - 双字节:
 *   - 高位字节 (Lead Byte):  范围 `0x81` - `0xFE`。
 *   - 低位字节 (Trail Byte): 范围 `0x40` - `0xFE`，但不包括 `0x7F`。
 */
inline bool is_valid_gbk(const char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        unsigned char byte = data[i];
        if (byte <= 0x7F) continue;           // 单字节ASCII
        if (byte >= 0x81 && byte <= 0xFE) {   // 双字节首字节
            if (i + 1 >= size) return false;  // 缺少第二字节
            unsigned char trail_byte = data[++i];
            if (trail_byte < 0x40 || trail_byte > 0xFE || trail_byte == 0x7F) return false;  // 第二字节无效
        } else {
            return false;  // 非法首字节
        }
    }
    return true;
}

/**
 * @brief (内部实现) 验证一个字节序列是否是UTF-8，并返回详细状态。
 * @details
 * UTF-8 编码规则：
 *    - 单字节: `0xxxxxxx`
 *    - 双字节: `110xxxxx 10xxxxxx`
 *    - 三字节: `1110xxxx 10xxxxxx 10xxxxxx`
 *    - 四字节: `11110xxx 10xxxxxx 10xxxxxx 10xxxxxx`
 */
inline Utf8Status validate_utf8(const char* data, size_t size, bool& out_is_all_ascii) {
    out_is_all_ascii = true;
    int bytes_to_check = 0;
    for (size_t i = 0; i < size; ++i) {
        unsigned char byte = static_cast<unsigned char>(data[i]);
        if (byte > 0x7F) out_is_all_ascii = false;  // 非ASCII
        if (bytes_to_check > 0) {
            if ((byte & 0xC0) != 0x80) return Utf8Status::INVALID_SEQUENCE;  // 非法后续字节
            bytes_to_check--;
        } else if (byte >= 0xC2 && byte <= 0xDF) {
            bytes_to_check = 1;  // 2字节序列
        } else if (byte >= 0xE0 && byte <= 0xEF) {
            bytes_to_check = 2;  // 3字节序列
        } else if (byte >= 0xF0 && byte <= 0xF4) {
            bytes_to_check = 3;  // 4字节序列
        } else if (byte >= 0x80) {
            return Utf8Status::INVALID_SEQUENCE;  // 非法首字节
        }
    }
    return (bytes_to_check == 0) ? Utf8Status::VALID : Utf8Status::INCOMPLETE_SEQUENCE;
}
}  // namespace detail


/**
 * @brief 检测给定字节序列的编码格式。
 *
 * @details
 * 此函数通过严格的规则验证来判断编码。
 * 检测逻辑：
 * 1. 优先验证是否为UTF-8。
 *    - 如果是完整的UTF-8，则返回 Encoding::UTF8 或 Encoding::ASCII。
 *    - 如果是不完整的UTF-8，立即返回 Encoding::UNKNOWN，因为它很可能是被截断的文件。
 * 2. 如果是无效的UTF-8序列，再检查是否为合法的GBK编码。
 * 3. 如果是GBK，则返回 Encoding::GBK。
 * 4. 如果所有检查都失败，则返回 Encoding::UNKNOWN。
 */
inline Encoding detect_encoding(std::string_view sv) {
    if (sv.empty()) return Encoding::ASCII;

    bool is_all_ascii = false;
    detail::Utf8Status utf8_status = detail::validate_utf8(sv.data(), sv.size(), is_all_ascii);

    switch (utf8_status) {
        case detail::Utf8Status::VALID: return is_all_ascii ? Encoding::ASCII : Encoding::UTF8;
        case detail::Utf8Status::INCOMPLETE_SEQUENCE:
            return Encoding::UNKNOWN;  // 不完整的UTF-8序列被视为未知，以避免误判
        case detail::Utf8Status::INVALID_SEQUENCE:
            // 如果不是有效的UTF-8，继续检查是否为GBK
            if (detail::is_valid_gbk(sv.data(), sv.size())) {
                return Encoding::GBK;
            }
            return Encoding::UNKNOWN;
    }
    return Encoding::UNKNOWN;
}

/**
 * @brief 检查一个字符串是否为有效的 UTF-8 或 ASCII 编码。
 * @param sv 要检查的字符串视图。
 * @return 如果是 UTF-8 或 ASCII，则为 true；否则为 false。
 */
inline bool is_utf8(std::string_view sv) {
    Encoding enc = detect_encoding(sv);
    return enc == Encoding::UTF8 || enc == Encoding::ASCII;
}

/**
 * @brief 检查一个字符串是否为有效的 GBK 或 ASCII 编码。
 * @param sv 要检查的字符串视图。
 * @return 如果是 GBK 或 ASCII，则为 true；否则为 false。
 */
inline bool is_gbk(std::string_view sv) {
    Encoding enc = detect_encoding(sv);
    return enc == Encoding::GBK || enc == Encoding::ASCII;
}

#ifdef _WIN32

// ========== Windows 平台实现 (Win32 API) ==========
namespace detail {
inline std::string convert_win32(std::string_view input, UINT from_cp, UINT to_cp) {
    if (input.empty()) {
        return "";
    }

    if (input.size() > INT_MAX) {
        throw std::runtime_error("WinAPI错误：输入字符串过大，无法处理。");
    }
    const int input_len = static_cast<int>(input.size());

    // --- 步骤 1: 将任何源编码转换为标准中间格式 UTF-16 ---
    const DWORD mb_flags = (from_cp == CP_UTF8) ? MB_ERR_INVALID_CHARS : 0;
    int wide_len = MultiByteToWideChar(from_cp, mb_flags, input.data(), input_len, nullptr, 0);
    if (wide_len == 0) {
        throw std::runtime_error("WinAPI错误：计算宽字符缓冲区大小时失败。错误码：" + std::to_string(GetLastError()));
    }

    std::vector<wchar_t> wstr(wide_len);
    if (MultiByteToWideChar(from_cp, mb_flags, input.data(), input_len, wstr.data(), wide_len) == 0) {
        throw std::runtime_error("WinAPI错误：源编码转换为宽字符失败。错误码：" + std::to_string(GetLastError()));
    }


    // --- 步骤 2: 将 UTF-16 转换为最终的目标编码 ---
    // 仅当目标是GBK时，我们才需要检查是否有字符替换。
    BOOL has_used_default_char = FALSE;
    BOOL* p_used_default_char = nullptr;
    if (to_cp == 936) {
        p_used_default_char = &has_used_default_char;
    }

    // 第一次调用：计算输出大小。
    // 需要使用确切的 wide_len，第七个参数(lpDefaultChar)和第八个参数(lpUsedDefaultChar)在计算大小时必须为 nullptr。
    int out_len = WideCharToMultiByte(to_cp, 0, wstr.data(), wide_len, nullptr, 0, nullptr, nullptr);
    if (out_len == 0) {
        throw std::runtime_error("WinAPI错误：计算目标编码缓冲区大小时失败。错误码：" + std::to_string(GetLastError()));
    }

    std::string result(out_len, '\0');

    // 第二次调用：执行真正的转换。
    // 使用之前设置好的 p_used_default_char 指针，它要么是 nullptr (对于非GBK转换)，要么指向我们的信号变量。
    if (WideCharToMultiByte(to_cp, 0, wstr.data(), wide_len, &result[0], out_len, nullptr, p_used_default_char) == 0) {
        throw std::runtime_error("WinAPI错误：宽字符转换为目标编码失败。错误码：" + std::to_string(GetLastError()));
    }

    // --- 步骤 3: 仅在必要时进行最终检查 ---
    // 如果目标是GBK并且信号变量被API设置成了TRUE...
    if (to_cp == 936 && has_used_default_char) {
        // ...那么就抛出所期望的异常。
        throw std::runtime_error("转换失败：字符串中包含无法在目标编码(GBK)中表示的字符。");
    }

    return result;
}
}  // namespace detail

inline std::string gbk_to_utf8(std::string_view gbk_sv) {
    return detail::convert_win32(gbk_sv, 936, CP_UTF8);
}

inline std::string utf8_to_gbk(std::string_view utf8_sv) {
    return detail::convert_win32(utf8_sv, CP_UTF8, 936);
}

#else

// ========== Linux, macOS 等 POSIX 平台实现 (iconv) ==========
namespace detail {
inline std::string iconv_convert(std::string_view input, const char* to_encoding, const char* from_encoding) {
    if (input.empty()) return "";
    iconv_t cd = iconv_open(to_encoding, from_encoding);
    if (cd == (iconv_t)-1) throw std::runtime_error("iconv_open: 无法创建转换描述符。");

    struct IconvGuard {
        ~IconvGuard() {
            if (cd != (iconv_t)-1) iconv_close(cd);
        }
        iconv_t cd;
    } guard{cd};

    size_t in_bytes_left = input.size();
    char* in_buf = const_cast<char*>(input.data());
    std::string result;
    result.reserve(in_bytes_left);
    std::vector<char> out_buffer(4096);

    while (in_bytes_left > 0) {
        char* out_buf_ptr = out_buffer.data();
        size_t out_bytes_left = out_buffer.size();
        errno = 0;
        if (iconv(cd, &in_buf, &in_bytes_left, &out_buf_ptr, &out_bytes_left) == (size_t)-1) {
            if (errno == EILSEQ || errno == EINVAL) {
                throw std::runtime_error("iconv: 字符串包含目标编码无法表示的字符或无效序列。");
            }
            if (errno == E2BIG) {
                result.append(out_buffer.data(), out_buffer.size() - out_bytes_left);
                continue;
            }
            throw std::runtime_error("iconv: 转换失败，errno: " + std::to_string(errno));
        }
        result.append(out_buffer.data(), out_buffer.size() - out_bytes_left);
    }
    return result;
}
}  // namespace detail

inline std::string gbk_to_utf8(std::string_view gbk_sv) {
    return detail::iconv_convert(gbk_sv, "UTF-8", "GBK");
}

inline std::string utf8_to_gbk(std::string_view utf8_sv) {
    return detail::iconv_convert(utf8_sv, "GBK", "UTF-8");
}

#endif

// ========== 智能转换接口 ==========
/**
 * @brief (智能转换) 将字符串转换为 UTF-8 编码。
 * @details
 * - 如果输入已经是 UTF-8 或 ASCII，则直接返回其拷贝。
 * - 如果输入是 GBK，则执行转换。
 * - 如果输入是 UNKNOWN，则抛出 std::runtime_error。
 * @param sv 输入的字符串视图。
 * @return 转换为 UTF-8 编码的字符串。
 * @throws std::runtime_error 如果输入编码未知或转换失败。
 */
inline std::string to_utf8(std::string_view sv) {
    switch (detect_encoding(sv)) {
        case Encoding::UTF8:
        case Encoding::ASCII: return std::string(sv);
        case Encoding::GBK: return gbk_to_utf8(sv);
        case Encoding::UNKNOWN:
        default: throw std::runtime_error("to_utf8: 输入字符串的编码无法识别。");
    }
}

/**
 * @brief (智能转换) 将字符串转换为 GBK 编码。
 * @details
 * - 如果输入已经是 GBK 或 ASCII，则直接返回其拷贝。
 * - 如果输入是 UTF-8，则执行转换。
 * - 如果输入是 UNKNOWN，则抛出 std::runtime_error。
 * @param sv 输入的字符串视图。
 * @return 转换为 GBK 编码的字符串。
 * @throws std::runtime_error 如果输入编码未知或转换失败。
 */
inline std::string to_gbk(std::string_view sv) {
    switch (detect_encoding(sv)) {
        case Encoding::GBK:
        case Encoding::ASCII: return std::string(sv);
        case Encoding::UTF8: return utf8_to_gbk(sv);
        case Encoding::UNKNOWN:
        default: throw std::runtime_error("to_gbk: 输入字符串的编码无法识别。");
    }
}

// ========== C++20 u8string 兼容层 ==========
// 仅在支持 char8_t 的情况下提供以下函数，__cplusplus 宏在 gcc 10.3 不会被定义为 202002L，故使用 __cpp_char8_t 更准确
#if defined(__cpp_char8_t)

/**
 * @brief (智能转换) 将 C++20 u8string (UTF-8) 转换为 GBK 编码的 std::string。
 * @param u8_sv 输入的 u8string_view。
 * @return 转换为 GBK 编码的字符串。
 * @throws std::runtime_error 如果转换失败。
 */
inline std::string to_gbk(std::u8string_view u8_sv) {
    return utf8_to_gbk(std::string_view(reinterpret_cast<const char*>(u8_sv.data()), u8_sv.size()));
}

/**
 * @brief (智能转换) 将任意编码的字符串转换为 C++20 u8string (UTF-8)。
 * @details
 * - 如果输入是 GBK，则执行转换。
 * - 如果输入是 UTF-8 或 ASCII，则直接构造 u8string。
 * - 如果输入是 UNKNOWN，则抛出 std::runtime_error。
 * @param sv 输入的字符串视图。
 * @return 转换为 UTF-8 的 u8string。
 * @throws std::runtime_error 如果输入编码未知或转换失败。
 */
inline std::u8string to_u8string(std::string_view sv) {
    std::string utf8_std_str = to_utf8(sv);
    return std::u8string(reinterpret_cast<const char8_t*>(utf8_std_str.c_str()), utf8_std_str.length());
}

#endif  // defined(__cpp_char8_t)

}  // namespace encoding_util