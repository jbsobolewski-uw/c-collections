//
// Tests for the core bst module (src/bst/bst.{h,c}).
//
// Exercises the tree through the public core API only; the traversal add-on
// (bst_traversals) is covered by a separate suite.
//

#include "collections_tests.h"
#include "../src/bst/bst.h"
#include <limits.h>
#include <stdlib.h>

/* Checks that a call returns BST_ERR with the given errno. */
#define EXPECT_ERRNO(call, e)                     \
  do {                                            \
    errno = 0;                                    \
    if ((call) != BST_ERR || errno != (e))        \
      return FAIL;                                \
  } while (0)

/** HELPERS **/

typedef struct {
  long key;
  int tag;
} item_t;

static int item_cmp(void *a, void *b) {
  const item_t *x = (const item_t *) a;
  const item_t *y = (const item_t *) b;
  if (x->key < y->key)
    return BST_LE;
  if (x->key > y->key)
    return BST_GR;
  return BST_EQ;
}

/* Fills elems[i] = keys[i] and inserts each into a fresh default tree. */
static bst_t *make_tree(long *elems, long const *keys, size_t n) {
  bst_t *t = bst_create(NULL, NULL);
  if (!t)
    return NULL;
  for (size_t i = 0; i < n; ++i) {
    elems[i] = keys[i];
    if (bst_insert(t, &elems[i]) != BST_OK) {
      bst_destroy(t);
      return NULL;
    }
  }
  return t;
}

/* True iff the tree has exactly size n and every key in keys is findable. */
static int tree_matches(bst_t *t, long const *keys, size_t n) {
  size_t sz = 0;
  if (bst_size(t, &sz) != BST_OK || sz != n)
    return 0;
  for (size_t i = 0; i < n; ++i) {
    long k = keys[i];
    void *out = NULL;
    if (bst_search(t, &k, &out) != BST_OK || out == NULL ||
        *(long *) out != keys[i])
      return 0;
  }
  return 1;
}

/* Builds a tree from ins[0..n), removes victim, and checks that exactly
 * rest[0..nrest) remains and that victim is gone. */
static int remove_shape(long const *ins, size_t n, long victim,
                        long const *rest, size_t nrest) {
  long elems[16];
  if (n > SIZE(elems))
    return 0;
  bst_t *t = make_tree(elems, ins, n);
  if (!t)
    return 0;

  long k = victim;
  int ok = bst_remove(t, &k) == BST_OK && tree_matches(t, rest, nrest);
  if (ok) {
    void *out = NULL;
    errno = 0;
    ok = bst_search(t, &k, &out) == BST_ERR && errno == ENOENT;
  }
  bst_destroy(t);
  return ok;
}

/** TEST CASES **/

// Tests basic API behaviour: comparators, insert/search/size/is_empty,
// all removal shapes, and duplicate keys.
static int api(void) {
  /* Ready-made comparators, including sign-boundary behaviour. */
  {
    long a = -5, b = 3, c = -5;
    ASSERT(bst_cmp_signed_int(&a, &b) < 0);
    ASSERT(bst_cmp_signed_int(&b, &a) > 0);
    ASSERT(bst_cmp_signed_int(&a, &c) == 0);

    long mn = LONG_MIN, mx = LONG_MAX, neg = -1, pos = 1;
    ASSERT(bst_cmp_signed_int(&mn, &mx) < 0);
    ASSERT(bst_cmp_signed_int(&mx, &mn) > 0);
    ASSERT(bst_cmp_signed_int(&neg, &pos) < 0);

    unsigned long uz = 0, umax = ULONG_MAX, uone = 1;
    ASSERT(bst_cmp_unsigned_int(&uz, &umax) < 0);
    ASSERT(bst_cmp_unsigned_int(&umax, &uz) > 0);
    ASSERT(bst_cmp_unsigned_int(&umax, &umax) == 0);
    /* Same bit pattern as (long) -1, but unsigned: ULONG_MAX > 1. */
    ASSERT(bst_cmp_unsigned_int(&umax, &uone) > 0);
    ASSERT(bst_cmp_unsigned_int(&uone, &umax) < 0);
  }

  /* Default comparator; search yields the stored pointer and does not
   * remove the element. */
  {
    bst_t *t = bst_create(NULL, NULL);
    ASSERT(t != NULL);

    int empty = 0;
    ASSERT(bst_is_empty(t, &empty) == BST_OK && empty == 1);

    long elems[3] = {2, 1, 3};
    for (size_t i = 0; i < SIZE(elems); ++i)
      ASSERT(bst_insert(t, &elems[i]) == BST_OK);
    ASSERT(bst_is_empty(t, &empty) == BST_OK && empty == 0);

    long probe = 1;
    void *out = NULL;
    ASSERT(bst_search(t, &probe, &out) == BST_OK);
    ASSERT(out == &elems[1]);

    /* Still in the tree afterwards. */
    size_t sz = 0;
    ASSERT(bst_size(t, &sz) == BST_OK && sz == 3);
    out = NULL;
    ASSERT(bst_search(t, &probe, &out) == BST_OK && out == &elems[1]);

    ASSERT(bst_destroy(t) == BST_OK);
  }

  /* Custom comparator over a custom struct. */
  {
    bst_t *t = bst_create(item_cmp, NULL);
    ASSERT(t != NULL);

    item_t items[4] = {{50, 0}, {20, 1}, {80, 2}, {60, 3}};
    for (size_t i = 0; i < SIZE(items); ++i)
      ASSERT(bst_insert(t, &items[i]) == BST_OK);

    item_t probe = {60, -1};
    void *out = NULL;
    ASSERT(bst_search(t, &probe, &out) == BST_OK);
    ASSERT(out == &items[3] && ((item_t *) out)->tag == 3);

    size_t sz = 0;
    int empty = -1;
    ASSERT(bst_size(t, &sz) == BST_OK && sz == 4);
    ASSERT(bst_is_empty(t, &empty) == BST_OK && empty == 0);

    probe.key = 20;
    ASSERT(bst_remove(t, &probe) == BST_OK);
    ASSERT(bst_size(t, &sz) == BST_OK && sz == 3);
    errno = 0;
    ASSERT(bst_search(t, &probe, &out) == BST_ERR && errno == ENOENT);

    ASSERT(bst_destroy(t) == BST_OK);
  }

  /* Removal of all three node shapes, non-root and root. */
  {
    static long const i1[] = {10, 5, 15}, r1[] = {10, 15};
    ASSERT(remove_shape(i1, SIZE(i1), 5, r1, SIZE(r1)));   /* leaf */

    static long const i2[] = {10, 5, 3}, r2[] = {10, 3};
    ASSERT(remove_shape(i2, SIZE(i2), 5, r2, SIZE(r2)));   /* left child only */

    static long const i3[] = {10, 15, 20}, r3[] = {10, 20};
    ASSERT(remove_shape(i3, SIZE(i3), 15, r3, SIZE(r3)));  /* right child only */

    static long const i4[] = {10, 5, 15, 12, 20}, r4[] = {10, 5, 12, 20};
    ASSERT(remove_shape(i4, SIZE(i4), 15, r4, SIZE(r4)));  /* two children */

    static long const i5[] = {10};
    ASSERT(remove_shape(i5, SIZE(i5), 10, NULL, 0));       /* root leaf */

    static long const i6[] = {10, 5}, r6[] = {5};
    ASSERT(remove_shape(i6, SIZE(i6), 10, r6, SIZE(r6)));  /* root, left only */

    static long const i7[] = {10, 15}, r7[] = {15};
    ASSERT(remove_shape(i7, SIZE(i7), 10, r7, SIZE(r7)));  /* root, right only */

    static long const i8[] = {10, 5, 15}, r8[] = {5, 15};
    ASSERT(remove_shape(i8, SIZE(i8), 10, r8, SIZE(r8)));  /* root, two children */

    /* Root with two children where the successor itself has a right child. */
    static long const i9[] = {10, 5, 20, 15, 25, 17},
                      r9[] = {5, 15, 17, 20, 25};
    ASSERT(remove_shape(i9, SIZE(i9), 10, r9, SIZE(r9)));
  }

  /* Duplicates: comparator result <= BST_EQ goes left, so equal keys are
   * allowed; remove them one by one. */
  {
    bst_t *t = bst_create(NULL, NULL);
    ASSERT(t != NULL);

    long lo = 3, hi = 9, dups[5];
    ASSERT(bst_insert(t, &lo) == BST_OK);
    ASSERT(bst_insert(t, &hi) == BST_OK);
    for (size_t i = 0; i < SIZE(dups); ++i) {
      dups[i] = 7;
      ASSERT(bst_insert(t, &dups[i]) == BST_OK);
    }

    size_t sz = 0;
    ASSERT(bst_size(t, &sz) == BST_OK && sz == 2 + SIZE(dups));

    long k7 = 7;
    for (size_t i = 0; i < SIZE(dups); ++i) {
      void *out = NULL;
      ASSERT(bst_search(t, &k7, &out) == BST_OK && *(long *) out == 7);
      ASSERT(bst_remove(t, &k7) == BST_OK);
      ASSERT(bst_size(t, &sz) == BST_OK && sz == 1 + SIZE(dups) - i);
    }

    void *out = NULL;
    errno = 0;
    ASSERT(bst_search(t, &k7, &out) == BST_ERR && errno == ENOENT);
    errno = 0;
    ASSERT(bst_remove(t, &k7) == BST_ERR && errno == ENOENT);

    long k = 3;
    ASSERT(bst_search(t, &k, &out) == BST_OK && out == &lo);
    k = 9;
    ASSERT(bst_search(t, &k, &out) == BST_OK && out == &hi);

    ASSERT(bst_destroy(t) == BST_OK);
  }

  return PASS;
}

// Tests argument validation and boundary conditions.
static int edge(void) {
  long x = 1;
  void *out = NULL;
  size_t sz = 0;
  int empty = 0;

  /* NULL tree for every function. */
  EXPECT_ERRNO(bst_destroy(NULL), EINVAL);
  EXPECT_ERRNO(bst_insert(NULL, &x), EINVAL);
  EXPECT_ERRNO(bst_remove(NULL, &x), EINVAL);
  EXPECT_ERRNO(bst_search(NULL, &x, &out), EINVAL);
  EXPECT_ERRNO(bst_size(NULL, &sz), EINVAL);
  EXPECT_ERRNO(bst_is_empty(NULL, &empty), EINVAL);

  /* Internal accessors are NULL-safe. */
  ASSERT(bst_get_root(NULL) == NULL);
  ASSERT(bst_node_get_left(NULL) == NULL);
  ASSERT(bst_node_get_right(NULL) == NULL);
  ASSERT(bst_node_get_data(NULL) == NULL);

  bst_t *t = bst_create(NULL, NULL);
  ASSERT(t != NULL);

  /* NULL out-pointers. */
  EXPECT_ERRNO(bst_search(t, &x, NULL), EINVAL);
  EXPECT_ERRNO(bst_size(t, NULL), EINVAL);
  EXPECT_ERRNO(bst_is_empty(t, NULL), EINVAL);

  /* Empty tree. */
  ASSERT(bst_get_root(t) == NULL);
  EXPECT_ERRNO(bst_search(t, &x, &out), ENOENT);
  EXPECT_ERRNO(bst_remove(t, &x), ENOENT);
  ASSERT(bst_size(t, &sz) == BST_OK && sz == 0);
  ASSERT(bst_is_empty(t, &empty) == BST_OK && empty == 1);

  /* Single-element lifecycle. */
  ASSERT(bst_insert(t, &x) == BST_OK);
  ASSERT(bst_size(t, &sz) == BST_OK && sz == 1);
  ASSERT(bst_is_empty(t, &empty) == BST_OK && empty == 0);
  ASSERT(bst_get_root(t) != NULL);
  ASSERT(bst_node_get_data(bst_get_root(t)) == &x);
  ASSERT(bst_node_get_left(bst_get_root(t)) == NULL);
  ASSERT(bst_node_get_right(bst_get_root(t)) == NULL);

  long probe = 1;
  ASSERT(bst_search(t, &probe, &out) == BST_OK && out == &x);

  /* Absent keys on a non-empty tree. */
  long absent = 2;
  EXPECT_ERRNO(bst_search(t, &absent, &out), ENOENT);
  EXPECT_ERRNO(bst_remove(t, &absent), ENOENT);
  absent = 0;
  EXPECT_ERRNO(bst_search(t, &absent, &out), ENOENT);
  EXPECT_ERRNO(bst_remove(t, &absent), ENOENT);

  /* Back to empty, then reusable. */
  ASSERT(bst_remove(t, &probe) == BST_OK);
  ASSERT(bst_size(t, &sz) == BST_OK && sz == 0);
  ASSERT(bst_is_empty(t, &empty) == BST_OK && empty == 1);
  ASSERT(bst_get_root(t) == NULL);
  EXPECT_ERRNO(bst_search(t, &probe, &out), ENOENT);
  ASSERT(bst_insert(t, &x) == BST_OK);
  ASSERT(bst_size(t, &sz) == BST_OK && sz == 1);
  ASSERT(bst_destroy(t) == BST_OK);

  return PASS;
}

static size_t dtor_count;
static long dtor_sum;

static void counting_dtor(void *p) {
  ++dtor_count;
  dtor_sum += *(long *) p;
}

/* Per-pointer destructor accounting for the duplicate-successor regression:
 * detects both a double destroy and a dropped element. */
static long dup_elems[5];
static unsigned dup_counts[5];
static unsigned dup_unknown;

static void dup_dtor(void *p) {
  for (size_t i = 0; i < SIZE(dup_elems); ++i) {
    if (p == &dup_elems[i]) {
      ++dup_counts[i];
      return;
    }
  }
  ++dup_unknown;
}

// Tests destructor ownership: exactly one destructor call per element,
// on bst_remove (all shapes) and on bst_destroy.
static int ownership(void) {
  /* Counting destructor over remove shapes and destroy. */
  {
    bst_t *t = bst_create(NULL, counting_dtor);
    ASSERT(t != NULL);

    static long const keys[] = {10, 5, 15, 12, 20, 3};
    long elems[SIZE(keys)];
    for (size_t i = 0; i < SIZE(keys); ++i) {
      elems[i] = keys[i];
      ASSERT(bst_insert(t, &elems[i]) == BST_OK);
    }

    dtor_count = 0;
    dtor_sum = 0;

    long k = 20; /* leaf */
    ASSERT(bst_remove(t, &k) == BST_OK);
    ASSERT(dtor_count == 1 && dtor_sum == 20);

    k = 15; /* only a left child (12) now that 20 is gone */
    ASSERT(bst_remove(t, &k) == BST_OK);
    ASSERT(dtor_count == 2 && dtor_sum == 35);

    k = 10; /* root with two children (5 and 12) */
    ASSERT(bst_remove(t, &k) == BST_OK);
    ASSERT(dtor_count == 3 && dtor_sum == 45);

    static long const rest[] = {5, 3, 12};
    ASSERT(tree_matches(t, rest, SIZE(rest)));

    /* Destroy invokes the destructor once per remaining element. */
    ASSERT(bst_destroy(t) == BST_OK);
    ASSERT(dtor_count == 6 && dtor_sum == 45 + 5 + 3 + 12);
  }

  /* Non-root two-children removal: still exactly one destructor call,
   * on the removed element's data. */
  {
    bst_t *t = bst_create(NULL, counting_dtor);
    ASSERT(t != NULL);

    static long const keys[] = {50, 30, 70, 60, 80, 65};
    long elems[SIZE(keys)];
    for (size_t i = 0; i < SIZE(keys); ++i) {
      elems[i] = keys[i];
      ASSERT(bst_insert(t, &elems[i]) == BST_OK);
    }

    dtor_count = 0;
    dtor_sum = 0;

    long k = 70; /* two children: 60 and 80 */
    ASSERT(bst_remove(t, &k) == BST_OK);
    ASSERT(dtor_count == 1 && dtor_sum == 70);

    static long const rest[] = {50, 30, 60, 65, 80};
    ASSERT(tree_matches(t, rest, SIZE(rest)));

    ASSERT(bst_destroy(t) == BST_OK);
    ASSERT(dtor_count == 6 && dtor_sum == 70 + 50 + 30 + 60 + 65 + 80);
  }

  /* NULL destructor: the tree never touches the elements. */
  {
    bst_t *t = bst_create(NULL, NULL);
    ASSERT(t != NULL);

    long *p[3];
    for (size_t i = 0; i < SIZE(p); ++i) {
      p[i] = (long *) malloc(sizeof *p[i]);
      ASSERT(p[i] != NULL);
      *p[i] = (long) i + 100;
      ASSERT(bst_insert(t, p[i]) == BST_OK);
    }

    long k = 101;
    ASSERT(bst_remove(t, &k) == BST_OK);
    ASSERT(*p[1] == 101); /* removed, but not destroyed */

    ASSERT(bst_destroy(t) == BST_OK);
    ASSERT(*p[0] == 100 && *p[1] == 101 && *p[2] == 102);

    for (size_t i = 0; i < SIZE(p); ++i)
      free(p[i]);
  }

  /* Regression: two-children removal where the successor's key also exists
   * as a duplicate on the path to it (insert 10,5,15,12,12 makes the two
   * 12s a chain in the right subtree of 10; the successor is the deeper
   * one). The successor must be unlinked by node, not by key - every
   * element's destructor must run exactly once, with no data pointer
   * duplicated or dropped. */
  {
    static long const keys[] = {10, 5, 15, 12, 12};
    for (size_t i = 0; i < SIZE(keys); ++i) {
      dup_elems[i] = keys[i];
      dup_counts[i] = 0;
    }
    dup_unknown = 0;

    bst_t *t = bst_create(NULL, dup_dtor);
    ASSERT(t != NULL);
    for (size_t i = 0; i < SIZE(keys); ++i)
      ASSERT(bst_insert(t, &dup_elems[i]) == BST_OK);

    long k = 10;
    ASSERT(bst_remove(t, &k) == BST_OK);
    ASSERT(dup_counts[0] == 1 && dup_unknown == 0);

    size_t n = 0;
    ASSERT(bst_size(t, &n) == BST_OK && n == 4);

    ASSERT(bst_destroy(t) == BST_OK);
    for (size_t i = 0; i < SIZE(dup_elems); ++i)
      ASSERT(dup_counts[i] == 1);
    ASSERT(dup_unknown == 0);
  }

  return PASS;
}

/** STRESS TEST **/

#define STRESS_N 4000
#define SORTED_N 2000

static long stress_vals[STRESS_N];
static size_t stress_perm[STRESS_N];
static long sorted_vals[SORTED_N];

/* Deterministic PRNG (64-bit LCG, high bits). */
static unsigned long lcg_next(unsigned long *state) {
  *state = *state * 6364136223846793005UL + 1442695040888963407UL;
  return *state >> 33;
}

static void shuffle(size_t *perm, size_t n, unsigned long seed) {
  for (size_t i = n; i > 1; --i) {
    size_t j = (size_t) (lcg_next(&seed) % i);
    size_t tmp = perm[i - 1];
    perm[i - 1] = perm[j];
    perm[j] = tmp;
  }
}

// Tests many shuffled insertions/removals and a degenerate sorted tree.
static int stress(void) {
  bst_t *t = bst_create(NULL, NULL);
  ASSERT(t != NULL);

  /* Insert even keys 0, 2, ..., 2 * (STRESS_N - 1) in shuffled order. */
  for (size_t i = 0; i < STRESS_N; ++i) {
    stress_vals[i] = (long) i * 2;
    stress_perm[i] = i;
  }
  shuffle(stress_perm, STRESS_N, 0x9e3779b97f4a7c15UL);
  for (size_t i = 0; i < STRESS_N; ++i)
    ASSERT(bst_insert(t, &stress_vals[stress_perm[i]]) == BST_OK);

  size_t sz = 0;
  ASSERT(bst_size(t, &sz) == BST_OK && sz == STRESS_N);

  /* Every inserted key is findable, yielding the stored pointer. */
  for (size_t i = 0; i < STRESS_N; ++i) {
    long k = stress_vals[i];
    void *out = NULL;
    ASSERT(bst_search(t, &k, &out) == BST_OK && out == &stress_vals[i]);
  }

  /* Absent (odd and out-of-range) keys miss. */
  for (size_t i = 0; i < STRESS_N; ++i) {
    long k = (long) i * 2 + 1;
    void *out = NULL;
    errno = 0;
    ASSERT(bst_search(t, &k, &out) == BST_ERR && errno == ENOENT);
  }
  {
    long k = -1;
    void *out = NULL;
    errno = 0;
    ASSERT(bst_search(t, &k, &out) == BST_ERR && errno == ENOENT);
    k = 2 * STRESS_N;
    errno = 0;
    ASSERT(bst_search(t, &k, &out) == BST_ERR && errno == ENOENT);
  }

  /* Remove half of the keys in a different shuffled order. */
  shuffle(stress_perm, STRESS_N, 0x0123456789abcdefUL);
  for (size_t i = 0; i < STRESS_N / 2; ++i) {
    long k = stress_vals[stress_perm[i]];
    ASSERT(bst_remove(t, &k) == BST_OK);
  }
  ASSERT(bst_size(t, &sz) == BST_OK && sz == STRESS_N / 2);

  for (size_t i = 0; i < STRESS_N / 2; ++i) {
    long k = stress_vals[stress_perm[i]];
    void *out = NULL;
    errno = 0;
    ASSERT(bst_search(t, &k, &out) == BST_ERR && errno == ENOENT);
  }
  for (size_t i = STRESS_N / 2; i < STRESS_N; ++i) {
    size_t idx = stress_perm[i];
    long k = stress_vals[idx];
    void *out = NULL;
    ASSERT(bst_search(t, &k, &out) == BST_OK && out == &stress_vals[idx]);
  }
  ASSERT(bst_destroy(t) == BST_OK);

  /* Adversarial sorted-order insert: the tree degenerates to a list. */
  t = bst_create(NULL, NULL);
  ASSERT(t != NULL);
  for (size_t i = 0; i < SORTED_N; ++i) {
    sorted_vals[i] = (long) i;
    ASSERT(bst_insert(t, &sorted_vals[i]) == BST_OK);
  }
  ASSERT(bst_size(t, &sz) == BST_OK && sz == SORTED_N);

  for (size_t i = 0; i < SORTED_N; i += 37) {
    long k = (long) i;
    void *out = NULL;
    ASSERT(bst_search(t, &k, &out) == BST_OK && out == &sorted_vals[i]);
  }
  {
    long k = SORTED_N;
    void *out = NULL;
    errno = 0;
    ASSERT(bst_search(t, &k, &out) == BST_ERR && errno == ENOENT);
    k = -1;
    errno = 0;
    ASSERT(bst_search(t, &k, &out) == BST_ERR && errno == ENOENT);
  }

  /* Remove the head (root, right child only), the deepest leaf, and a
   * middle node of the chain. */
  {
    long k = 0;
    ASSERT(bst_remove(t, &k) == BST_OK);
    k = SORTED_N - 1;
    ASSERT(bst_remove(t, &k) == BST_OK);
    k = SORTED_N / 2;
    ASSERT(bst_remove(t, &k) == BST_OK);
    ASSERT(bst_size(t, &sz) == BST_OK && sz == SORTED_N - 3);

    void *out = NULL;
    k = 1;
    ASSERT(bst_search(t, &k, &out) == BST_OK && out == &sorted_vals[1]);
    k = SORTED_N - 2;
    ASSERT(bst_search(t, &k, &out) == BST_OK &&
           out == &sorted_vals[SORTED_N - 2]);
    k = SORTED_N / 2 + 1;
    ASSERT(bst_search(t, &k, &out) == BST_OK &&
           out == &sorted_vals[SORTED_N / 2 + 1]);
    k = SORTED_N / 2;
    errno = 0;
    ASSERT(bst_search(t, &k, &out) == BST_ERR && errno == ENOENT);
  }
  ASSERT(bst_destroy(t) == BST_OK);

  return PASS;
}

/** MEMORY TEST **/

// Allocation-failure scenario: every allocating call follows the
// fail-once/retry pattern; non-allocating calls are checked plainly.
// Every path frees everything (heap longs are owned by the tree via free).
static unsigned long scenario(void) {
  static long const keys[] = {20, 10, 30, 25};
  unsigned long visited = 0;
  bst_t *t;
  long *p;

  errno = 0;
  if ((t = bst_create(NULL, free)) != NULL)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (t = bst_create(NULL, free)) != NULL)
    visited |= V(2, 0);
  else
    return visited | V(4, 0); // This should not execute.

  for (unsigned i = 0; i < 4; ++i) {
    unsigned w = 1 + 2 * i;

    errno = 0;
    if ((p = (long *) malloc(sizeof *p)) != NULL)
      visited |= V(1, w);
    else if (errno == ENOMEM && (p = (long *) malloc(sizeof *p)) != NULL)
      visited |= V(2, w);
    else {
      bst_destroy(t);
      return visited | V(4, w); // This should not execute.
    }
    *p = keys[i];

    errno = 0;
    if (bst_insert(t, p) == BST_OK)
      visited |= V(1, w + 1);
    else if (errno == ENOMEM && bst_insert(t, p) == BST_OK)
      visited |= V(2, w + 1);
    else {
      free(p);
      bst_destroy(t);
      return visited | V(4, w + 1); // This should not execute.
    }
  }

  /* Non-allocating calls; no injected failure can hit these. */
  long k = 25;
  void *out = NULL;
  errno = 0;
  if (bst_search(t, &k, &out) == BST_OK && out != NULL && *(long *) out == 25)
    visited |= V(1, 9);
  else {
    bst_destroy(t);
    return visited | V(4, 9); // This should not execute.
  }

  size_t sz = 0;
  errno = 0;
  if (bst_size(t, &sz) == BST_OK && sz == 4)
    visited |= V(1, 10);
  else {
    bst_destroy(t);
    return visited | V(4, 10); // This should not execute.
  }

  k = 30; /* one child; frees the payload, recycles the node */
  errno = 0;
  if (bst_remove(t, &k) == BST_OK)
    visited |= V(1, 11);
  else {
    bst_destroy(t);
    return visited | V(4, 11); // This should not execute.
  }

  /* After the remove the next insert reuses the recycled node, so only the
   * payload allocation below can see an injected failure. */
  errno = 0;
  if ((p = (long *) malloc(sizeof *p)) != NULL)
    visited |= V(1, 12);
  else if (errno == ENOMEM && (p = (long *) malloc(sizeof *p)) != NULL)
    visited |= V(2, 12);
  else {
    bst_destroy(t);
    return visited | V(4, 12); // This should not execute.
  }
  *p = 15;

  errno = 0;
  if (bst_insert(t, p) == BST_OK)
    visited |= V(1, 13);
  else if (errno == ENOMEM && bst_insert(t, p) == BST_OK)
    visited |= V(2, 13);
  else {
    free(p);
    bst_destroy(t);
    return visited | V(4, 13); // This should not execute.
  }

  int empty = -1;
  errno = 0;
  if (bst_is_empty(t, &empty) == BST_OK && empty == 0)
    visited |= V(1, 14);
  else {
    bst_destroy(t);
    return visited | V(4, 14); // This should not execute.
  }

  k = 20; /* root with two children */
  errno = 0;
  if (bst_remove(t, &k) == BST_OK)
    visited |= V(1, 15);
  else {
    bst_destroy(t);
    return visited | V(4, 15); // This should not execute.
  }

  errno = 0;
  if (bst_size(t, &sz) == BST_OK && sz == 3)
    visited |= V(1, 16);
  else {
    bst_destroy(t);
    return visited | V(4, 16); // This should not execute.
  }

  bst_destroy(t);
  return visited;
}

// Tests the implementation's reaction to allocation failures.
static int memory(void) {
  memory_tests_check();
  return memory_test(scenario);
}

/** TEST REGISTRY **/

static const test_list_t test_list[] = {
  TEST(api),
  TEST(edge),
  TEST(ownership),
  TEST(stress),
  TEST(memory)
};

int main(int argc, char *argv[]) {
  return run_test_main(argc, argv, test_list, SIZE(test_list));
}
