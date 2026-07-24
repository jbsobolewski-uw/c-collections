//
// Test suite for the id_manager module.
//
// Test cases:
//   api        - constructor variants, sequential assignment, availability,
//                release/reassign round trip, destruction.
//   edge       - NULL-manager handling for every function and rejection of
//                ids below first_id (which must not enter the recycle pool).
//   recycling  - LIFO reuse of released ids, the last-issued counter
//                optimization, stack-over-counter precedence and a large
//                release/reassign cycle preserving the id set.
//   exhaustion - draining the id space up to UINT32_MAX, ENOSPC reporting,
//                the UINT32_MAX sentinel/errno discipline and recovery
//                after a release.
//   memory     - allocation-failure injection over both allocation sites
//                (idm_create, idm_release_id) plus the zero-allocation
//                steady state of pooled nodes.
//

#include "collections_tests.h"
#include "../src/id_manager/id_manager.h"

#include <stdbool.h>
#include <stdint.h>

// Tests basic manager life cycle and sequential id assignment.
static int api(void) {
    static const uint32_t first_ids[] = {0u, 1u, 100u};

    for (size_t f = 0; f < SIZE(first_ids); ++f) {
        uint32_t first = first_ids[f];

        id_manager_t *m = idm_create(first);
        ASSERT(m != NULL);

        // A fresh manager always has ids available.
        errno = 0;
        ASSERT(idm_is_available(m) == 1);
        ASSERT(errno == 0);

        // New ids are issued sequentially starting at first_id.
        for (uint32_t i = 0; i < 5u; ++i) {
            errno = 0;
            ASSERT(idm_assign_id(m) == first + i);
            ASSERT(errno == 0);
        }

        // Release one id and get it back on the next assignment.
        ASSERT(idm_release_id(m, first + 2u) == ID_MANAGER_OK);
        ASSERT(idm_is_available(m) == 1);
        errno = 0;
        ASSERT(idm_assign_id(m) == first + 2u);
        ASSERT(errno == 0);

        // Sequential generation resumes after the recycled id is used up.
        ASSERT(idm_assign_id(m) == first + 5u);

        ASSERT(idm_destroy(m) == ID_MANAGER_OK);
    }

    return PASS;
}


// Tests NULL-manager handling and rejection of ids below first_id.
static int edge(void) {
    // Every function must reject a NULL manager with EINVAL.
    errno = 0;
    ASSERT(idm_assign_id(NULL) == UINT32_MAX);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(idm_release_id(NULL, 0u) == ID_MANAGER_ERR);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(idm_is_available(NULL) == 0);
    ASSERT(errno == EINVAL);

    errno = 0;
    ASSERT(idm_destroy(NULL) == ID_MANAGER_ERR);
    ASSERT(errno == EINVAL);

    // An id below first_id could never have been issued by this manager.
    id_manager_t *m = idm_create(100u);
    ASSERT(m != NULL);

    errno = 0;
    ASSERT(idm_release_id(m, 99u) == ID_MANAGER_ERR);
    ASSERT(errno == EINVAL);
    errno = 0;
    ASSERT(idm_release_id(m, 0u) == ID_MANAGER_ERR);
    ASSERT(errno == EINVAL);

    // The rejected ids must not have entered the recycle pool: the next
    // assignments have to produce the plain sequential values.
    errno = 0;
    ASSERT(idm_assign_id(m) == 100u);
    ASSERT(errno == 0);
    ASSERT(idm_assign_id(m) == 101u);

    // Same after assignments have been made.
    errno = 0;
    ASSERT(idm_release_id(m, 42u) == ID_MANAGER_ERR);
    ASSERT(errno == EINVAL);
    ASSERT(idm_assign_id(m) == 102u);

    ASSERT(idm_destroy(m) == ID_MANAGER_OK);
    return PASS;
}


// Tests LIFO recycling, the last-issued optimization and a large cycle.
static int recycling(void) {
    enum {
        CYCLE = 1000
    };

    static bool seen[CYCLE];

    id_manager_t *m = idm_create(100u);
    ASSERT(m != NULL);

    for (uint32_t i = 0; i < 6u; ++i) ASSERT(idm_assign_id(m) == 100u + i);

    // Release a, b, c (none of them the newest id) - reassignment must yield
    // c, b, a (LIFO) before any new sequential id.
    ASSERT(idm_release_id(m, 100u) == ID_MANAGER_OK);
    ASSERT(idm_release_id(m, 101u) == ID_MANAGER_OK);
    ASSERT(idm_release_id(m, 102u) == ID_MANAGER_OK);
    ASSERT(idm_assign_id(m) == 102u);
    ASSERT(idm_assign_id(m) == 101u);
    ASSERT(idm_assign_id(m) == 100u);
    ASSERT(idm_assign_id(m) == 106u);
    ASSERT(idm_destroy(m) == ID_MANAGER_OK);

    // Last-issued optimization: releasing the newest id steps the counter
    // back, so the very same id is issued again.
    m = idm_create(100u);
    ASSERT(m != NULL);
    ASSERT(idm_assign_id(m) == 100u);
    ASSERT(idm_assign_id(m) == 101u);
    ASSERT(idm_release_id(m, 101u) == ID_MANAGER_OK);
    errno = 0;
    ASSERT(idm_assign_id(m) == 101u);
    ASSERT(errno == 0);
    ASSERT(idm_assign_id(m) == 102u);

    // Interleave: release the newest id (counter path) and an older id
    // (recycle stack); the stack takes precedence over the counter.
    for (uint32_t i = 3u; i < 6u; ++i) ASSERT(idm_assign_id(m) == 100u + i);
    ASSERT(idm_release_id(m, 105u) == ID_MANAGER_OK); // newest -> counter
    ASSERT(idm_release_id(m, 103u) == ID_MANAGER_OK); // older  -> stack
    ASSERT(idm_assign_id(m) == 103u);                 // stack first
    ASSERT(idm_assign_id(m) == 105u);                 // then the counter
    ASSERT(idm_destroy(m) == ID_MANAGER_OK);

    // Large cycle: assign CYCLE ids, release them all in a mixed order,
    // reassign CYCLE ids - the resulting id set must be identical.
    m = idm_create(100u);
    ASSERT(m != NULL);

    for (uint32_t i = 0; i < (uint32_t) CYCLE; ++i)
        ASSERT(idm_assign_id(m) == 100u + i);

    // Even offsets ascending, then odd offsets descending; the very last
    // release (offset 1) exercises the stack while earlier ones near the
    // top exercise the counter-decrement path as well.
    for (uint32_t i = 0; i < (uint32_t) CYCLE; i += 2u)
        ASSERT(idm_release_id(m, 100u + i) == ID_MANAGER_OK);
    for (uint32_t k = 0; k < (uint32_t) CYCLE / 2u; ++k)
        ASSERT(idm_release_id(m, 100u + ((uint32_t) CYCLE - 1u - 2u * k)) ==
               ID_MANAGER_OK);

    for (size_t i = 0; i < (size_t) CYCLE; ++i) seen[i] = false;
    for (uint32_t i = 0; i < (uint32_t) CYCLE; ++i) {
        errno       = 0;
        uint32_t id = idm_assign_id(m);
        ASSERT(errno == 0);
        ASSERT(id >= 100u && id < 100u + (uint32_t) CYCLE);
        ASSERT(!seen[id - 100u]); // no duplicates
        seen[id - 100u] = true;
    }
    for (size_t i = 0; i < (size_t) CYCLE; ++i)
        ASSERT(seen[i]); // every id came back exactly once

    // Nothing recycled is left over: the next id is a fresh sequential one.
    ASSERT(idm_assign_id(m) == 100u + (uint32_t) CYCLE);

    ASSERT(idm_destroy(m) == ID_MANAGER_OK);
    return PASS;
}


// Tests draining the id space, ENOSPC reporting and recovery on release.
static int exhaustion(void) {
    id_manager_t *m = idm_create(UINT32_MAX - 2u);
    ASSERT(m != NULL);

    errno = 0;
    ASSERT(idm_assign_id(m) == UINT32_MAX - 2u);
    ASSERT(errno == 0);
    errno = 0;
    ASSERT(idm_assign_id(m) == UINT32_MAX - 1u);
    ASSERT(errno == 0);
    ASSERT(idm_is_available(m) == 1);

    // UINT32_MAX is the last valid id, not an error: errno stays 0. This is
    // the disambiguation discipline documented in docs/id_manager.md.
    errno       = 0;
    uint32_t id = idm_assign_id(m);
    ASSERT(id == UINT32_MAX);
    ASSERT(errno == 0);

    // The space is now exhausted.
    errno = 0;
    ASSERT(idm_is_available(m) == 0);
    ASSERT(errno == 0);
    errno = 0;
    id    = idm_assign_id(m);
    ASSERT(id == UINT32_MAX);
    ASSERT(errno == ENOSPC);

    // Releasing an id makes the manager available again and the released id
    // is exactly what the next assignment yields.
    ASSERT(idm_release_id(m, UINT32_MAX - 1u) == ID_MANAGER_OK);
    ASSERT(idm_is_available(m) == 1);
    errno = 0;
    ASSERT(idm_assign_id(m) == UINT32_MAX - 1u);
    ASSERT(errno == 0);

    // Drained again: ENOSPC once more.
    ASSERT(idm_is_available(m) == 0);
    errno = 0;
    id    = idm_assign_id(m);
    ASSERT(id == UINT32_MAX);
    ASSERT(errno == ENOSPC);

    ASSERT(idm_destroy(m) == ID_MANAGER_OK);
    return PASS;
}


// Allocation-failure scenario. Sites: idm_create (one malloc) and
// idm_release_id when the node pool is empty. idm_assign_id and idm_destroy
// never allocate, and steady-state release/assign pairs reuse pooled nodes,
// so every step besides the two sites must succeed unconditionally.
static unsigned long scenario(void) {
    unsigned long visited = 0;
    id_manager_t *m;

    // Site A: idm_create.
    errno = 0;
    if ((m = idm_create(100u)) != NULL) visited |= V(1, 0);
    else if (errno == ENOMEM && (m = idm_create(100u)) != NULL)
        visited |= V(2, 0);
    else return visited | V(4, 0);

    // Assignment never allocates: 100..104.
    for (uint32_t i = 0; i < 5u; ++i) {
        errno = 0;
        if (idm_assign_id(m) != 100u + i) {
            idm_destroy(m);
            return visited | V(4, 1);
        }
    }
    visited |= V(1, 1);

    // Releasing the NEWEST id takes the counter-decrement path: no node is
    // needed, so this cannot fail.
    errno = 0;
    if (idm_release_id(m, 104u) != ID_MANAGER_OK) {
        idm_destroy(m);
        return visited | V(4, 2);
    }
    visited |= V(1, 2);

    errno = 0;
    if (idm_assign_id(m) != 104u) {
        idm_destroy(m);
        return visited | V(4, 3);
    }
    visited |= V(1, 3);

    // Site B: releasing an OLD id with an empty node pool allocates a node.
    errno = 0;
    if (idm_release_id(m, 100u) == ID_MANAGER_OK) visited |= V(1, 4);
    else if (errno == ENOMEM && idm_release_id(m, 100u) == ID_MANAGER_OK)
        visited |= V(2, 4);
    else {
        idm_destroy(m);
        return visited | V(4, 4);
    }

    // Assigning the recycled id moves its node into the pool.
    errno = 0;
    if (idm_assign_id(m) != 100u) {
        idm_destroy(m);
        return visited | V(4, 5);
    }
    visited |= V(1, 5);

    // Zero-allocation steady state: releasing another old id reuses the
    // pooled node - it cannot fail.
    errno = 0;
    if (idm_release_id(m, 101u) != ID_MANAGER_OK) {
        idm_destroy(m);
        return visited | V(4, 6);
    }
    visited |= V(1, 6);

    errno = 0;
    if (idm_assign_id(m) != 101u) {
        idm_destroy(m);
        return visited | V(4, 7);
    }
    visited |= V(1, 7);

    // Pool holds one node again: this release must succeed without malloc.
    errno = 0;
    if (idm_release_id(m, 100u) != ID_MANAGER_OK) {
        idm_destroy(m);
        return visited | V(4, 8);
    }
    visited |= V(1, 8);

    // Site B again: the pool is empty now, so a second old release allocates.
    errno = 0;
    if (idm_release_id(m, 102u) == ID_MANAGER_OK) visited |= V(1, 9);
    else if (errno == ENOMEM && idm_release_id(m, 102u) == ID_MANAGER_OK)
        visited |= V(2, 9);
    else {
        idm_destroy(m);
        return visited | V(4, 9);
    }

    // Drain the recycle stack in LIFO order; no allocations involved.
    errno = 0;
    if (idm_assign_id(m) != 102u || idm_assign_id(m) != 100u) {
        idm_destroy(m);
        return visited | V(4, 10);
    }
    visited |= V(1, 10);

    // Destruction never allocates and frees the two pooled nodes.
    errno = 0;
    if (idm_destroy(m) != ID_MANAGER_OK) return visited | V(4, 11);
    visited |= V(1, 11);

    return visited;
}


static int memory(void) {
    memory_tests_check();
    return memory_test(scenario);
}


static const test_list_t test_list[] = {
        TEST(api), TEST(edge), TEST(recycling), TEST(exhaustion), TEST(memory),
};

int main(int argc, char *argv[]) {
    return run_test_main(argc, argv, test_list, SIZE(test_list));
}
