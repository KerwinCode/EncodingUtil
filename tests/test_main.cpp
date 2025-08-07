#include <gtest/gtest.h>

#include "encoding_util/encoding_util.hpp"


// ========== 测试数据 ==========
const std::string gbk_hello_world = "\xc4\xe3\xba\xc3\xca\xc0\xbd\xe7";                   // "你好世界"
const std::string utf8_hello_world = "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c";  // "你好世界"
const std::string ascii_str = "Hello C++ World 123!@#";
const std::string utf8_with_emoji = "UTF-8 with emoji 😂";
const std::string broken_gbk = "\x81\x20";
const std::string incomplete_utf8 = "\xE4\xBD";  // 缺少一个字节
const std::string incomplete_gbk = "\xC4";       // 缺少一个字节


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
    EXPECT_EQ(encoding_util::detect_encoding(broken_gbk), encoding_util::Encoding::UNKNOWN);
    const std::string invalid_byte = "\xFF\xFE";
    EXPECT_EQ(encoding_util::detect_encoding(invalid_byte), encoding_util::Encoding::UNKNOWN);
}

TEST(Detection, DetectsIncompleteSequencesAsUnknown) {
    EXPECT_EQ(encoding_util::detect_encoding(incomplete_utf8), encoding_util::Encoding::UNKNOWN);
    EXPECT_EQ(encoding_util::detect_encoding(incomplete_gbk), encoding_util::Encoding::UNKNOWN);
}


// ========== 布尔值检查测试 (Boolean Checks) ==========
TEST(BooleanChecks, IsUtf8) {
    EXPECT_TRUE(encoding_util::is_utf8(utf8_hello_world));
    EXPECT_TRUE(encoding_util::is_utf8(ascii_str));
    EXPECT_FALSE(encoding_util::is_utf8(gbk_hello_world));
    EXPECT_FALSE(encoding_util::is_utf8(broken_gbk));
    EXPECT_FALSE(encoding_util::is_utf8(incomplete_utf8));
}

TEST(BooleanChecks, IsGbk) {
    EXPECT_TRUE(encoding_util::is_gbk(gbk_hello_world));
    EXPECT_TRUE(encoding_util::is_gbk(ascii_str));
    EXPECT_FALSE(encoding_util::is_gbk(utf8_hello_world));
    EXPECT_FALSE(encoding_util::is_gbk(broken_gbk));
    EXPECT_FALSE(encoding_util::is_gbk(incomplete_utf8));
}


// ========== 智能转换测试 (Smart Conversion Tests) ==========
TEST(SmartConversion, ToUtf8) {
    EXPECT_EQ(encoding_util::to_utf8(gbk_hello_world), utf8_hello_world);
    EXPECT_EQ(encoding_util::to_utf8(utf8_hello_world), utf8_hello_world);
    EXPECT_EQ(encoding_util::to_utf8(ascii_str), ascii_str);
    EXPECT_THROW(encoding_util::to_utf8(broken_gbk), std::runtime_error);
}

TEST(SmartConversion, ToGbk) {
    EXPECT_EQ(encoding_util::to_gbk(utf8_hello_world), gbk_hello_world);
    EXPECT_EQ(encoding_util::to_gbk(gbk_hello_world), gbk_hello_world);
    EXPECT_EQ(encoding_util::to_gbk(ascii_str), ascii_str);
    EXPECT_THROW(encoding_util::to_gbk(incomplete_utf8), std::runtime_error);
    EXPECT_THROW(encoding_util::to_gbk(utf8_with_emoji), std::runtime_error);
}


// ========== 底层转换和错误处理测试 (Low-Level Conversion & Error Handling) ==========
TEST(LowLevelConversion, RoundTrip) {
    std::string utf8 = encoding_util::gbk_to_utf8(gbk_hello_world);
    EXPECT_EQ(utf8, utf8_hello_world);
    std::string gbk_restored = encoding_util::utf8_to_gbk(utf8);
    EXPECT_EQ(gbk_restored, gbk_hello_world);
}

TEST(LowLevelConversion, FailsOnUnrepresentableChars) {
    EXPECT_THROW(encoding_util::utf8_to_gbk(utf8_with_emoji), std::runtime_error);
}


// ========== C++20 专属功能测试 ==========
#if defined(__cpp_char8_t)

const std::u8string u8s_hello_world = u8"你好世界";
const std::u8string u8s_ascii = u8"Hello C++ World 123!@#";
const std::u8string u8s_with_emoji = u8"UTF-8 with emoji 😂";

TEST(ConversionCpp20, ToGbkFromU8String) {
    EXPECT_EQ(encoding_util::to_gbk(u8s_hello_world), gbk_hello_world);
    EXPECT_THROW(encoding_util::to_gbk(u8s_with_emoji), std::runtime_error);
}

TEST(ConversionCpp20, ToU8String) {
    // 从GBK转换
    EXPECT_EQ(encoding_util::to_u8string(gbk_hello_world), u8s_hello_world);
    // 输入已是UTF-8 (无操作)
    EXPECT_EQ(encoding_util::to_u8string(utf8_hello_world), u8s_hello_world);
    // 输入是ASCII (无操作)
    EXPECT_EQ(encoding_util::to_u8string(ascii_str), u8s_ascii);
    // 输入编码未知，应抛出异常
    EXPECT_THROW(encoding_util::to_u8string(broken_gbk), std::runtime_error);
}

#endif  // defined(__cpp_char8_t)