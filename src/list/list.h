//
// Created by jakub on 4/13/26.
//

#ifndef C_COLLECTIONS_LIST_H
#define C_COLLECTIONS_LIST_H

#include <stddef.h>
#include "../collections_errors.h"

/* Return codes */
#define LIST_OK  COLLECTIONS_OK
#define LIST_ERR COLLECTIONS_ERR

/* Function pointer types for the destructor and foreach operations */
typedef void (*object_destructor_function_t)(void *);

/* A foreach job should return 0 to continue, or non-zero to stop the iteration
 */
typedef int (*object_job_function_t)(void *obj, void *argstruct);

/* Opaque pointer - the list structure is hidden */
typedef struct list list_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a new list.
 * @param dtor Function used to clean up list elements. Can be NULL.
 * @return Pointer to the new list, or NULL on error (sets errno = ENOMEM).
 */
list_t *list_create(object_destructor_function_t dtor);

/**
 * @brief Destroys the list, invoking the destructor on the remaining elements
 * and freeing memory.
 * @param list Pointer to the list.
 * @return LIST_OK or LIST_ERR (sets errno).
 */
int list_destroy(list_t *list);

/**
 * @brief Prepends a new element to the front of the list.
 * @param list Pointer to the list.
 * @param data Pointer to the data.
 * @return LIST_OK or LIST_ERR (sets errno).
 */
int list_add(list_t *list, void *data);

/**
 * @brief Removes the first occurrence of an element from the list, invoking its
 * destructor.
 * @param list Pointer to the list.
 * @param data Pointer to the data to remove.
 * @return LIST_OK or LIST_ERR (sets errno = ENOENT when not found).
 */
int list_remove(list_t *list, void *data);

/**
 * @brief Returns the list size via an output parameter.
 * @param list Pointer to the list.
 * @param out_size Pointer where the size will be stored.
 * @return LIST_OK or LIST_ERR (sets errno).
 */
int list_size(list_t *list, size_t *out_size);

/**
 * @brief Checks whether the list is empty via an output parameter.
 * @param list Pointer to the list.
 * @param out_is_empty 1 if empty, 0 otherwise.
 * @return LIST_OK or LIST_ERR (sets errno).
 */
int list_is_empty(list_t *list, int *out_is_empty);

/**
 * @brief Iterates over all list elements. Iteration stops when job() returns a
 * non-zero value.
 * @param list Pointer to the list.
 * @param job Function invoked on every element.
 * @param argstruct Extra argument passed to the job function.
 * @return LIST_OK or LIST_ERR (sets errno).
 */
int list_foreach(list_t *list, object_job_function_t job, void *argstruct);

#ifdef __cplusplus
}
#endif

#endif // C_COLLECTIONS_LIST_H
