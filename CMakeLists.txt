set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.12)

project(C-COLLECTIONS LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_CLANG_TIDY ".clang-tidy")

include_directories(${PROJECT_SOURCE_DIR})
add_subdirectory(src)

add_compile_options(
        -pedantic
        -Wall
        -Wextra
        -Wshadow
        -Wconversion
        -Wfloat-equal
        -Wlogical-not-parentheses
        -Wnull-dereference
        -Wno-sign-compare
        -Wno-unused-parameter
        -Wno-uninitialized
        -Wno-unused-function
        -Werror
        -fsanitize=undefined
        -fno-sanitize-recover
        -fno-omit-frame-pointer
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-march=native -O3)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-O1 -g)
endif()
