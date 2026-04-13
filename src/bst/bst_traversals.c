//
// Created by jakub on 4/13/26.
//

#include "bst_traversals.h"
#include "bst.h"
#include <errno.h>

/* Recursive engine to handle all 6 DFS variations cleanly */
static int traverse_recursive(bst_node_t *node, bst_traversal_t order, object_job_function_t job, void *argstruct) {
    if (!node) return BST_OK;

    int res = BST_OK;
    bst_node_t *left = bst_node_get_left(node);
    bst_node_t *right = bst_node_get_right(node);
    void *data = bst_node_get_data(node);

    switch (order) {
        case BST_TRAVERSAL_NLR: /* Pre-order */
            if ((res = job(data, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != BST_OK) return res;
            break;

        case BST_TRAVERSAL_LNR: /* In-order */
            if ((res = traverse_recursive(left, order, job, argstruct)) != BST_OK) return res;
            if ((res = job(data, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != BST_OK) return res;
            break;

        case BST_TRAVERSAL_LRN: /* Post-order */
            if ((res = traverse_recursive(left, order, job, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != BST_OK) return res;
            if ((res = job(data, argstruct)) != BST_OK) return res;
            break;

        case BST_TRAVERSAL_NRL: /* Reverse Pre-order */
            if ((res = job(data, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != BST_OK) return res;
            break;

        case BST_TRAVERSAL_RNL: /* Reverse In-order */
            if ((res = traverse_recursive(right, order, job, argstruct)) != BST_OK) return res;
            if ((res = job(data, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != BST_OK) return res;
            break;

        case BST_TRAVERSAL_RLN: /* Reverse Post-order */
            if ((res = traverse_recursive(right, order, job, argstruct)) != BST_OK) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != BST_OK) return res;
            if ((res = job(data, argstruct)) != BST_OK) return res;
            break;
    }
    return res;
}

int bst_apply(bst_t *tree, bst_traversal_t order, object_job_function_t job, void *argstruct) {
    if (!tree || !job) {
        errno = EINVAL;
        return BST_ERR;
    }

    /* Fetch the root opaquely using the accessor from bst.h */
    bst_node_t *root = bst_get_root(tree);
    traverse_recursive(root, order, job, argstruct);

    return BST_OK;
}
