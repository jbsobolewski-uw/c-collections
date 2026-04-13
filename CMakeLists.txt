set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.12)

project(C-COLLECTIONS LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_CLANG_TIDY "clang-tidy")

include_directories(${PROJECT_SOURCE_DIR})
add_subdirectory(src/slist)
add_subdirectory(src/queue)
add_subdirectory(src/stack)
add_subdirectory(src/bst)

add_compile_options(
        -Wall
        -Wextra
        -Wno-sign-compare
        -Wno-unused-parameter
        -Wno-uninitialized
        -Wno-unused-function
        -Werror
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-march=native -O3)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-march=native -O1 -g)
    add_link_options(-g)
endif()
