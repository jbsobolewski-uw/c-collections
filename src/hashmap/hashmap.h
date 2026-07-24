//
// Created by jakub on 3/22/26.
//

#ifndef C_COLLECTIONS_HASHMAP_H
#define C_COLLECTIONS_HASHMAP_H


#include <stdint.h>
#include <stddef.h>
#include "../collections_errors.h"

/* Return codes */
#define HASHMAP_OK  COLLECTIONS_OK
#define HASHMAP_ERR COLLECTIONS_ERR

/* Behaviour flags for hm_create */
#define HASHMAP_AUTO_SHRINK 0x1u

/**
 * 32-bit hashmap object
 */
typedef struct HashMap hash_map_t;


/**
 * Object destroyer function type declaration
 */
typedef void(hm_object_destroyer_func_t)(void *value);


/**
 * Creates a hashmap instance.
 * @param capacity Starting capacity (raised to MIN_INITIAL_CAPACITY if
 * smaller).
 * @param destroyer Object destroyer function (can be NULL).
 * @param flags Bitwise OR of behaviour flags, or 0 for defaults.
 * With HASHMAP_AUTO_SHRINK the table halves its capacity when removals
 * bring the load factor down to 1/4 (never below the minimum capacity).
 * @return Pointer to the new hashmap, or NULL on error (sets errno = ENOMEM).
 */
hash_map_t *hm_create(size_t capacity, hm_object_destroyer_func_t *destroyer,
                      unsigned flags);

/**
 * Inserts or updates the key-value pair.
 * @return HASHMAP_OK or HASHMAP_ERR (sets errno = EINVAL when map is NULL,
 * or ENOMEM when the table is full and cannot grow).
 */
int hm_insert(hash_map_t *map, uint32_t key, void *value);

/**
 * Retrieves the object from the hashmap.
 * @return Pointer to the stored value, or NULL on error
 * (sets errno = EINVAL when map is NULL, or ENOENT when the key is not found).
 */
void *hm_get(hash_map_t *map, uint32_t key);

/**
 * Removes an element from the hashmap and calls its object
 * destroyer function if provided.
 * @return HASHMAP_OK or HASHMAP_ERR (sets errno = EINVAL when map is NULL,
 * or ENOENT when the key is not found).
 */
int hm_remove(hash_map_t *map, uint32_t key);


/**
 * Destroys the hashmap. If an object destroyer function was provided
 * the objects stored in the hashmap are destroyed too.
 * @return HASHMAP_OK or HASHMAP_ERR (sets errno = EINVAL when map is NULL).
 */
int hm_destroy(hash_map_t *map);


/**
 * Returns how many elements are stored in the hashmap via an output parameter.
 * @return HASHMAP_OK or HASHMAP_ERR (sets errno = EINVAL).
 */
int hm_size(hash_map_t *map, size_t *out_size);


#endif // C_COLLECTIONS_HASHMAP_H
