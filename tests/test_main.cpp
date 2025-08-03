#include <gtest/gtest.h>

#include <string_view>

#include "encoding_util/encoding_util.hpp"



// --- 测试数据 ---
const std::string gbk_hello_world = "\xc4\xe3\xba\xc3\xca\xc0\xbd\xe7";
const std::string utf8_hello_world = "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c";
const std::string ascii_str = "Hello C++ World 123!@#";
const std::string utf8_with_emoji = "UTF-8 with emoji 😂";


// ========== 检测功能测试 (Detection Tests) ==========
TEST(Detection, DetectsPureASCII) {
    EXPECT_EQ(encoding_util::detect_encoding(ascii_str), encoding_util::Encoding::ASCII);
}

TEST(Detection, DetectsUTF8) {
    EXPECT_EQ(encoding_util::detect_encoding(utf8_hello_world), encoding_util::Encoding::UTF8);
}

TEST(Detection, DetectsGBK) {
    EXPECT_EQ(encoding_util::detect_encoding(gbk_hello_world), encoding_util::Encoding::GBK);
}

TEST(Detection, DetectsInvalidAsUnknown) {
    // 这是一个损坏的GBK序列 (Lead Byte 0x81 后面跟了一个非法的 Trail Byte 0x20)
    const std::string broken_gbk = "\x81\x20";
    EXPECT_EQ(encoding_util::detect_encoding(broken_gbk), encoding_util::Encoding::UNKNOWN);
    // 0xFF 是无效的起始字节
    const std::string invalid_byte = "\xFF\xFE";
    EXPECT_EQ(encoding_util::detect_encoding(invalid_byte), encoding_util::Encoding::UNKNOWN);
    // 随机的二进制数据
    const std::string random_binary = "\x80\x90\xA0\xB0";
    EXPECT_EQ(encoding_util::detect_encoding(random_binary), encoding_util::Encoding::UNKNOWN);
}

TEST(Detection, DetectsIncompleteSequences) {
    // 不完整的UTF-8序列
    const std::string incomplete_utf8 = "\xE4\xBD";  // 缺少一个字节
    EXPECT_EQ(encoding_util::detect_encoding(incomplete_utf8), encoding_util::Encoding::UNKNOWN);
    // 不完整的GBK序列
    const std::string incomplete_gbk = "\xC4";
    EXPECT_EQ(encoding_util::detect_encoding(incomplete_gbk), encoding_util::Encoding::UNKNOWN);
}


// ========== 转换功能测试 (Conversion Tests) ==========
TEST(Conversion, RoundTripGbkToUtf8) {
    std::string utf8 = encoding_util::gbk_to_utf8(gbk_hello_world);
    EXPECT_EQ(utf8, utf8_hello_world);
    std::string gbk_restored = encoding_util::utf8_to_gbk(utf8);
    EXPECT_EQ(gbk_restored, gbk_hello_world);
}

TEST(Conversion, StringViewSupport) {
    using namespace std::string_view_literals;
    std::string_view gbk_sv = gbk_hello_world;
    std::string_view utf8_sv = utf8_hello_world;

    std::string utf8_from_sv = encoding_util::gbk_to_utf8(gbk_sv);
    EXPECT_EQ(utf8_from_sv, utf8_hello_world);

    std::string gbk_from_sv = encoding_util::utf8_to_gbk(utf8_sv);
    EXPECT_EQ(gbk_from_sv, gbk_hello_world);
}

TEST(Conversion, FailsOnUnrepresentableChars) {
    // 包含Emoji的UTF-8字符串无法转换为严格的GBK，应该抛出异常
    EXPECT_THROW(encoding_util::utf8_to_gbk(utf8_with_emoji), std::runtime_error);
}

// ========== C++20 专属功能测试 ==========
#if defined(__cpp_char8_t)

const std::u8string u8s_hello_world = u8"你好世界";

TEST(ConversionCpp20, U8StringViewSupport) {
    using namespace std::string_view_literals;
    std::u8string_view u8sv_hello = u8s_hello_world;
    std::string_view gbk_sv_hello = gbk_hello_world;

    // u8string_view -> gbk
    std::string gbk_from_u8sv = encoding_util::utf8_to_gbk(u8sv_hello);
    EXPECT_EQ(gbk_from_u8sv, gbk_hello_world);

    // gbk string_view -> u8string
    std::u8string u8s_from_gbk_sv = encoding_util::gbk_to_u8string(gbk_sv_hello);
    EXPECT_EQ(u8s_from_gbk_sv, u8s_hello_world);
}

TEST(ConversionCpp20, FailsOnUnrepresentableCharsU8) {
    const std::u8string u8s_with_emoji = u8"UTF-8 with emoji 😂";
    EXPECT_THROW(encoding_util::utf8_to_gbk(u8s_with_emoji), std::runtime_error);
}

#endif  // defined(__cpp_char8_t)