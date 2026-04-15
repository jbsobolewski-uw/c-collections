//
// Created by jakub on 3/22/26.
//

#ifndef SIK_KAYLES_HASHMAP_H
#define SIK_KAYLES_HASHMAP_H


#include <stdint.h>
#include <stddef.h>

/**
 * 32-bit hashmap object
 */
typedef struct HashMap hash_map_t;


/**
 * Object destroyer function type declaration
 */
typedef void (hm_object_destroyer_func_t)(void *value);


/**
 * Creates a hashmap instance
 * @param capacity Starting capacity (will be aligned to INITIAL_CAPACITY).
 * @param destroyer Object destroyer function (can be NULL).
 */
hash_map_t *hm_create(size_t capacity, hm_object_destroyer_func_t *destroyer);

/**
 * Inserts or updates the key-value pair
 */
void hm_insert(hash_map_t *map, uint32_t key, void *value);

/**
 * Retries the object from hashmap. Returns NULL if not found.
 */
void *hm_get(hash_map_t *map, uint32_t key);

/**
 * Removes an element from the hashmap and calls its object
 * destroyer function if provided.
 */
void hm_remove(hash_map_t *map, uint32_t key);


/**
 * Clears the hashmap. If an object destroyer function was provided
 * the objects stored in the hashmap are destroyed too.
 */
void hm_destroy(hash_map_t *map);


/**
 * Return how many elements are stored in the hashmap
 */
size_t hm_size(hash_map_t *map);


#endif //SIK_KAYLES_HASHMAP_H