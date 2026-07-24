add_collection_library(bstlib
        bst.c
        bst.h
        bst_traversals.c
        bst_traversals.h
)

# The BFS traversal uses the queue module internally.
target_link_libraries(bstlib queuelib)
target_link_libraries(bstlib_shared queuelib_shared)
