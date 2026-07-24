# Allocation-interception harness (vendored; see the header comment in
# memory_tests.c for the required build options).
add_library(memtestlib STATIC
        memory_tests.c
        memory_tests.h
)
target_compile_options(memtestlib PRIVATE -std=gnu17 -w)

# Redirect the allocation functions to the __wrap_* interceptors from
# memory_tests.c; the real allocator stays reachable as __real_*.
set(WRAP_LINK_FLAGS
        -Wl,--wrap=malloc
        -Wl,--wrap=calloc
        -Wl,--wrap=realloc
        -Wl,--wrap=reallocarray
        -Wl,--wrap=free
        -Wl,--wrap=strdup
        -Wl,--wrap=strndup
)

# Builds <name>_tests from <name>_tests.c, links it against <lib> and the
# memory harness, and registers one ctest entry per test case name.
function(add_collection_test name lib)
    add_executable(${name}_tests ${name}_tests.c)
    target_link_libraries(${name}_tests ${lib} memtestlib)
    target_link_options(${name}_tests PRIVATE ${WRAP_LINK_FLAGS})
    foreach (case IN LISTS ARGN)
        add_test(NAME ${name}.${case} COMMAND ${name}_tests ${case})
    endforeach ()
endfunction()

add_collection_test(list listlib api edge ownership stress memory)
add_collection_test(queue queuelib api edge ownership stress memory)
add_collection_test(stack stacklib api edge ownership stress memory)
add_collection_test(bst bstlib api edge ownership stress memory)
add_collection_test(bst_traversals bstlib orders early_stop edge bfs memory)
add_collection_test(hashmap hashmaplib api edge ownership collisions growth shrink memory)
add_collection_test(id_manager idlib api edge recycling exhaustion memory)
