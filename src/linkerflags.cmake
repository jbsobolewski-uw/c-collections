# Linker flags applied to every c-collections target.
# Included by src/CMakeLists.txt before any target is defined.

# The sanitizer runtime must be linked into every produced binary and
# shared library, so the sanitizer flags are repeated at link time.
# They mirror the Debug-only compile options in compilerflags.cmake.
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_link_options(
            -fsanitize=undefined
            -fno-sanitize-recover
    )
endif ()
