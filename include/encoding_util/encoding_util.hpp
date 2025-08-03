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
inline Encoding detect_encoding(const char* data, size_t size) {
    if (size == 0) return Encoding::ASCII;

    bool is_all_ascii = false;
    detail::Utf8Status utf8_status = detail::validate_utf8(data, size, is_all_ascii);

    switch (utf8_status) {
        case detail::Utf8Status::VALID: return is_all_ascii ? Encoding::ASCII : Encoding::UTF8;
        case detail::Utf8Status::INCOMPLETE_SEQUENCE:
            return Encoding::UNKNOWN;  // 不完整的UTF-8序列被视为未知，以避免误判
        case detail::Utf8Status::INVALID_SEQUENCE:
            // 不是合法的UTF-8，继续检查是否为GBK
            if (detail::is_valid_gbk(data, size)) {
                return Encoding::GBK;
            }
            return Encoding::UNKNOWN;
    }
    return Encoding::UNKNOWN;
}

inline Encoding detect_encoding(std::string_view sv) {
    return detect_encoding(sv.data(), sv.size());
}

#ifdef _WIN32
// ========== Windows 平台实现 (Win32 API) ==========
namespace detail {
inline std::string convert_win32(const std::string& input, UINT from_cp, UINT to_cp) {
    if (input.empty()) return "";
    int wide_char_len = MultiByteToWideChar(from_cp, 0, input.c_str(), -1, nullptr, 0);
    if (wide_char_len == 0) throw std::runtime_error("WinAPI: 计算中间宽字符长度失败。");
    std::vector<wchar_t> wstr(wide_char_len);
    if (MultiByteToWideChar(from_cp, 0, input.c_str(), -1, wstr.data(), wide_char_len) == 0)
        throw std::runtime_error("WinAPI: 源编码转换为宽字符失败。");

    BOOL used_default_char = FALSE;
    DWORD flags = (to_cp == 936) ? WC_ERR_INVALID_CHARS : 0;

    int out_len = WideCharToMultiByte(
        to_cp, flags, wstr.data(), -1, nullptr, 0, nullptr, (to_cp == 936 ? &used_default_char : nullptr));
    if (out_len == 0) throw std::runtime_error("WinAPI: 计算目标编码长度失败。");
    if (used_default_char) throw std::runtime_error("WinAPI: 字符串包含目标编码(GBK)无法表示的字符。");

    std::string result(out_len, '\0');
    if (WideCharToMultiByte(
            to_cp, flags, wstr.data(), -1, &result[0], out_len, nullptr, (to_cp == 936 ? &used_default_char : nullptr))
        == 0)
        throw std::runtime_error("WinAPI: 宽字符转换为目标编码失败。");
    if (used_default_char) throw std::runtime_error("WinAPI: 字符串包含目标编码(GBK)无法表示的字符。");

    result.pop_back();  // 移除空终止符
    return result;
}
}  // namespace detail

inline std::string gbk_to_utf8(std::string_view gbk_sv) {
    return detail::convert_win32(std::string(gbk_sv), 936, CP_UTF8);
}

inline std::string utf8_to_gbk(std::string_view utf8_sv) {
    return detail::convert_win32(std::string(utf8_sv), CP_UTF8, 936);
}

#else

// ========== Linux, macOS 等 POSIX 平台实现 (iconv) ==========
namespace detail {
inline std::string iconv_convert(const std::string& input, const char* to_encoding, const char* from_encoding) {
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
    char* in_buf = const_cast<char*>(input.c_str());
    std::string result;
    result.reserve(in_bytes_left);
    std::vector<char> out_buffer(4096);

    while (in_bytes_left > 0) {
        char* out_buf_ptr = out_buffer.data();
        size_t out_bytes_left = out_buffer.size();
        errno = 0;
        if (iconv(cd, &in_buf, &in_bytes_left, &out_buf_ptr, &out_bytes_left) == (size_t)-1) {
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
    return detail::iconv_convert(std::string(gbk_sv), "UTF-8", "GBK");
}

inline std::string utf8_to_gbk(std::string_view utf8_sv) {
    return detail::iconv_convert(std::string(utf8_sv), "GBK", "UTF-8");
}

#endif


// ========== C++20 u8string 兼容层 ==========
#if defined(__cpp_char8_t)

inline Encoding detect_encoding(std::u8string_view u8_sv) {
    for (char8_t c : u8_sv) {
        if (c > 0x7F) return Encoding::UTF8;
    }
    return Encoding::ASCII;
}

inline std::string utf8_to_gbk(std::u8string_view u8_sv) {
    return utf8_to_gbk(std::string_view(reinterpret_cast<const char*>(u8_sv.data()), u8_sv.size()));
}

inline std::u8string gbk_to_u8string(std::string_view gbk_sv) {
    std::string utf8_std_str = gbk_to_utf8(gbk_sv);
    return std::u8string(reinterpret_cast<const char8_t*>(utf8_std_str.c_str()), utf8_std_str.length());
}

#endif  // defined(__cpp_char8_t)

}  // namespace encoding_util