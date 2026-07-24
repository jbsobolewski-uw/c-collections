//
// Created by jakub on 4/13/26.
//

#include "stack.h"
#include <stdlib.h>
#include <errno.h>

/* Internal node structure */
typedef struct stack_node {
    void              *data;
    struct stack_node *next;
} stack_node_t;

/* Internal stack structure */
struct stack {
    stack_node_t                *top; /* Top of the stack */
    size_t                       size;
    object_destructor_function_t destructor;

    /* Pool of recycled nodes (free list) */
    stack_node_t *recycled_nodes;
};

/* --- Node recycling helpers --- */

static stack_node_t *allocate_node(stack_t *stack) {
    if (stack->recycled_nodes != NULL) {
        stack_node_t *node    = stack->recycled_nodes;
        stack->recycled_nodes = node->next;
        return node;
    }
    return (stack_node_t *) malloc(sizeof(stack_node_t));
}


static void free_node(stack_t *stack, stack_node_t *node) {
    node->next            = stack->recycled_nodes;
    stack->recycled_nodes = node;
}


/* --- API implementation --- */

stack_t *stack_create(object_destructor_function_t dtor) {
    stack_t *stack = (stack_t *) malloc(sizeof(stack_t));
    if (!stack) {
        errno = ENOMEM;
        return NULL;
    }

    stack->top            = NULL;
    stack->size           = 0;
    stack->destructor     = dtor;
    stack->recycled_nodes = NULL;

    return stack;
}


int stack_destroy(stack_t *stack) {
    if (!stack) {
        errno = EINVAL;
        return STACK_ERR;
    }

    /* 1. Destroy the active stack elements */
    stack_node_t *current = stack->top;
    while (current) {
        stack_node_t *next = current->next;
        if (stack->destructor && current->data)
            stack->destructor(current->data);
        free(current);
        current = next;
    }

    /* 2. Free the nodes held in the recycling pool */
    current = stack->recycled_nodes;
    while (current) {
        stack_node_t *next = current->next;
        free(current);
        current = next;
    }

    /* 3. Free the main structure */
    free(stack);

    return STACK_OK;
}


int stack_push(stack_t *stack, void *data) {
    if (!stack) {
        errno = EINVAL;
        return STACK_ERR;
    }

    stack_node_t *new_node = allocate_node(stack);
    if (!new_node) {
        errno = ENOMEM;
        return STACK_ERR;
    }

    /* The new node becomes the top */
    new_node->data = data;
    new_node->next = stack->top;
    stack->top     = new_node;

    stack->size++;
    return STACK_OK;
}


int stack_pop(stack_t *stack, void **out_data) {
    if (!stack) {
        errno = EINVAL;
        return STACK_ERR;
    }

    if (stack->top == NULL) {
        /* The stack is empty */
        errno = ENOENT;
        return STACK_ERR;
    }

    stack_node_t *node_to_remove = stack->top;
    void         *data           = node_to_remove->data;

    /* Move the top down to the next element */
    stack->top = node_to_remove->next;

    /* Hand the data over or invoke the destructor */
    if (out_data != NULL) {
        *out_data = data; /* The caller takes ownership of the object */
    } else {
        /* The caller ignored the result (NULL), so destroy the resource */
        if (stack->destructor && data) stack->destructor(data);
    }

    /* Return the node to the recycling pool */
    free_node(stack, node_to_remove);
    stack->size--;

    return STACK_OK;
}


int stack_size(stack_t *stack, size_t *out_size) {
    if (!stack || !out_size) {
        errno = EINVAL;
        return STACK_ERR;
    }

    *out_size = stack->size;
    return STACK_OK;
}


int stack_is_empty(stack_t *stack, int *out_is_empty) {
    if (!stack || !out_is_empty) {
        errno = EINVAL;
        return STACK_ERR;
    }

    *out_is_empty = (stack->size == 0) ? 1 : 0;
    return STACK_OK;
}


int stack_foreach(stack_t *stack, object_job_function_t job, void *argstruct) {
    if (!stack || !job) {
        errno = EINVAL;
        return STACK_ERR;
    }

    /* Iteration starts at the top and walks down the stack */
    stack_node_t *current = stack->top;
    while (current) {
        if (job(current->data, argstruct) != 0)
            break; /* Iteration stopped by the caller */
        current = current->next;
    }

    return STACK_OK;
}
