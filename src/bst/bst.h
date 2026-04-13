//
// Created by jakub on 4/13/26.
//

#ifndef C_COLLECTIONS_BST_H
#define C_COLLECTIONS_BST_H

#include <stddef.h>
#include "bst_traversals.h"

/* Return Codes */
#define BST_OK   0
#define BST_ERR (-1)

/* Comparator Macros */
#define BST_LEQ  (-2)
#define BST_LE   (-1)
#define BST_EQ    0
#define BST_GR    1
#define BST_GREQ  2

/* Function pointer types */
typedef void (*object_destructor_function_t)(void *);

typedef int (*object_comparator_function_t)(void *obj1, void *obj2);

/* Opaque pointer definitions */
typedef struct bst_node bst_node_t;
/* bst_t is already forward-declared in bst_traversals.h */

#ifdef __cplusplus
extern "C" {


#endif

/**
 * @brief Creates a new BST.
 * @param cmp Comparator function returning BST_LE, BST_EQ, or BST_GR.
 * @param dtor Destructor for freeing objects. Can be NULL.
 * @return Pointer to new BST, or NULL on error.
 */
bst_t *bst_create(object_comparator_function_t cmp, object_destructor_function_t dtor);

/**
 * @brief Destroys the tree, invoking the destructor on all remaining items.
 */
int bst_destroy(bst_t *tree);

/**
 * @brief Inserts data into the BST based on comparator logic.
 */
int bst_insert(bst_t *tree, void *data);

/**
 * @brief Removes a specific element, calling its destructor.
 */
int bst_remove(bst_t *tree, void *data);

/**
 * @brief Searches for an element. Output param out_data gets the found object.
 */
int bst_search(bst_t *tree, void *data, void **out_data);

int bst_size(bst_t *tree, size_t *out_size);

int bst_is_empty(bst_t *tree, int *out_is_empty);

/* --- Internal Accessors for bst_traversals.c --- */
/* These keep bst_node_t hidden from the public while allowing traversal */
bst_node_t *bst_get_root(bst_t *tree);

bst_node_t *bst_node_get_left(bst_node_t *node);

bst_node_t *bst_node_get_right(bst_node_t *node);

void *bst_node_get_data(bst_node_t *node);

#ifdef __cplusplus
}
#endif

#endif /* C_COLLECTIONS_BST_H */
