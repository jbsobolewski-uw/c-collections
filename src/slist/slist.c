//
// Created by jakub on 4/13/26.
//

#include "slist.h"
#include <stdlib.h>
#include <errno.h>

/* Wewnętrzna struktura węzła */
typedef struct slist_node {
    void *data;
    struct slist_node *next;
} slist_node_t;

/* Wewnętrzna struktura listy */
struct slist {
    slist_node_t *head;
    size_t size;
    object_destructor_function_t destructor;

    /* Lista węzłów poddanych recyklingowi (free list) */
    slist_node_t *recycled_nodes;
};

/* --- Funkcje pomocnicze do recyklingu węzłów --- */

static slist_node_t *allocate_node(slist_t *list) {
    if (list->recycled_nodes != NULL) {
        /* Pobierz węzeł z listy recyklingu */
        slist_node_t *node = list->recycled_nodes;
        list->recycled_nodes = node->next;
        return node;
    }
    /* Brak węzłów do recyklingu - alokuj nowy */
    return (slist_node_t *) malloc(sizeof(slist_node_t));
}

static void free_node(slist_t *list, slist_node_t *node) {
    /* Odkłada węzeł na listę recyklingu zamiast robić free() */
    node->next = list->recycled_nodes;
    list->recycled_nodes = node;
}

/* --- Implementacja API --- */

slist_t *slist_create(object_destructor_function_t dtor) {
    slist_t *list = (slist_t *) malloc(sizeof(slist_t));
    if (!list) {
        errno = ENOMEM;
        return NULL;
    }

    list->head = NULL;
    list->size = 0;
    list->destructor = dtor;
    list->recycled_nodes = NULL;

    return list;
}

int slist_destroy(slist_t *list) {
    if (!list) {
        errno = EINVAL;
        return SLIST_ERR;
    }

    /* 1. Niszczenie aktywnych węzłów i wywoływanie destruktora */
    slist_node_t *current = list->head;
    while (current) {
        slist_node_t *next = current->next;
        if (list->destructor && current->data) {
            list->destructor(current->data);
        }
        free(current); /* Zwolnienie ostateczne */
        current = next;
    }

    /* 2. Zwalnianie węzłów z puli recyklingu */
    current = list->recycled_nodes;
    while (current) {
        slist_node_t *next = current->next;
        free(current); /* Zwolnienie ostateczne */
        current = next;
    }

    /* 3. Zwolnienie głównej struktury */
    free(list);

    return SLIST_OK;
}

int slist_add(slist_t *list, void *data) {
    if (!list) {
        errno = EINVAL;
        return SLIST_ERR;
    }

    slist_node_t *new_node = allocate_node(list);
    if (!new_node) {
        errno = ENOMEM;
        return SLIST_ERR;
    }

    new_node->data = data;
    new_node->next = list->head;
    list->head = new_node;
    list->size++;

    return SLIST_OK;
}

int slist_remove(slist_t *list, void *data) {
    if (!list) {
        errno = EINVAL;
        return SLIST_ERR;
    }

    slist_node_t **current_ptr = &list->head;

    while (*current_ptr) {
        if ((*current_ptr)->data == data) {
            slist_node_t *node_to_remove = *current_ptr;

            /* Przepięcie wskaźników */
            *current_ptr = node_to_remove->next;

            /* Wywołanie destruktora przed recyklingiem węzła */
            if (list->destructor && node_to_remove->data) {
                list->destructor(node_to_remove->data);
            }

            free_node(list, node_to_remove); /* Recykling */
            list->size--;

            return SLIST_OK;
        }
        current_ptr = &(*current_ptr)->next;
    }

    /* Nie znaleziono węzła zawierającego podane dane */
    errno = ENOENT;
    return SLIST_ERR;
}

int slist_size(slist_t *list, size_t *out_size) {
    if (!list || !out_size) {
        errno = EINVAL;
        return SLIST_ERR;
    }

    *out_size = list->size;
    return SLIST_OK;
}

int slist_is_empty(slist_t *list, int *out_is_empty) {
    if (!list || !out_is_empty) {
        errno = EINVAL;
        return SLIST_ERR;
    }

    *out_is_empty = (list->size == 0) ? 1 : 0;
    return SLIST_OK;
}

int slist_foreach(slist_t *list, object_job_function_t job, void *argstruct) {
    if (!list || !job) {
        errno = EINVAL;
        return SLIST_ERR;
    }

    slist_node_t *current = list->head;
    while (current) {
        /* Jeśli job() zwróci wartość inną niż 0, można przerwać iterację */
        if (job(current->data, argstruct) != 0) {
            break;
        }
        current = current->next;
    }

    return SLIST_OK;
}
