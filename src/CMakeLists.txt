include(${CMAKE_CURRENT_SOURCE_DIR}/compilerflags.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/linkerflags.cmake)

# Defines <name> (static, lib<name>.a) and <name>_shared (dynamic,
# lib<name>.so) built from a single set of object files.
function(add_collection_library name)
    add_library(${name}_obj OBJECT ${ARGN})
    add_library(${name} STATIC $<TARGET_OBJECTS:${name}_obj>)
    add_library(${name}_shared SHARED $<TARGET_OBJECTS:${name}_obj>)
    set_target_properties(${name}_shared PROPERTIES OUTPUT_NAME ${name})
endfunction()

add_subdirectory(bst)
add_subdirectory(hashmap)
add_subdirectory(id_manager)
add_subdirectory(list)
add_subdirectory(queue)
add_subdirectory(stack)

add_collection_library(collectionslib
        collections.c
        collections.h
        collections_errors.h
)

target_link_libraries(collectionslib
        bstlib
        hashmaplib
        idlib
        listlib
        queuelib
        stacklib
)

target_link_libraries(collectionslib_shared
        bstlib_shared
        hashmaplib_shared
        idlib_shared
        listlib_shared
        queuelib_shared
        stacklib_shared
)
