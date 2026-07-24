# Building

The project builds with CMake (>= 3.13) and a GCC toolchain.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

## Targets

Every module produces **both** a static and a shared library from a single
set of position-independent objects (see the `add_collection_library` helper
in [`src/CMakeLists.txt`](../src/CMakeLists.txt)):

| Module | Static target | Shared target | Output files |
|---|---|---|---|
| list | `listlib` | `listlib_shared` | `liblistlib.a` / `liblistlib.so` |
| queue | `queuelib` | `queuelib_shared` | `libqueuelib.a` / `libqueuelib.so` |
| stack | `stacklib` | `stacklib_shared` | `libstacklib.a` / `libstacklib.so` |
| bst (+ traversals) | `bstlib` | `bstlib_shared` | `libbstlib.a` / `libbstlib.so` |
| hashmap | `hashmaplib` | `hashmaplib_shared` | `libhashmaplib.a` / `libhashmaplib.so` |
| id_manager | `idlib` | `idlib_shared` | `libidlib.a` / `libidlib.so` |
| umbrella | `collectionslib` | `collectionslib_shared` | `libcollectionslib.a` / `libcollectionslib.so` |

The umbrella target links all modules; `bstlib` additionally links `queuelib`
(the BFS traversal uses the queue internally).

## Flag files

All compiler and linker flags live in two files included by
`src/CMakeLists.txt` before any target is defined:

- [`src/compilerflags.cmake`](../src/compilerflags.cmake) — the C standard
  (C23, spelled `-std=c2x` for GCC 13), the full warning set with `-Werror`,
  and the build-type-specific groups.
- [`src/linkerflags.cmake`](../src/linkerflags.cmake) — link-time flags
  (currently the Debug-only sanitizer runtime).

### Build types

| | Debug | Release |
|---|---|---|
| Optimization | `-O1` (readable stack traces) | `-O3 -march=native` |
| Sanitizers | UBSan (`-fsanitize=undefined`, non-recovering) | — |
| Hardening | `-fstack-protector-strong` | — |
| Debug info | `-g`, `-fno-omit-frame-pointer` | — |

Warnings (`-Wall -Wextra -Wconversion -Wshadow …` plus `-Werror`) apply to
**every** build type.

Note: `-march=native` makes Release artifacts non-portable — they may use
instructions the build machine's CPU supports but older CPUs do not.

## clang-tidy

If a `clang-tidy` executable is found on the PATH, it runs alongside
compilation (`CMAKE_C_CLANG_TIDY`), using the configuration from the
project-root [`.clang-tidy`](../.clang-tidy) file.

## Linking against the library

```bash
# static
gcc app.c -Ipath/to/c-collections path/to/build/src/libcollectionslib.a \
    path/to/build/src/*/lib*.a -o app

# shared
gcc app.c -Ipath/to/c-collections -Lpath/to/build/src -lcollectionslib -o app
```

For Debug builds of the library, the consuming binary must be linked with
`-fsanitize=undefined` as well.

## Pre-commit check

A versioned git hook in `.githooks/pre-commit` builds the project and runs
the test suite before every commit. Enable it once per clone:

```bash
git config core.hooksPath .githooks
```

Bypass it in an emergency with `git commit --no-verify`.
