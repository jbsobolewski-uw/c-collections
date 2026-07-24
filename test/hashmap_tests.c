//
// Tests for the hashmap module (src/hashmap).
//
// Test cases: api, edge, ownership, collisions, growth, memory.
//

#include "collections_tests.h"
#include "../src/hashmap/hashmap.h"

#include <stdint.h>
#include <stdlib.h>

// Basic lifecycle: create/insert/get/remove/destroy/size, value replacement
// on repeated insert, stored NULL value vs. missing key, extreme keys.
static int api(void) {
  size_t size = 12345;

  hash_map_t *map = hm_create(64, free, 0);
  ASSERT(map != NULL);

  ASSERT(hm_size(map, &size) == HASHMAP_OK);
  ASSERT(size == 0);

  int *v1 = malloc(sizeof *v1);
  ASSERT(v1 != NULL);
  *v1 = 111;
  ASSERT(hm_insert(map, 42, v1) == HASHMAP_OK);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 1);
  ASSERT(hm_get(map, 42) == v1);

  // Inserting an existing key replaces the value (old one is destroyed
  // by the destroyer) and leaves the size unchanged.
  int *v2 = malloc(sizeof *v2);
  ASSERT(v2 != NULL);
  *v2 = 222;
  ASSERT(hm_insert(map, 42, v2) == HASHMAP_OK);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 1);
  ASSERT(hm_get(map, 42) == v2);

  // A stored NULL value is returned with errno untouched, while a missing
  // key yields NULL with errno == ENOENT.
  ASSERT(hm_insert(map, 7, NULL) == HASHMAP_OK);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 2);
  errno = 0;
  ASSERT(hm_get(map, 7) == NULL);
  ASSERT(errno == 0);
  errno = 0;
  ASSERT(hm_get(map, 12345) == NULL);
  ASSERT(errno == ENOENT);

  // Extreme keys.
  int *v0 = malloc(sizeof *v0);
  int *vmax = malloc(sizeof *vmax);
  ASSERT(v0 != NULL && vmax != NULL);
  *v0 = 13;
  *vmax = -1;
  ASSERT(hm_insert(map, 0, v0) == HASHMAP_OK);
  ASSERT(hm_insert(map, UINT32_MAX, vmax) == HASHMAP_OK);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 4);
  ASSERT(hm_get(map, 0) == v0);
  ASSERT(hm_get(map, UINT32_MAX) == vmax);

  ASSERT(hm_remove(map, 0) == HASHMAP_OK);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 3);
  errno = 0;
  ASSERT(hm_get(map, 0) == NULL && errno == ENOENT);
  ASSERT(hm_remove(map, UINT32_MAX) == HASHMAP_OK);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 2);

  ASSERT(hm_destroy(map) == HASHMAP_OK);
  return PASS;
}

// Invalid arguments and boundary conditions.
static int edge(void) {
  size_t size = 0;

  // NULL map for every function.
  errno = 0;
  ASSERT(hm_insert(NULL, 1, NULL) == HASHMAP_ERR);
  ASSERT(errno == EINVAL);
  errno = 0;
  ASSERT(hm_get(NULL, 1) == NULL);
  ASSERT(errno == EINVAL);
  errno = 0;
  ASSERT(hm_remove(NULL, 1) == HASHMAP_ERR);
  ASSERT(errno == EINVAL);
  errno = 0;
  ASSERT(hm_size(NULL, &size) == HASHMAP_ERR);
  ASSERT(errno == EINVAL);
  errno = 0;
  ASSERT(hm_destroy(NULL) == HASHMAP_ERR);
  ASSERT(errno == EINVAL);

  // Capacity 0 is raised to the internal minimum; the map works normally.
  static int values[5];
  hash_map_t *map = hm_create(0, NULL, 0);
  ASSERT(map != NULL);

  errno = 0;
  ASSERT(hm_size(map, NULL) == HASHMAP_ERR);
  ASSERT(errno == EINVAL);

  for (uint32_t i = 0; i < 5; ++i) {
    values[i] = (int) i;
    ASSERT(hm_insert(map, i, &values[i]) == HASHMAP_OK);
  }
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 5);
  for (uint32_t i = 0; i < 5; ++i)
    ASSERT(hm_get(map, i) == &values[i]);

  // Removing an absent key; removing the same key twice.
  errno = 0;
  ASSERT(hm_remove(map, 1000) == HASHMAP_ERR);
  ASSERT(errno == ENOENT);
  ASSERT(hm_remove(map, 3) == HASHMAP_OK);
  errno = 0;
  ASSERT(hm_remove(map, 3) == HASHMAP_ERR);
  ASSERT(errno == ENOENT);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 4);

  ASSERT(hm_destroy(map) == HASHMAP_OK);
  return PASS;
}

static int destroyed_count;
static void *last_destroyed;

static void counting_destroyer(void *value) {
  ++destroyed_count;
  last_destroyed = value;
}

// The destroyer runs exactly once per value on overwrite, remove and destroy;
// with a NULL destroyer the values are left untouched.
static int ownership(void) {
  static int a, b, c, d;
  size_t size = 0;

  destroyed_count = 0;
  last_destroyed = NULL;

  hash_map_t *map = hm_create(64, counting_destroyer, 0);
  ASSERT(map != NULL);

  ASSERT(hm_insert(map, 1, &a) == HASHMAP_OK);
  ASSERT(destroyed_count == 0);

  // Overwriting an existing key destroys exactly the old value.
  ASSERT(hm_insert(map, 1, &b) == HASHMAP_OK);
  ASSERT(destroyed_count == 1 && last_destroyed == &a);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 1);
  ASSERT(hm_get(map, 1) == &b);

  // hm_remove destroys the stored value.
  ASSERT(hm_insert(map, 2, &c) == HASHMAP_OK);
  ASSERT(hm_remove(map, 1) == HASHMAP_OK);
  ASSERT(destroyed_count == 2 && last_destroyed == &b);

  // A failed remove does not invoke the destroyer.
  errno = 0;
  ASSERT(hm_remove(map, 999) == HASHMAP_ERR && errno == ENOENT);
  ASSERT(destroyed_count == 2);

  // hm_destroy runs the destroyer once per remaining value.
  ASSERT(hm_insert(map, 3, &d) == HASHMAP_OK);
  ASSERT(hm_destroy(map) == HASHMAP_OK);
  ASSERT(destroyed_count == 4);

  // NULL destroyer: static values stay untouched through overwrite,
  // remove and destroy.
  map = hm_create(64, NULL, 0);
  ASSERT(map != NULL);
  a = 17;
  ASSERT(hm_insert(map, 1, &a) == HASHMAP_OK);
  ASSERT(hm_insert(map, 1, &b) == HASHMAP_OK);
  ASSERT(hm_get(map, 1) == &b);
  ASSERT(hm_insert(map, 2, &c) == HASHMAP_OK);
  ASSERT(hm_remove(map, 2) == HASHMAP_OK);
  ASSERT(hm_destroy(map) == HASHMAP_OK);
  ASSERT(destroyed_count == 4);
  ASSERT(a == 17);

  return PASS;
}

// Mirrors the private hash function of the implementation
// (hash_uint32 in src/hashmap/hashmap.c) so the test can construct
// keys that collide in the table.
static uint32_t test_hash(uint32_t x) {
  x = ((x >> 16) ^ x) * 0x45d9f3bu;
  x = ((x >> 16) ^ x) * 0x45d9f3bu;
  x = (x >> 16) ^ x;
  return x;
}

#define COLLIDING_KEYS 7
#define TEST_CAPACITY 64u

// Linear-probing clusters: colliding keys are all retrievable, and removing
// from the middle/front/back of a cluster keeps the rest reachable
// (exercising the cluster rehash in hm_remove).
static int collisions(void) {
  uint32_t keys[COLLIDING_KEYS];
  static int values[COLLIDING_KEYS];
  size_t size = 0;

  // Brute-force distinct keys that land in the same slot for capacity 64.
  size_t slot = (size_t) test_hash(0) % TEST_CAPACITY;
  size_t found = 0;
  for (uint32_t k = 0; found < COLLIDING_KEYS && k < 100000; ++k) {
    if ((size_t) test_hash(k) % TEST_CAPACITY == slot)
      keys[found++] = k;
  }
  ASSERT(found == COLLIDING_KEYS);

  hash_map_t *map = hm_create(TEST_CAPACITY, NULL, 0);
  ASSERT(map != NULL);

  // Insert all but the last colliding key (well below the resize
  // threshold of 48 entries, so the capacity stays at 64).
  for (size_t i = 0; i + 1 < COLLIDING_KEYS; ++i) {
    values[i] = (int) (i + 1);
    ASSERT(hm_insert(map, keys[i], &values[i]) == HASHMAP_OK);
  }
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == COLLIDING_KEYS - 1);
  for (size_t i = 0; i + 1 < COLLIDING_KEYS; ++i)
    ASSERT(hm_get(map, keys[i]) == &values[i]);

  // A colliding key that was never inserted reports ENOENT.
  errno = 0;
  ASSERT(hm_get(map, keys[COLLIDING_KEYS - 1]) == NULL && errno == ENOENT);
  errno = 0;
  ASSERT(hm_remove(map, keys[COLLIDING_KEYS - 1]) == HASHMAP_ERR &&
         errno == ENOENT);

  // Remove from the middle of the probe cluster; the rest must survive.
  ASSERT(hm_remove(map, keys[2]) == HASHMAP_OK);
  errno = 0;
  ASSERT(hm_get(map, keys[2]) == NULL && errno == ENOENT);
  for (size_t i = 0; i + 1 < COLLIDING_KEYS; ++i)
    if (i != 2)
      ASSERT(hm_get(map, keys[i]) == &values[i]);

  // Remove the first and the last key of the cluster too.
  ASSERT(hm_remove(map, keys[0]) == HASHMAP_OK);
  ASSERT(hm_remove(map, keys[5]) == HASHMAP_OK);
  errno = 0;
  ASSERT(hm_get(map, keys[0]) == NULL && errno == ENOENT);
  errno = 0;
  ASSERT(hm_get(map, keys[5]) == NULL && errno == ENOENT);
  ASSERT(hm_get(map, keys[1]) == &values[1]);
  ASSERT(hm_get(map, keys[3]) == &values[3]);
  ASSERT(hm_get(map, keys[4]) == &values[4]);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 3);

  ASSERT(hm_destroy(map) == HASHMAP_OK);
  return PASS;
}

#define GROWTH_KEYS 1000u
#define GROWTH_EXTRA 100u

// Multiple capacity doublings (threshold is 3/4 of the capacity, starting
// at 64): every key still maps to its exact value after growth, removals
// leave the rest intact, and insertion keeps working after removals.
static int growth(void) {
  static int values[GROWTH_KEYS + GROWTH_EXTRA];
  size_t size = 0;

  hash_map_t *map = hm_create(64, NULL, 0);
  ASSERT(map != NULL);

  for (uint32_t i = 0; i < GROWTH_KEYS; ++i) {
    values[i] = (int) i;
    ASSERT(hm_insert(map, i, &values[i]) == HASHMAP_OK);
  }
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == GROWTH_KEYS);
  for (uint32_t i = 0; i < GROWTH_KEYS; ++i)
    ASSERT(hm_get(map, i) == &values[i]);

  // Remove every third key.
  size_t removed = 0;
  for (uint32_t i = 0; i < GROWTH_KEYS; i += 3) {
    ASSERT(hm_remove(map, i) == HASHMAP_OK);
    ++removed;
  }
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == GROWTH_KEYS - removed);
  for (uint32_t i = 0; i < GROWTH_KEYS; ++i) {
    if (i % 3 == 0) {
      errno = 0;
      ASSERT(hm_get(map, i) == NULL && errno == ENOENT);
    } else {
      ASSERT(hm_get(map, i) == &values[i]);
    }
  }

  // Inserting after removals still works.
  for (uint32_t i = GROWTH_KEYS; i < GROWTH_KEYS + GROWTH_EXTRA; ++i) {
    values[i] = (int) i;
    ASSERT(hm_insert(map, i, &values[i]) == HASHMAP_OK);
  }
  ASSERT(hm_size(map, &size) == HASHMAP_OK &&
         size == GROWTH_KEYS - removed + GROWTH_EXTRA);
  for (uint32_t i = GROWTH_KEYS; i < GROWTH_KEYS + GROWTH_EXTRA; ++i)
    ASSERT(hm_get(map, i) == &values[i]);
  for (uint32_t i = 1; i < GROWTH_KEYS; i += 3)
    ASSERT(hm_get(map, i) == &values[i]);

  ASSERT(hm_destroy(map) == HASHMAP_OK);
  return PASS;
}

#define SHRINK_KEYS 2000u
#define SHRINK_KEEP 100u

// HASHMAP_AUTO_SHRINK trades memory for removal latency: with the flag set
// the table halves once the load factor drops to 1/4; without it removals
// never reallocate. Observed through the wrap harness counters -
// alloc_counter only moves when a rehash allocates a new table.
static int shrink(void) {
  static int values[SHRINK_KEYS];
  memory_test_data_t *mtd = get_memory_test_data();
  size_t size = 0;

  // Default map (flags 0): draining it must never allocate.
  hash_map_t *map = hm_create(64, NULL, 0);
  ASSERT(map != NULL);
  for (uint32_t i = 0; i < SHRINK_KEYS; ++i) {
    values[i] = (int) i;
    ASSERT(hm_insert(map, i, &values[i]) == HASHMAP_OK);
  }
  unsigned allocs_before = mtd->alloc_counter;
  for (uint32_t i = 0; i < SHRINK_KEYS; ++i)
    ASSERT(hm_remove(map, i) == HASHMAP_OK);
  ASSERT(mtd->alloc_counter == allocs_before);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == 0);
  ASSERT(hm_destroy(map) == HASHMAP_OK);

  // Auto-shrink map: draining it rehashes into smaller tables, and every
  // surviving entry stays intact across the shrinks.
  map = hm_create(64, NULL, HASHMAP_AUTO_SHRINK);
  ASSERT(map != NULL);
  for (uint32_t i = 0; i < SHRINK_KEYS; ++i)
    ASSERT(hm_insert(map, i, &values[i]) == HASHMAP_OK);

  allocs_before = mtd->alloc_counter;
  for (uint32_t i = 0; i < SHRINK_KEYS - SHRINK_KEEP; ++i) {
    ASSERT(hm_remove(map, i) == HASHMAP_OK);
    if (i % 500 == 0) {
      for (uint32_t j = i + 1; j < SHRINK_KEYS; ++j)
        ASSERT(hm_get(map, j) == &values[j]);
    }
  }
  ASSERT(mtd->alloc_counter > allocs_before); /* shrinking actually rehashed */
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == SHRINK_KEEP);
  for (uint32_t j = SHRINK_KEYS - SHRINK_KEEP; j < SHRINK_KEYS; ++j)
    ASSERT(hm_get(map, j) == &values[j]);

  // The map keeps working after shrinking: grow it right back up.
  for (uint32_t i = 0; i < 500; ++i)
    ASSERT(hm_insert(map, i, &values[i]) == HASHMAP_OK);
  ASSERT(hm_size(map, &size) == HASHMAP_OK && size == SHRINK_KEEP + 500);
  for (uint32_t i = 0; i < 500; ++i)
    ASSERT(hm_get(map, i) == &values[i]);

  ASSERT(hm_destroy(map) == HASHMAP_OK);
  return PASS;
}

// Allocation-failure scenario for memory_test(). Heap ints are stored with
// free as the destroyer, so every path must end in hm_destroy to keep
// alloc_counter == free_counter.
//
// Note on hm_insert: a failed internal resize is tolerated by the
// implementation - the insert can succeed even though the injected failure
// hit the resize calloc (the map is merely left at its old capacity). So a
// plain success is recorded as V(1, i) even on the runs where the injection
// hit the resize; V(2, i) covers an ENOMEM followed by a successful retry
// (e.g. the malloc of the value itself failing).
static unsigned long scenario(void) {
  unsigned long visited = 0;
  hash_map_t *map;
  size_t size = 0;

  // hm_create allocates twice (struct + entries array).
  errno = 0;
  if ((map = hm_create(64, free, 0)) != NULL)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (map = hm_create(64, free, 0)) != NULL)
    visited |= V(2, 0);
  else
    return visited | V(4, 0);

  // 50 inserts (crossing the resize threshold of 48 with capacity 64,
  // so the sweep also hits the resize calloc), batched in groups of 5
  // under indices 1..10.
  for (uint32_t key = 0; key < 50; ++key) {
    unsigned where = 1u + key / 5u;
    int *val;

    errno = 0;
    val = malloc(sizeof *val);
    if (!val) {
      if (errno != ENOMEM || (val = malloc(sizeof *val)) == NULL) {
        hm_destroy(map);
        return visited | V(4, where);
      }
      visited |= V(2, where);
    }
    *val = (int) key + 1000;

    errno = 0;
    if (hm_insert(map, key, val) == HASHMAP_OK)
      visited |= V(1, where);
    else if (errno == ENOMEM && hm_insert(map, key, val) == HASHMAP_OK)
      visited |= V(2, where);
    else {
      free(val);
      hm_destroy(map);
      return visited | V(4, where);
    }
  }

  // Non-allocating queries.
  if (hm_size(map, &size) == HASHMAP_OK && size == 50)
    visited |= V(1, 11);
  else {
    hm_destroy(map);
    return visited | V(4, 11);
  }

  {
    int *g0 = hm_get(map, 0);
    int *g25 = hm_get(map, 25);
    int *g49 = hm_get(map, 49);
    if (g0 != NULL && *g0 == 1000 && g25 != NULL && *g25 == 1025 &&
        g49 != NULL && *g49 == 1049)
      visited |= V(1, 12);
    else {
      hm_destroy(map);
      return visited | V(4, 12);
    }
  }

  if (hm_remove(map, 3) == HASHMAP_OK && hm_remove(map, 44) == HASHMAP_OK &&
      hm_size(map, &size) == HASHMAP_OK && size == 48)
    visited |= V(1, 13);
  else {
    hm_destroy(map);
    return visited | V(4, 13);
  }

  errno = 0;
  if (hm_remove(map, 3) == HASHMAP_ERR && errno == ENOENT)
    visited |= V(1, 14);
  else {
    hm_destroy(map);
    return visited | V(4, 14);
  }

  if (hm_destroy(map) == HASHMAP_OK)
    visited |= V(1, 15);
  else
    visited |= V(4, 15);

  return visited;
}

static int memory(void) {
  memory_tests_check();
  return memory_test(scenario);
}

static const test_list_t test_list[] = {
  TEST(api), TEST(edge), TEST(ownership),
  TEST(collisions), TEST(growth), TEST(shrink), TEST(memory),
};

int main(int argc, char *argv[]) {
  return run_test_main(argc, argv, test_list, SIZE(test_list));
}
