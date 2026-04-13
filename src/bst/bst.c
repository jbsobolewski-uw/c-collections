//
// Created by jakub on 4/13/26.
//

#include "bst.h"
#include <stdlib.h>
#include <errno.h>

struct bst_node {
    void* data;
    struct bst_node* left;
    struct bst_node* right;
};

struct bst {
    bst_node_t* root;
    size_t size;
    object_comparator_function_t cmp;
    object_destructor_function_t destructor;
    bst_node_t* recycled_nodes;
};

/* --- Node Recycling Memory Management --- */
static bst_node_t* allocate_node(bst_t* tree) {
    if (tree->recycled_nodes) {
        bst_node_t* node = tree->recycled_nodes;
        tree->recycled_nodes = node->left; /* use left as next pointer */
        return node;
    }
    return (bst_node_t*)malloc(sizeof(bst_node_t));
}

static void free_node(bst_t* tree, bst_node_t* node) {
    node->left = tree->recycled_nodes;
    tree->recycled_nodes = node;
}

/* --- API Implementation --- */
bst_t* bst_create(object_comparator_function_t cmp, object_destructor_function_t dtor) {
    if (!cmp) {
        errno = EINVAL;
        return NULL;
    }
    bst_t* tree = (bst_t*)malloc(sizeof(bst_t));
    if (!tree) {
        errno = ENOMEM;
        return NULL;
    }
    tree->root = NULL;
    tree->size = 0;
    tree->cmp = cmp;
    tree->destructor = dtor;
    tree->recycled_nodes = NULL;
    return tree;
}

static void destroy_nodes_recursive(bst_t* tree, bst_node_t* node) {
    if (!node) return;
    destroy_nodes_recursive(tree, node->left);
    destroy_nodes_recursive(tree, node->right);
    if (tree->destructor && node->data) {
        tree->destructor(node->data);
    }
    free(node);
}

int bst_destroy(bst_t* tree) {
    if (!tree) return (errno = EINVAL, BST_ERR);

    destroy_nodes_recursive(tree, tree->root);

    bst_node_t* curr = tree->recycled_nodes;
    while (curr) {
        bst_node_t* next = curr->left;
        free(curr);
        curr = next;
    }
    free(tree);
    return BST_OK;
}

int bst_insert(bst_t* tree, void* data) {
    if (!tree) return (errno = EINVAL, BST_ERR);

    bst_node_t** curr = &tree->root;
    while (*curr) {
        int res = tree->cmp(data, (*curr)->data);
        if (res == BST_LE || res == BST_LEQ) {
            curr = &(*curr)->left;
        } else {
            curr = &(*curr)->right;
        }
    }

    bst_node_t* new_node = allocate_node(tree);
    if (!new_node) return (errno = ENOMEM, BST_ERR);

    new_node->data = data;
    new_node->left = NULL;
    new_node->right = NULL;
    *curr = new_node;
    tree->size++;
    return BST_OK;
}

static bst_node_t* remove_recursive(bst_t* tree, bst_node_t* root, void* data, int* removed) {
    if (!root) return NULL;

    int res = tree->cmp(data, root->data);
    if (res == BST_LE || res == BST_LEQ) {
        root->left = remove_recursive(tree, root->left, data, removed);
    } else if (res == BST_GR || res == BST_GREQ) {
        root->right = remove_recursive(tree, root->right, data, removed);
    } else { /* BST_EQ */
        *removed = 1;
        if (!root->left) {
            bst_node_t* temp = root->right;
            if (tree->destructor) tree->destructor(root->data);
            free_node(tree, root);
            return temp;
        } else if (!root->right) {
            bst_node_t* temp = root->left;
            if (tree->destructor) tree->destructor(root->data);
            free_node(tree, root);
            return temp;
        }

        /* Node with 2 children: Find successor */
        bst_node_t* temp = root->right;
        while (temp && temp->left) temp = temp->left;

        /* Swap data, destroying original data first */
        if (tree->destructor) tree->destructor(root->data);
        root->data = temp->data;

        /* Remove successor. We disable dtor temporarily so successor's data isn't destroyed twice */
        object_destructor_function_t temp_dtor = tree->destructor;
        tree->destructor = NULL;
        root->right = remove_recursive(tree, root->right, temp->data, removed);
        tree->destructor = temp_dtor;
    }
    return root;
}

int bst_remove(bst_t* tree, void* data) {
    if (!tree) return (errno = EINVAL, BST_ERR);
    int removed = 0;
    tree->root = remove_recursive(tree, tree->root, data, &removed);
    if (!removed) return (errno = ENOENT, BST_ERR);
    tree->size--;
    return BST_OK;
}

int bst_search(bst_t* tree, void* data, void** out_data) {
    if (!tree || !out_data) return (errno = EINVAL, BST_ERR);
    bst_node_t* curr = tree->root;
    while (curr) {
        int res = tree->cmp(data, curr->data);
        if (res == BST_EQ) {
            *out_data = curr->data;
            return BST_OK;
        }
        curr = (res == BST_LE || res == BST_LEQ) ? curr->left : curr->right;
    }
    return (errno = ENOENT, BST_ERR);
}

int bst_size(bst_t* tree, size_t* out_size) {
    if (!tree || !out_size) return (errno = EINVAL, BST_ERR);
    *out_size = tree->size;
    return BST_OK;
}

int bst_is_empty(bst_t* tree, int* out_is_empty) {
    if (!tree || !out_is_empty) return (errno = EINVAL, BST_ERR);
    *out_is_empty = (tree->size == 0) ? 1 : 0;
    return BST_OK;
}

/* --- Accessors for bst_traversals.c --- */
inline bst_node_t* bst_get_root(bst_t* tree) { return tree ? tree->root : NULL; }
inline bst_node_t* bst_node_get_left(bst_node_t* node) { return node ? node->left : NULL; }
inline bst_node_t* bst_node_get_right(bst_node_t* node) { return node ? node->right : NULL; }
inline void* bst_node_get_data(bst_node_t* node) { return node ? node->data : NULL; }
