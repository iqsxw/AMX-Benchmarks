cmake_minimum_required(VERSION 3.10)

project(benchmark LANGUAGES C CXX ASM_NASM)

set(CMAKE_VERBOSE_MAKEFILE True)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_FILES
    sample.cpp)

source_group("\\" FILES ${SRC_FILES})

add_executable(${PROJECT_NAME}
    ${SRC_FILES})

target_compile_options(${PROJECT_NAME} PRIVATE -march=skylake-avx512 -mavx)

link_directories(/root/C/ffmpeg/libavcodec/)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
    amx_transform
    x86
    libavcodec.a)
