# --- 创建测试可执行文件 ---
add_executable(run_tests
    test_main.cpp
)

# --- 链接依赖 ---
# 将我们的库(encoding_util)和Google Test链接到测试程序
# GTest::gtest_main 包含了main函数，我们无需自己编写
target_link_libraries(run_tests PRIVATE
    EncodingUtil::encoding_util
    GTest::gtest_main
)

# --- 注册测试到CTest ---
# 包含GoogleTest的CMake集成模块
include(GoogleTest)
# 自动发现 run_tests 中的所有 TEST_F 和 TEST 宏，并添加到CTest
gtest_add_tests(TARGET run_tests)