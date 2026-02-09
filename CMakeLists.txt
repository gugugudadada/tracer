cmake_minimum_required(VERSION 3.10)

project(PathTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项（Mac / Linux / 之后 Windows 都能用）
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 头文件搜索路径：include 和 external
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

add_executable(path_tracer
    src/main.cpp
    src/tinyobj_impl.cpp
    src/stb_image_impl.cpp
)
add_executable(test_external
    src/test_external.cpp
)
# 为运行时设置工作目录（可选）
# 这样之后用 CLion / VSCode 之类 IDE 时，程序在工程根目录下运行，方便读 scene/*
set_target_properties(path_tracer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
