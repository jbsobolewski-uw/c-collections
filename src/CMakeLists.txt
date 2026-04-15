add_subdirectory(bst)
add_subdirectory(hashmap)
add_subdirectory(id_manager)
add_subdirectory(queue)
add_subdirectory(slist)
add_subdirectory(stack)

add_library(collectionslib
        collections.c
        collections.h
)

target_link_libraries(collectionslib
        bstlib
        hashmaplib
        idlib
        queuelib
        slistlib
        stacklib
)