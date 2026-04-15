//
// Created by jakub on 3/22/26.
//

#include "hashmap.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MIN_INITIAL_CAPACITY    64
#define HASH_32_VALUE           0x45d9f3b

/**
 * @brief Represents a single entry (key-value pair) within the hash map.
 * * This structure is used internally by the hash map to store elements.
 * It includes an occupation flag to support open addressing (linear probing).
 */
typedef struct HashMapEntry {
    uint32_t key; /**< The 32-bit unsigned integer key used for hashing. */
    void *value; /**< A pointer to the stored value associated with the key. */
    bool occupied; /**< Flag indicating whether this slot currently holds a valid entry. */
} hash_entry_t;

/**
 * @brief The main hash map structure.
 * * Implements a hash map using open addressing with linear probing.
 * It keeps track of its current capacity, number of elements (size), and an
 * optional function to safely destroy/free the stored values when they are removed.
 */
struct HashMap {
    hash_entry_t *entries; /**< Array of hash map entries. */
    size_t capacity; /**< Total number of allocated slots in the entries array. */
    size_t size; /**< Current number of elements stored in the hash map. */
    hm_object_destroyer_func_t *object_destroyer; /**< Function pointer used to free stored values. Can be NULL. */
};

/**
 * @brief Computes a 32-bit hash from a 32-bit integer.
 * * Uses a bit-mixing integer hash function (often associated with Thomas Wang's hash)
 * to ensure a good distribution of keys across the hash map.
 * * @param x The integer key to hash.
 * @return The computed 32-bit hash value.
 */
static uint32_t hash_uint32(uint32_t x) {
    x = ((x >> 16) ^ x) * HASH_32_VALUE;
    x = ((x >> 16) ^ x) * HASH_32_VALUE;
    x = (x >> 16) ^ x;
    return x;
}

/**
 * @brief Creates and initializes a new hash map.
 * * Allocates memory for the hash map structure and its internal entries array.
 * The initial capacity is bounded by MIN_INITIAL_CAPACITY.
 * * @param capacity The desired initial capacity of the hash map.
 * @param object_destroyer A function pointer to handle freeing stored values. Pass NULL if not needed.
 * @return A pointer to the newly created hash map, or NULL if memory allocation fails.
 */
hash_map_t *hm_create(size_t capacity, hm_object_destroyer_func_t object_destroyer) {
    hash_map_t *map = malloc(sizeof(hash_map_t));
    map->capacity = (capacity < MIN_INITIAL_CAPACITY) ? MIN_INITIAL_CAPACITY : capacity;
    map->size = 0;
    map->entries = calloc(map->capacity, sizeof(hash_entry_t));
    map->object_destroyer = object_destroyer;

    if (!map->entries) {
        free(map);
        return NULL;
    }

    return map;
}

/**
 * @brief Finds the correct slot index for a given key using linear probing.
 * * Scans the entries array starting from the hashed index. It stops when it finds
 * either an empty slot or a slot containing the exact matching key.
 * * @param map Pointer to the hash map.
 * @param key The key to locate.
 * @return The index of the array slot where the key resides or where it should be inserted.
 */
static size_t hm_find_slot(hash_map_t *map, uint32_t key) {
    size_t index = hash_uint32(key) % map->capacity;

    while (map->entries[index].occupied) {
        if (map->entries[index].key == key) {
            return index; // Klucz już istnieje
        }
        index = (index + 1) % map->capacity; // Próbkowanie liniowe
    }
    return index; // Pusty slot
}

/**
 * @brief Doubles the capacity of the hash map and rehashes all existing entries.
 * * Called automatically during insertion when the load factor exceeds 3/4.
 * * @param map Pointer to the hash map to resize.
 */
static void hm_resize(hash_map_t *map) {
    size_t old_capacity = map->capacity;
    hash_entry_t *old_entries = map->entries;

    map->capacity *= 2;
    map->entries = calloc(map->capacity, sizeof(hash_entry_t));
    map->size = 0;

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].occupied) {
            hm_insert(map, old_entries[i].key, old_entries[i].value);
        }
    }
    free(old_entries);
}

/**
 * @brief Inserts a key-value pair into the hash map.
 * * If the key already exists, the old value is replaced (and destroyed if an
 * object_destroyer was provided). The map will automatically resize if the load
 * factor threshold is reached.
 * * @param map Pointer to the hash map.
 * @param key The 32-bit integer key.
 * @param value Pointer to the value to store.
 */
void hm_insert(hash_map_t *map, uint32_t key, void *value) {
    if (!map) return;

    if (map->size >= (map->capacity * 3) / 4) hm_resize(map);

    size_t index = hm_find_slot(map, key);
    if (!map->entries[index].occupied) {
        map->entries[index].occupied = true;
        map->entries[index].key = key;
        map->size++;
    } else if (map->object_destroyer) map->object_destroyer(map->entries[index].value);

    map->entries[index].value = value;
}

/**
 * @brief Retrieves a value from the hash map by its key.
 * * @param map Pointer to the hash map.
 * @param key The key to search for.
 * @return Pointer to the stored value, or NULL if the key is not found.
 */
void *hm_get(hash_map_t *map, uint32_t key) {
    if (!map) return NULL;

    size_t index = hash_uint32(key) % map->capacity;
    size_t start_index = index;

    while (map->entries[index].occupied) {
        if (map->entries[index].key == key) {
            return map->entries[index].value;
        }
        index = (index + 1) % map->capacity;
        if (index == start_index) break; // Przeszukano całą tabelę
    }
    return NULL;
}

/**
 * @brief Removes a key-value pair from the hash map.
 * * Frees the associated value (if object_destroyer is set) and performs
 * cluster rehashing to maintain the integrity of the linear probing chains.
 * * @param map Pointer to the hash map.
 * @param key The key of the entry to remove.
 */
void hm_remove(hash_map_t *map, uint32_t key) {
    if (!map) return;

    size_t i = hash_uint32(key) % map->capacity;
    while (map->entries[i].occupied) {
        if (map->entries[i].key == key) break;
        i = (i + 1) % map->capacity;
    }

    if (!map->entries[i].occupied) return;

    if (map->object_destroyer && map->entries[i].value) {
        map->object_destroyer(map->entries[i].value);
    }

    map->entries[i].occupied = false;
    map->entries[i].value = NULL;
    map->size--;

    // Rehash clustera
    size_t j = i;
    while (true) {
        j = (j + 1) % map->capacity;
        if (!map->entries[j].occupied) break;

        uint32_t k = map->entries[j].key;
        void *v = map->entries[j].value;

        // Ważne: usuwamy bez wywoływania destruktora, bo tylko przesuwamy
        map->entries[j].occupied = false;
        map->size--;

        hm_insert(map, k, v);
    }
}

/**
 * @brief Destroys the hash map and frees all associated memory.
 * * Iterates through all entries and safely destroys their values using
 * the provided object_destroyer function before freeing the internal arrays.
 * * @param map Pointer to the hash map to destroy.
 */
void hm_destroy(hash_map_t *map) {
    if (!map) return;

    if (map->object_destroyer) {
        for (size_t i = 0; i < map->capacity; i++) {
            if (map->entries[i].occupied && map->entries[i].value) {
                map->object_destroyer(map->entries[i].value);
                map->entries[i].occupied = false;
                map->entries[i].value = NULL;
                map->size--;
            }
        }
    }

    free(map->entries);
    free(map);
}

/**
 * @brief Returns the current number of elements stored in the hash map.
 * * @param map Pointer to the hash map.
 * @return The number of elements currently in the map. Returns 0 if map is NULL.
 */
size_t hm_size(hash_map_t *map) {
    if (!map) return 0;
    return map->size;
}