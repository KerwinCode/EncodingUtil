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