//
// Created by jakub on 4/13/26.
//

#include "bst_traversals.h"
#include "bst.h"
#include "../queue/queue.h"
#include <errno.h>
#include <stddef.h>

/* --- Algorytmy DFS (Depth-First Search) --- */
static int traverse_recursive(bst_node_t *node, bst_traversal_t order, object_job_function_t job, void *argstruct) {
    if (!node) return 0;

    int res = 0;
    bst_node_t *left = bst_node_get_left(node);
    bst_node_t *right = bst_node_get_right(node);
    void *data = bst_node_get_data(node);

    switch (order) {
        case BST_TRAVERSAL_NLR: /* Pre-order */
            if ((res = job(data, argstruct)) != 0) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != 0) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != 0) return res;
            break;

        case BST_TRAVERSAL_LNR: /* In-order */
            if ((res = traverse_recursive(left, order, job, argstruct)) != 0) return res;
            if ((res = job(data, argstruct)) != 0) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != 0) return res;
            break;

        case BST_TRAVERSAL_LRN: /* Post-order */
            if ((res = traverse_recursive(left, order, job, argstruct)) != 0) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != 0) return res;
            if ((res = job(data, argstruct)) != 0) return res;
            break;

        case BST_TRAVERSAL_NRL: /* Reverse Pre-order */
            if ((res = job(data, argstruct)) != 0) return res;
            if ((res = traverse_recursive(right, order, job, argstruct)) != 0) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != 0) return res;
            break;

        case BST_TRAVERSAL_RNL: /* Reverse In-order */
            if ((res = traverse_recursive(right, order, job, argstruct)) != 0) return res;
            if ((res = job(data, argstruct)) != 0) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != 0) return res;
            break;

        case BST_TRAVERSAL_RLN: /* Reverse Post-order */
            if ((res = traverse_recursive(right, order, job, argstruct)) != 0) return res;
            if ((res = traverse_recursive(left, order, job, argstruct)) != 0) return res;
            if ((res = job(data, argstruct)) != 0) return res;
            break;

        default:
            break;
    }
    return res;
}

/* --- Algorytm BFS (Breadth-First Search / Level-Order) --- */
static int traverse_bfs(bst_node_t *root, object_job_function_t job, void *argstruct) {
    if (!root) return 0;

    /* Tworzymy kolejkę bez destruktora, bo węzły należą do drzewa, nie do kolejki */
    queue_t *q = queue_create(NULL);
    if (!q) return BST_ERR; /* errno ustawione przez queue_create (ENOMEM) */

    if (queue_enqueue(q, root) != QUEUE_OK) {
        queue_destroy(q);
        return BST_ERR;
    }

    int is_empty = 0;
    int res = 0;

    /* Wykonuj dopóki kolejka nie jest pusta */
    while (queue_is_empty(q, &is_empty) == QUEUE_OK && !is_empty) {
        bst_node_t *current_node = NULL;

        if (queue_dequeue(q, (void **) &current_node) != QUEUE_OK) {
            res = BST_ERR;
            break;
        }

        /* Wywołujemy funkcję użytkownika na danych węzła */
        void *user_data = bst_node_get_data(current_node);
        if ((res = job(user_data, argstruct)) != 0) {
            break; /* Użytkownik przerwał iterację */
        }

        /* Dodajemy dzieci zdezdekodowanego węzła do kolejki (lewe, potem prawe) */
        bst_node_t *left = bst_node_get_left(current_node);
        if (left) {
            if (queue_enqueue(q, left) != QUEUE_OK) {
                res = BST_ERR;
                break;
            }
        }

        bst_node_t *right = bst_node_get_right(current_node);
        if (right) {
            if (queue_enqueue(q, right) != QUEUE_OK) {
                res = BST_ERR;
                break;
            }
        }
    }

    queue_destroy(q); /* Zwalniamy zasoby kolejki (tylko jej wewnętrzne węzły) */
    return res;
}

/* --- Główna funkcja API --- */
int bst_apply(bst_t *tree, bst_traversal_t order, object_job_function_t job, void *argstruct) {
    if (!tree || !job) {
        errno = EINVAL;
        return BST_ERR;
    }

    bst_node_t *root = bst_get_root(tree);
    if (!root) {
        return BST_OK; /* Puste drzewo, nic do zrobienia */
    }

    if (order == BST_TRAVERSAL_BFS) {
        return traverse_bfs(root, job, argstruct);
    } else {
        return traverse_recursive(root, order, job, argstruct);
    }
}
