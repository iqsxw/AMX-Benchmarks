cmake_minimum_required(VERSION 3.10)

project(AVX512_Benchmarks LANGUAGES C CXX ASM_NASM)

set(CMAKE_VERBOSE_MAKEFILE True)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(amx)
add_subdirectory(src)
add_subdirectory(x86)
add_subdirectory(media)
add_subdirectory(libjpeg-benchmark)
