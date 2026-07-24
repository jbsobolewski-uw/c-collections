# Compiler flags applied to every c-collections target.
# Included by src/CMakeLists.txt before any target is defined.

# The C standard (C23) is pinned here instead of via CMAKE_C_STANDARD,
# so this file is the single source of truth for compilation flags.
# GCC 13 spells it -std=c2x; from GCC 14 on, -std=c23 also works.

# Compile all objects as position independent code, so the same object
# files can feed both the static and the shared library variants.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Diagnostics that apply to every build type.
add_compile_options(
        -std=c2x
        -pedantic
        -Wall
        -Wextra
        -Wformat-security
        -Wduplicated-cond
        -Wfloat-equal
        -Wshadow
        -Wconversion
        -Wjump-misses-init
        -Wlogical-not-parentheses
        -Wnull-dereference
        -Wvla
        -Werror
)

# Debugging aids: sanitizers, stack protection, debug info and a low
# optimization level that keeps stack traces readable.
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
            -fsanitize=undefined
            -fno-sanitize-recover
            -fstack-protector-strong
            -g
            -fno-omit-frame-pointer
            -O1
    )
endif ()

# Release builds trade the debugging aids for stronger optimizations.
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(
            -O3
            -march=native
    )
endif ()
