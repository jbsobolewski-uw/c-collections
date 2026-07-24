cmake_minimum_required(VERSION 3.13)

project(C-COLLECTIONS LANGUAGES C)
set(CMAKE_VERBOSE_MAKEFILE ON)

# CMAKE_C_CLANG_TIDY expects the clang-tidy executable (the .clang-tidy
# config file in the project root is picked up automatically). clang does
# not know some of the GCC-only warning flags from compilerflags.cmake, so
# they must not be treated as errors during the tidy pass.
find_program(CLANG_TIDY_EXE clang-tidy)
if (CLANG_TIDY_EXE)
    set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_EXE}"
            "--extra-arg=-Wno-unknown-warning-option")
endif ()

# Compiler and linker flags are maintained in src/compilerflags.cmake and
# src/linkerflags.cmake. They must be included here, at the top level, so
# that they apply to every directory (src and test alike) - add_compile_options
# and add_link_options are directory-scoped.
include(${PROJECT_SOURCE_DIR}/src/compilerflags.cmake)
include(${PROJECT_SOURCE_DIR}/src/linkerflags.cmake)

include_directories(${PROJECT_SOURCE_DIR})
add_subdirectory(src)

enable_testing()
add_subdirectory(test)
