add_subdirectory(bst)
add_subdirectory(queue)
add_subdirectory(slist)
add_subdirectory(stack)

add_library(collectionslib
        collections.c
        collections.h
)

target_link_libraries(collectionslib
        bstlib
        queuelib
        slistlib
        stacklib
)