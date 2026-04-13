//
// Created by jakub on 4/13/26.
//

#ifndef C_COLLECTIONS_BST_TRAVERSALS_H
#define C_COLLECTIONS_BST_TRAVERSALS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Enum for traversal orders */
typedef enum {
    BST_TRAVERSAL_NLR, /* Pre-order: Node, Left, Right */
    BST_TRAVERSAL_LNR, /* In-order: Left, Node, Right */
    BST_TRAVERSAL_LRN, /* Post-order: Left, Right, Node */
    BST_TRAVERSAL_NRL, /* Reverse Pre-order: Node, Right, Left */
    BST_TRAVERSAL_RNL, /* Reverse In-order: Right, Node, Left */
    BST_TRAVERSAL_RLN  /* Reverse Post-order: Right, Left, Node */
} bst_traversal_t;

/* Function pointer type for operations applied to each node */
typedef int (*object_job_function_t)(void* obj, void* argstruct);

/* Forward declaration of the opaque BST type */
typedef struct bst bst_t;

/**
 * @brief Traverses the BST using the specified traversal order.
 * @param tree Pointer to the tree.
 * @param order Enum indicating the traversal type (e.g., BST_TRAVERSAL_LNR).
 * @param job Function applied to each object. Returns non-zero to break.
 * @param argstruct Additional argument passed to the job function.
 * @return BST_OK or BST_ERR (sets errno).
 */
int bst_apply(bst_t* tree, bst_traversal_t order, object_job_function_t job, void* argstruct);

#ifdef __cplusplus
}
#endif

#endif /* C_COLLECTIONS_BST_TRAVERSALS_H */
