#ifndef C_COLLECTIONS_BST_TRAVERSALS_H
#define C_COLLECTIONS_BST_TRAVERSALS_H

/* Optional add-on for the core BST module: include this header only
 * when the traversal algorithms are needed. */
#include "bst.h"

/* Enum for traversal orders */
typedef enum {
    BST_TRAVERSAL_NLR, /* Pre-order: Node, Left, Right (DFS) */
    BST_TRAVERSAL_LNR, /* In-order: Left, Node, Right (DFS) */
    BST_TRAVERSAL_LRN, /* Post-order: Left, Right, Node (DFS) */
    BST_TRAVERSAL_NRL, /* Reverse Pre-order: Node, Right, Left (DFS) */
    BST_TRAVERSAL_RNL, /* Reverse In-order: Right, Node, Left (DFS) */
    BST_TRAVERSAL_RLN, /* Reverse Post-order: Right, Left, Node (DFS) */
    BST_TRAVERSAL_BFS /* Level-order: Breadth-First Search */
} bst_traversal_t;

/* Function pointer type for operations applied to each node */
typedef int (*object_job_function_t)(void *obj, void *argstruct);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Traverses the BST using the specified traversal order.
 * @param tree Pointer to the tree.
 * @param order Enum indicating the traversal type (e.g., BST_TRAVERSAL_LNR or BST_TRAVERSAL_BFS).
 * @param job Function applied to each object. Returns non-zero to break.
 * @param argstruct Additional argument passed to the job function.
 * @return BST_OK or BST_ERR (sets errno).
 */
int bst_apply(bst_t *tree, bst_traversal_t order, object_job_function_t job, void *argstruct);

#ifdef __cplusplus
}
#endif

#endif /* C_COLLECTIONS_BST_TRAVERSALS_H */
