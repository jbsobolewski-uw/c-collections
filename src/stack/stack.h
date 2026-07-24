//
// Created by jakub on 4/13/26.
//

#ifndef C_COLLECTIONS_STACK_H
#define C_COLLECTIONS_STACK_H

#include <stddef.h>
#include "../collections_errors.h"

/* Return codes */
#define STACK_OK  COLLECTIONS_OK
#define STACK_ERR COLLECTIONS_ERR

/* Function pointer types */
typedef void (*object_destructor_function_t)(void *);
typedef int (*object_job_function_t)(void *obj, void *argstruct);

/* Opaque pointer - the stack structure is hidden */
typedef struct stack stack_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a new stack.
 * @param dtor Function used to clean up stack elements when the stack is
 * destroyed. Can be NULL.
 * @return Pointer to the new stack, or NULL on error (sets errno = ENOMEM).
 */
stack_t *stack_create(object_destructor_function_t dtor);

/**
 * @brief Destroys the stack, invoking the destructor on the remaining elements
 * and freeing memory.
 * @param stack Pointer to the stack.
 * @return STACK_OK or STACK_ERR (sets errno).
 */
int stack_destroy(stack_t *stack);

/**
 * @brief Pushes a new element onto the top of the stack.
 * @param stack Pointer to the stack.
 * @param data Pointer to the data.
 * @return STACK_OK or STACK_ERR (sets errno).
 */
int stack_push(stack_t *stack, void *data);

/**
 * @brief Pops the element off the top of the stack.
 * @param stack Pointer to the stack.
 * @param out_data Location where the pointer to the popped data is stored.
 * If NULL is passed, the data is destroyed using the destructor.
 * @return STACK_OK or STACK_ERR (sets errno = ENOENT if the stack is empty).
 */
int stack_pop(stack_t *stack, void **out_data);

/**
 * @brief Returns the stack size (number of elements) via an output parameter.
 * @param stack Pointer to the stack.
 * @param out_size Pointer where the size will be stored.
 * @return STACK_OK or STACK_ERR (sets errno).
 */
int stack_size(stack_t *stack, size_t *out_size);

/**
 * @brief Checks whether the stack is empty via an output parameter.
 * @param stack Pointer to the stack.
 * @param out_is_empty 1 if empty, 0 otherwise.
 * @return STACK_OK or STACK_ERR (sets errno).
 */
int stack_is_empty(stack_t *stack, int *out_is_empty);

/**
 * @brief Iterates over all stack elements (from the top down to the bottom).
 * @param stack Pointer to the stack.
 * @param job Function invoked on every element (a non-zero return value stops
 * the iteration).
 * @param argstruct Extra argument passed to the job function.
 * @return STACK_OK or STACK_ERR (sets errno).
 */
int stack_foreach(stack_t *stack, object_job_function_t job, void *argstruct);

#ifdef __cplusplus
}
#endif

#endif // C_COLLECTIONS_STACK_H
