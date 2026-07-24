//
// Created by jakub on 4/13/26.
//

#include "list.h"
#include <stdlib.h>
#include <errno.h>

/* Internal node structure */
typedef struct list_node {
    void             *data;
    struct list_node *next;
} list_node_t;

/* Internal list structure */
struct list {
    list_node_t                 *head;
    size_t                       size;
    object_destructor_function_t destructor;

    /* Pool of recycled nodes (free list) */
    list_node_t *recycled_nodes;
};

/* --- Node recycling helpers --- */

static list_node_t *allocate_node(list_t *list) {
    if (list->recycled_nodes != NULL) {
        /* Take a node from the recycling pool */
        list_node_t *node    = list->recycled_nodes;
        list->recycled_nodes = node->next;
        return node;
    }
    /* No recycled nodes available - allocate a new one */
    return (list_node_t *) malloc(sizeof(list_node_t));
}


static void free_node(list_t *list, list_node_t *node) {
    /* Push the node onto the recycling pool instead of calling free() */
    node->next           = list->recycled_nodes;
    list->recycled_nodes = node;
}


/* --- API implementation --- */

list_t *list_create(object_destructor_function_t dtor) {
    list_t *list = (list_t *) malloc(sizeof(list_t));
    if (!list) {
        errno = ENOMEM;
        return NULL;
    }

    list->head           = NULL;
    list->size           = 0;
    list->destructor     = dtor;
    list->recycled_nodes = NULL;

    return list;
}


int list_destroy(list_t *list) {
    if (!list) {
        errno = EINVAL;
        return LIST_ERR;
    }

    /* 1. Destroy the active nodes, invoking the destructor */
    list_node_t *current = list->head;
    while (current) {
        list_node_t *next = current->next;
        if (list->destructor && current->data) list->destructor(current->data);
        free(current); /* Final release */
        current = next;
    }

    /* 2. Free the nodes held in the recycling pool */
    current = list->recycled_nodes;
    while (current) {
        list_node_t *next = current->next;
        free(current); /* Final release */
        current = next;
    }

    /* 3. Free the main structure */
    free(list);

    return LIST_OK;
}


int list_add(list_t *list, void *data) {
    if (!list) {
        errno = EINVAL;
        return LIST_ERR;
    }

    list_node_t *new_node = allocate_node(list);
    if (!new_node) {
        errno = ENOMEM;
        return LIST_ERR;
    }

    new_node->data = data;
    new_node->next = list->head;
    list->head     = new_node;
    list->size++;

    return LIST_OK;
}


int list_remove(list_t *list, void *data) {
    if (!list) {
        errno = EINVAL;
        return LIST_ERR;
    }

    list_node_t **current_ptr = &list->head;

    while (*current_ptr) {
        if ((*current_ptr)->data == data) {
            list_node_t *node_to_remove = *current_ptr;

            /* Relink the pointers */
            *current_ptr = node_to_remove->next;

            /* Invoke the destructor before recycling the node */
            if (list->destructor && node_to_remove->data)
                list->destructor(node_to_remove->data);

            free_node(list, node_to_remove); /* Recycle */
            list->size--;

            return LIST_OK;
        }
        current_ptr = &(*current_ptr)->next;
    }

    /* No node holding the given data was found */
    errno = ENOENT;
    return LIST_ERR;
}


int list_size(list_t *list, size_t *out_size) {
    if (!list || !out_size) {
        errno = EINVAL;
        return LIST_ERR;
    }

    *out_size = list->size;
    return LIST_OK;
}


int list_is_empty(list_t *list, int *out_is_empty) {
    if (!list || !out_is_empty) {
        errno = EINVAL;
        return LIST_ERR;
    }

    *out_is_empty = (list->size == 0) ? 1 : 0;
    return LIST_OK;
}


int list_foreach(list_t *list, object_job_function_t job, void *argstruct) {
    if (!list || !job) {
        errno = EINVAL;
        return LIST_ERR;
    }

    list_node_t *current = list->head;
    while (current) {
        /* A non-zero return value from job() stops the iteration */
        if (job(current->data, argstruct) != 0) break;
        current = current->next;
    }

    return LIST_OK;
}
