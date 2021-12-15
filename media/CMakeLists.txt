cmake_minimum_required(VERSION 3.10)

project(media LANGUAGES C CXX ASM_NASM)

set(CMAKE_VERBOSE_MAKEFILE True)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SRC_FILES
    structure.cpp
    structure.h)

source_group("\\" FILES ${SRC_FILES})

add_library(${PROJECT_NAME}
    ${SRC_FILES})

target_compile_options(${PROJECT_NAME} PRIVATE -march=skylake-avx512 -mavx)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
    x86)
