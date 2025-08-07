# 启用测试
enable_testing()
include(FetchContent)

# 在引入 GoogleTest 之前，强制设置其内部选项以使用动态 (DLL) 运行时库
if(WIN32)
  set(gtest_force_shared_crt ON CACHE BOOL "Force GoogleTest to use shared (DLL) CRT" FORCE)
endif()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# 当 MakeAvailable 执行时，会读取上面设置的 gtest_force_shared_crt 选项
FetchContent_MakeAvailable(googletest)

# 创建测试可执行文件
add_executable(run_tests
    test_main.cpp
)

# --- 链接依赖 ---
# 将 encoding_util 和 Google Test 链接到测试程序
# GTest::gtest_main 包含了main函数，无需自己编写
target_link_libraries(run_tests PRIVATE
    EncodingUtil::encoding_util
    GTest::gtest_main
)

# --- 注册测试到 CTest ---
# 包含 Google Test 的 CMake 集成模块
include(GoogleTest)
# 自动发现 run_tests 中的所有 TEST_F 和 TEST 宏，并添加到 CTest
gtest_add_tests(TARGET run_tests)