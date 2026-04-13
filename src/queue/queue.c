//
// Created by jakub on 4/13/26.
//

#include "queue.h"
#include <stdlib.h>
#include <errno.h>

/* Wewnętrzna struktura węzła */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node_t;

/* Wewnętrzna struktura kolejki */
struct queue {
    queue_node_t *head; /* Początek kolejki (do pobierania) */
    queue_node_t *tail; /* Koniec kolejki (do dodawania) */
    size_t size;
    object_destructor_function_t destructor;

    /* Pula zrecyklingowanych węzłów (free list) */
    queue_node_t *recycled_nodes;
};

/* --- Funkcje pomocnicze do recyklingu węzłów --- */

static queue_node_t *allocate_node(queue_t *q) {
    if (q->recycled_nodes != NULL) {
        queue_node_t *node = q->recycled_nodes;
        q->recycled_nodes = node->next;
        return node;
    }
    return (queue_node_t *) malloc(sizeof(queue_node_t));
}

static void free_node(queue_t *q, queue_node_t *node) {
    node->next = q->recycled_nodes;
    q->recycled_nodes = node;
}

/* --- Implementacja API --- */

queue_t *queue_create(object_destructor_function_t dtor) {
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    if (!q) {
        errno = ENOMEM;
        return NULL;
    }

    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
    q->destructor = dtor;
    q->recycled_nodes = NULL;

    return q;
}

int queue_destroy(queue_t *q) {
    if (!q) {
        errno = EINVAL;
        return QUEUE_ERR;
    }

    /* 1. Niszczenie aktywnych elementów kolejki */
    queue_node_t *current = q->head;
    while (current) {
        queue_node_t *next = current->next;
        if (q->destructor && current->data) {
            q->destructor(current->data);
        }
        free(current); /* Zwolnienie ostateczne */
        current = next;
    }

    /* 2. Zwalnianie węzłów z puli recyklingu */
    current = q->recycled_nodes;
    while (current) {
        queue_node_t *next = current->next;
        free(current); /* Zwolnienie ostateczne */
        current = next;
    }

    /* 3. Zwolnienie struktury głównej */
    free(q);

    return QUEUE_OK;
}

int queue_enqueue(queue_t *q, void *data) {
    if (!q) {
        errno = EINVAL;
        return QUEUE_ERR;
    }

    queue_node_t *new_node = allocate_node(q);
    if (!new_node) {
        errno = ENOMEM;
        return QUEUE_ERR;
    }

    new_node->data = data;
    new_node->next = NULL;

    if (q->tail != NULL) {
        /* Kolejka nie jest pusta, dołączamy na koniec */
        q->tail->next = new_node;
    }
    q->tail = new_node;

    if (q->head == NULL) {
        /* Była to pierwsza wstawka, więc head to też nowy węzeł */
        q->head = new_node;
    }

    q->size++;
    return QUEUE_OK;
}

int queue_dequeue(queue_t *q, void **out_data) {
    if (!q) {
        errno = EINVAL;
        return QUEUE_ERR;
    }

    if (q->head == NULL) {
        /* Kolejka jest pusta */
        errno = ENOENT;
        return QUEUE_ERR;
    }

    queue_node_t *node_to_remove = q->head;
    void *data = node_to_remove->data;

    /* Przepinamy head na kolejny element */
    q->head = node_to_remove->next;

    /* Jeśli zdjęliśmy ostatni element, tail też musi być zresetowany */
    if (q->head == NULL) {
        q->tail = NULL;
    }

    /* Przekazanie danych albo wywołanie destruktora */
    if (out_data != NULL) {
        *out_data = data; /* Użytkownik przejmuje obiekt */
    } else {
        /* Użytkownik pominął odbiór, więc usuwamy zasób permanentnie */
        if (q->destructor && data) {
            q->destructor(data);
        }
    }

    /* Węzeł wraca do recyklingu */
    free_node(q, node_to_remove);
    q->size--;

    return QUEUE_OK;
}

int queue_size(queue_t *q, size_t *out_size) {
    if (!q || !out_size) {
        errno = EINVAL;
        return QUEUE_ERR;
    }

    *out_size = q->size;
    return QUEUE_OK;
}

int queue_is_empty(queue_t *q, int *out_is_empty) {
    if (!q || !out_is_empty) {
        errno = EINVAL;
        return QUEUE_ERR;
    }

    *out_is_empty = (q->size == 0) ? 1 : 0;
    return QUEUE_OK;
}

int queue_foreach(queue_t *q, object_job_function_t job, void *argstruct) {
    if (!q || !job) {
        errno = EINVAL;
        return QUEUE_ERR;
    }

    queue_node_t *current = q->head;
    while (current) {
        if (job(current->data, argstruct) != 0) {
            break; /* Przerwanie iteracji przez użytkownika */
        }
        current = current->next;
    }

    return QUEUE_OK;
}
