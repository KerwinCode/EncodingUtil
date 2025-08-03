# CMake 最低版本要求
cmake_minimum_required(VERSION 3.14)

# 项目信息
project(EncodingUtil VERSION 1.0 LANGUAGES CXX)

# 设置 C++ 标准为 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 定义 Header-Only 库 ---
add_library(encoding_util INTERFACE)
add_library(EncodingUtil::encoding_util ALIAS encoding_util)
target_include_directories(encoding_util INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # 用于安装
)

# --- 启用测试 ---
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

# 当 MakeAvailable 执行时，它会读取我们上面设置的 gtest_force_shared_crt 选项
FetchContent_MakeAvailable(googletest)

# 添加测试子目录
add_subdirectory(tests)