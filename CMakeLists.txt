cmake_minimum_required(VERSION 3.14)

project(EncodingUtil VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# 定义 Header-Only 库
add_library(encoding_util INTERFACE)
add_library(EncodingUtil::encoding_util ALIAS encoding_util)
target_include_directories(encoding_util INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # 用于安装
)

# 添加测试子目录
add_subdirectory(tests)

# 添加示例子目录
add_subdirectory(example)