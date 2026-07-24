//
// Created by jakub on 3/22/26.
//

#include "id_manager.h"

#include <errno.h>
#include <stdlib.h>

/**
 * @brief Singly linked list node for the ID manager.
 * * Stores the recycled ID and a pointer to the next node.
 * Kept small and simple to be easily pooled.
 */
typedef struct id_node {
    uint32_t id; /**< The 32-bit identifier stored in this node. */
    struct id_node *next; /**< Pointer to the next node in the stack or node pool. */
} id_node_t;

/**
 * @brief Manages the generation and recycling of unique 32-bit identifiers.
 * * Uses a hybrid approach: sequentially generates new IDs using a counter,
 * and maintains a linked stack (LIFO) of released IDs to recycle them.
 * Employs a `node_pool` to recycle list nodes, avoiding frequent malloc/free
 * calls and preventing latency spikes typical of realloc-based dynamic arrays.
 */
struct IdManager {
    uint32_t min_id; /**< The lowest valid ID that can be assigned. */
    uint32_t next_id; /**< The next contiguous new ID to be issued. */
    id_node_t *recycled_ids; /**< Linked stack of released IDs waiting to be reused. */
    id_node_t *node_pool; /**< Linked stack of empty nodes ready for reuse (avoids malloc). */
    int exhausted; /**< Flag indicating if the sequence of new IDs has reached UINT32_MAX. */
};

/**
 * @brief Creates and initializes a new ID manager.
 * * Allocates memory for the manager structure. The linked stacks are initially
 * empty (NULL). Memory for nodes will be allocated lazily upon ID release.
 * * @param first_id The lowest ID number that this manager will issue.
 * @return A pointer to the newly created ID manager, or NULL if memory
 * allocation fails (sets errno = ENOMEM).
 */
id_manager_t *idm_create(uint32_t first_id) {
    id_manager_t *mgr = malloc(sizeof(id_manager_t));
    if (!mgr) {
        errno = ENOMEM;
        return NULL;
    }

    mgr->min_id = first_id;
    mgr->next_id = first_id;
    mgr->recycled_ids = NULL;
    mgr->node_pool = NULL;
    mgr->exhausted = 0;

    return mgr;
}

/**
 * @brief Destroys the ID manager and frees its associated memory.
 * * Iterates through both the stack of active recycled IDs and the pool
 * of spare nodes, safely freeing them all to prevent memory leaks.
 * * @param mgr Pointer to the ID manager to destroy.
 * @return ID_MANAGER_OK, or ID_MANAGER_ERR (sets errno = EINVAL) when mgr is NULL.
 */
int idm_destroy(id_manager_t *mgr) {
    if (!mgr) {
        errno = EINVAL;
        return ID_MANAGER_ERR;
    }

    // Free nodes holding ready-to-reuse IDs
    id_node_t *curr = mgr->recycled_ids;
    while (curr) {
        id_node_t *next = curr->next;
        free(curr);
        curr = next;
    }

    // Free spare nodes sitting in the object pool
    curr = mgr->node_pool;
    while (curr) {
        id_node_t *next = curr->next;
        free(curr);
        curr = next;
    }

    free(mgr);
    return ID_MANAGER_OK;
}

/**
 * @brief Acquires a unique ID from the manager.
 * * Time complexity: O(1).
 * Prioritizes recycled IDs. If an ID is popped from the `recycled_ids` stack,
 * the memory node is NOT freed. Instead, it is moved to the `node_pool`
 * for future use. If no recycled IDs exist, a new one is generated.
 * * @param mgr Pointer to the ID manager.
 * @return The acquired 32-bit ID, or UINT32_MAX on error (sets errno = EINVAL
 * when mgr is NULL, or ENOSPC when the ID space is exhausted). UINT32_MAX is
 * also the last valid ID; set errno to 0 before the call to tell the two apart.
 */
uint32_t idm_assign_id(id_manager_t *mgr) {
    if (!mgr) {
        errno = EINVAL;
        return UINT32_MAX;
    }

    // 1. Try to take an ID from the recycled stack
    if (mgr->recycled_ids) {
        id_node_t *node = mgr->recycled_ids;
        mgr->recycled_ids = node->next; // Pop from active stack

        uint32_t id = node->id;

        // 0 allocations: Instead of freeing, move the empty node to the pool
        node->next = mgr->node_pool;
        mgr->node_pool = node;

        return id;
    }

    // 2. Fallback: Generate a new ID from the counter
    if (mgr->exhausted) {
        errno = ENOSPC;
        return UINT32_MAX;
    }

    uint32_t id = mgr->next_id;

    if (mgr->next_id == UINT32_MAX) {
        mgr->exhausted = 1;
    } else {
        mgr->next_id++;
    }

    return id;
}

/**
 * @brief Releases a previously acquired ID back to the manager.
 * * Time complexity: O(1).
 * Recycles the ID. First checks if it can simply decrement the main counter
 * (optimization). Otherwise, it attempts to grab a spare node from the `node_pool`.
 * It only calls `malloc` if the pool is entirely empty.
 * * @param mgr Pointer to the ID manager.
 * @param id The ID to release.
 * @return ID_MANAGER_OK on success, or ID_MANAGER_ERR (sets errno) if mgr is
 * NULL or id is invalid (EINVAL), or if memory allocation fails (ENOMEM).
 */
int idm_release_id(id_manager_t *mgr, uint32_t id) {
    if (!mgr) {
        errno = EINVAL;
        return ID_MANAGER_ERR;
    }

    if (id < mgr->min_id) {
        /* An ID this manager could never have issued */
        errno = EINVAL;
        return ID_MANAGER_ERR;
    }

    // Optimization: if releasing the very last generated ID, just step back
    if (!mgr->exhausted && id + 1 == mgr->next_id) {
        mgr->next_id--;
        return ID_MANAGER_OK;
    }

    id_node_t *new_node;

    // 1. Attempt to recycle an empty node from the pool (0 allocations!)
    if (mgr->node_pool) {
        new_node = mgr->node_pool;
        mgr->node_pool = new_node->next;
    }
    // 2. Fallback: if the pool is empty, allocate a new node
    else {
        new_node = malloc(sizeof(id_node_t));
        if (!new_node) {
            errno = ENOMEM;
            return ID_MANAGER_ERR;
        }
    }

    // Push the released ID onto the active recycled_ids stack
    new_node->id = id;
    new_node->next = mgr->recycled_ids;
    mgr->recycled_ids = new_node;

    return ID_MANAGER_OK;
}

/**
 * @brief Checks if there are any IDs currently available.
 * @param mgr Pointer to the ID manager.
 * @return 1 if an ID can be acquired, 0 otherwise
 * (sets errno = EINVAL when mgr is NULL).
 */
int idm_is_available(id_manager_t *mgr) {
    if (!mgr) {
        errno = EINVAL;
        return 0;
    }
    return (mgr->recycled_ids != NULL || !mgr->exhausted) ? 1 : 0;
}