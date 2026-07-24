//
// Created by jakub on 4/13/26.
//

#ifndef C_COLLECTIONS_QUEUE_H
#define C_COLLECTIONS_QUEUE_H

#include <stddef.h>
#include "../collections_errors.h"

/* Return codes */
#define QUEUE_OK  COLLECTIONS_OK
#define QUEUE_ERR COLLECTIONS_ERR

/* Function pointer types */
typedef void (*object_destructor_function_t)(void *);

typedef int (*object_job_function_t)(void *obj, void *argstruct);

/* Opaque pointer - the queue structure is hidden */
typedef struct queue queue_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a new queue.
 * @param dtor Function used to clean up queue elements when the queue is destroyed. Can be NULL.
 * @return Pointer to the new queue, or NULL on error (sets errno = ENOMEM).
 */
queue_t *queue_create(object_destructor_function_t dtor);

/**
 * @brief Destroys the queue, invoking the destructor on the remaining elements and freeing memory.
 * @param q Pointer to the queue.
 * @return QUEUE_OK or QUEUE_ERR (sets errno).
 */
int queue_destroy(queue_t *q);

/**
 * @brief Appends a new element to the back of the queue (enqueue).
 * @param q Pointer to the queue.
 * @param data Pointer to the data.
 * @return QUEUE_OK or QUEUE_ERR (sets errno).
 */
int queue_enqueue(queue_t *q, void *data);

/**
 * @brief Removes the element at the front of the queue (dequeue).
 * @param q Pointer to the queue.
 * @param out_data Location where the pointer to the removed data is stored.
 * If NULL is passed, the data is destroyed using the destructor.
 * @return QUEUE_OK or QUEUE_ERR (sets errno = ENOENT if the queue is empty).
 */
int queue_dequeue(queue_t *q, void **out_data);

/**
 * @brief Returns the queue size via an output parameter.
 * @param q Pointer to the queue.
 * @param out_size Pointer where the size will be stored.
 * @return QUEUE_OK or QUEUE_ERR (sets errno).
 */
int queue_size(queue_t *q, size_t *out_size);

/**
 * @brief Checks whether the queue is empty via an output parameter.
 * @param q Pointer to the queue.
 * @param out_is_empty 1 if empty, 0 otherwise.
 * @return QUEUE_OK or QUEUE_ERR (sets errno).
 */
int queue_is_empty(queue_t *q, int *out_is_empty);

/**
 * @brief Iterates over all queue elements (from the front to the back).
 * @param q Pointer to the queue.
 * @param job Function invoked on every element (a non-zero return value stops the iteration).
 * @param argstruct Extra argument passed to the job function.
 * @return QUEUE_OK or QUEUE_ERR (sets errno).
 */
int queue_foreach(queue_t *q, object_job_function_t job, void *argstruct);

#ifdef __cplusplus
}
#endif

#endif //C_COLLECTIONS_QUEUE_H
