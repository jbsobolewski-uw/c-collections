//
// Tests for the bst_traversals module - the traversal add-on to the core BST.
// The core BST module is covered by a separate suite; here we only exercise
// bst_apply and the seven traversal orders.
//

#include "collections_tests.h"
#include "../src/bst/bst_traversals.h"

#include <stdlib.h>

/* --- Helpers ------------------------------------------------------------ */

#define COLLECT_MAX 128

typedef struct {
  long values[COLLECT_MAX];
  size_t count;
} collector_t;

/* Appends the visited long to the collector; bounds-checked. */
static int collect_job(void *obj, void *argstruct) {
  collector_t *c = argstruct;
  if (c->count >= COLLECT_MAX)
    return -100; /* overflow guard - fails the sequence comparison */
  c->values[c->count++] = *(long *) obj;
  return 0;
}

typedef struct {
  size_t visited;
  size_t stop_after;
} stopper_t;

/* Counts visits and returns the distinctive value 42 to stop the walk
 * after stop_after elements. */
static int stop_job(void *obj, void *argstruct) {
  (void) obj;
  stopper_t *s = argstruct;
  ++s->visited;
  return s->visited >= s->stop_after ? 42 : 0;
}

/* Counts visits, never stops. */
static int count_job(void *obj, void *argstruct) {
  (void) obj;
  ++*(size_t *) argstruct;
  return 0;
}

/* Builds a tree of heap longs (default signed-long comparator, free as the
 * destructor), inserting vals[0..n-1] in the given order.
 * Returns NULL on any failure. */
static bst_t *make_tree(long const *vals, size_t n) {
  bst_t *tree = bst_create(NULL, free);
  if (!tree)
    return NULL;
  for (size_t i = 0; i < n; ++i) {
    long *p = malloc(sizeof *p);
    if (!p) {
      bst_destroy(tree);
      return NULL;
    }
    *p = vals[i];
    if (bst_insert(tree, p) != BST_OK) {
      free(p);
      bst_destroy(tree);
      return NULL;
    }
  }
  return tree;
}

/* Runs one traversal and compares the exact visit sequence with expected. */
static int check_order(bst_t *tree, bst_traversal_t order,
                       long const *expected, size_t n) {
  collector_t c = {.count = 0};
  if (bst_apply(tree, order, collect_job, &c) != BST_OK)
    return FAIL;
  if (c.count != n)
    return FAIL;
  for (size_t i = 0; i < n; ++i)
    if (c.values[i] != expected[i])
      return FAIL;
  return PASS;
}

/* All seven traversal orders. */
static bst_traversal_t const all_orders[] = {
    BST_TRAVERSAL_NLR, BST_TRAVERSAL_LNR, BST_TRAVERSAL_LRN,
    BST_TRAVERSAL_NRL, BST_TRAVERSAL_RNL, BST_TRAVERSAL_RLN,
    BST_TRAVERSAL_BFS};

/* --- Test cases --------------------------------------------------------- */

// Verifies the exact visit sequence of all six DFS orders and BFS on a known
// complete tree, and that in-order is ascending on a larger shuffled tree.
static int orders(void) {
  /* Inserting 4,2,6,1,3,5,7 yields root 4, children 2 and 6,
   * leaves 1, 3, 5, 7 - a complete tree of height 3. */
  static long const ins[] = {4, 2, 6, 1, 3, 5, 7};
  static long const nlr[] = {4, 2, 1, 3, 6, 5, 7};
  static long const lnr[] = {1, 2, 3, 4, 5, 6, 7};
  static long const lrn[] = {1, 3, 2, 5, 7, 6, 4};
  static long const nrl[] = {4, 6, 7, 5, 2, 3, 1};
  static long const rnl[] = {7, 6, 5, 4, 3, 2, 1};
  static long const rln[] = {7, 5, 6, 3, 1, 2, 4};
  static long const bfs_seq[] = {4, 2, 6, 1, 3, 5, 7};

  bst_t *tree = make_tree(ins, SIZE(ins));
  ASSERT(tree != NULL);
  ASSERT(check_order(tree, BST_TRAVERSAL_NLR, nlr, SIZE(nlr)) == PASS);
  ASSERT(check_order(tree, BST_TRAVERSAL_LNR, lnr, SIZE(lnr)) == PASS);
  ASSERT(check_order(tree, BST_TRAVERSAL_LRN, lrn, SIZE(lrn)) == PASS);
  ASSERT(check_order(tree, BST_TRAVERSAL_NRL, nrl, SIZE(nrl)) == PASS);
  ASSERT(check_order(tree, BST_TRAVERSAL_RNL, rnl, SIZE(rnl)) == PASS);
  ASSERT(check_order(tree, BST_TRAVERSAL_RLN, rln, SIZE(rln)) == PASS);
  ASSERT(check_order(tree, BST_TRAVERSAL_BFS, bfs_seq, SIZE(bfs_seq)) == PASS);
  ASSERT(bst_destroy(tree) == BST_OK);

  /* Larger tree: the LCG x -> (21 x + 7) mod 100 has full period
   * (Hull-Dobell), so 100 steps produce a deterministic permutation of
   * 0..99; in-order traversal must yield them ascending. */
  long shuffled[100];
  unsigned long x = 0;
  for (size_t i = 0; i < SIZE(shuffled); ++i) {
    shuffled[i] = (long) x;
    x = (x * 21 + 7) % 100;
  }
  long ascending[100];
  for (size_t i = 0; i < SIZE(ascending); ++i)
    ascending[i] = (long) i;

  tree = make_tree(shuffled, SIZE(shuffled));
  ASSERT(tree != NULL);
  ASSERT(check_order(tree, BST_TRAVERSAL_LNR,
                     ascending, SIZE(ascending)) == PASS);
  ASSERT(bst_destroy(tree) == BST_OK);

  return PASS;
}

// Verifies that a non-zero job return value stops the traversal after exactly
// k elements and is propagated as bst_apply's return value, for every order.
static int early_stop(void) {
  static long const ins[] = {4, 2, 6, 1, 3, 5, 7};

  bst_t *tree = make_tree(ins, SIZE(ins));
  ASSERT(tree != NULL);

  for (size_t i = 0; i < SIZE(all_orders); ++i) {
    /* Stop after the third element: exactly 3 visits, 42 propagated. */
    stopper_t s = {.visited = 0, .stop_after = 3};
    ASSERT(bst_apply(tree, all_orders[i], stop_job, &s) == 42);
    ASSERT(s.visited == 3);

    /* Stop at the very first element. */
    s.visited = 0;
    s.stop_after = 1;
    ASSERT(bst_apply(tree, all_orders[i], stop_job, &s) == 42);
    ASSERT(s.visited == 1);
  }

  ASSERT(bst_destroy(tree) == BST_OK);
  return PASS;
}

// Verifies parameter validation, the empty tree and the single-node tree.
static int edge(void) {
  size_t cnt = 0;

  /* NULL tree. */
  errno = 0;
  ASSERT(bst_apply(NULL, BST_TRAVERSAL_LNR, count_job, &cnt) == BST_ERR);
  ASSERT(errno == EINVAL);

  bst_t *tree = bst_create(NULL, free);
  ASSERT(tree != NULL);

  /* NULL job. */
  errno = 0;
  ASSERT(bst_apply(tree, BST_TRAVERSAL_LNR, NULL, NULL) == BST_ERR);
  ASSERT(errno == EINVAL);

  /* Invalid traversal order. */
  errno = 0;
  ASSERT(bst_apply(tree, (bst_traversal_t) 99, count_job, &cnt) == BST_ERR);
  ASSERT(errno == EINVAL);
  ASSERT(cnt == 0);

  /* Empty tree: no-op success, the job is never called. */
  for (size_t i = 0; i < SIZE(all_orders); ++i)
    ASSERT(bst_apply(tree, all_orders[i], count_job, &cnt) == BST_OK);
  ASSERT(cnt == 0);

  /* Single-node tree: every order visits exactly that one element. */
  long *p = malloc(sizeof *p);
  ASSERT(p != NULL);
  *p = 99;
  ASSERT(bst_insert(tree, p) == BST_OK);
  static long const only[] = {99};
  for (size_t i = 0; i < SIZE(all_orders); ++i)
    ASSERT(check_order(tree, all_orders[i], only, SIZE(only)) == PASS);

  ASSERT(bst_destroy(tree) == BST_OK);
  return PASS;
}

// Verifies level-order visits on specific tree shapes.
static int bfs(void) {
  /* Complete 7-node tree: levels 4 | 2 6 | 1 3 5 7. */
  static long const complete_ins[] = {4, 2, 6, 1, 3, 5, 7};
  static long const complete_bfs[] = {4, 2, 6, 1, 3, 5, 7};
  bst_t *tree = make_tree(complete_ins, SIZE(complete_ins));
  ASSERT(tree != NULL);
  ASSERT(check_order(tree, BST_TRAVERSAL_BFS,
                     complete_bfs, SIZE(complete_bfs)) == PASS);
  ASSERT(bst_destroy(tree) == BST_OK);

  /* Left-degenerate chain (descending inserts): one node per level,
   * BFS visits the chain top-down. */
  static long const desc[] = {5, 4, 3, 2, 1};
  tree = make_tree(desc, SIZE(desc));
  ASSERT(tree != NULL);
  ASSERT(check_order(tree, BST_TRAVERSAL_BFS, desc, SIZE(desc)) == PASS);
  ASSERT(bst_destroy(tree) == BST_OK);

  /* Right-degenerate chain (ascending inserts). */
  static long const asc[] = {1, 2, 3, 4, 5};
  tree = make_tree(asc, SIZE(asc));
  ASSERT(tree != NULL);
  ASSERT(check_order(tree, BST_TRAVERSAL_BFS, asc, SIZE(asc)) == PASS);
  ASSERT(bst_destroy(tree) == BST_OK);

  /* Perfect 15-node tree inserted in level order: BFS must reproduce the
   * insertion sequence exactly - level by level, left to right within a
   * level, every parent before its children. */
  static long const perfect[] = {8, 4, 12, 2,  6,  10, 14, 1,
                                 3, 5, 7,  9,  11, 13, 15};
  tree = make_tree(perfect, SIZE(perfect));
  ASSERT(tree != NULL);
  ASSERT(check_order(tree, BST_TRAVERSAL_BFS, perfect, SIZE(perfect)) == PASS);
  /* Cross-check the shape: in-order must be 1..15 ascending. */
  static long const one_to_fifteen[] = {1, 2, 3,  4,  5,  6,  7, 8,
                                        9, 10, 11, 12, 13, 14, 15};
  ASSERT(check_order(tree, BST_TRAVERSAL_LNR,
                     one_to_fifteen, SIZE(one_to_fifteen)) == PASS);
  ASSERT(bst_destroy(tree) == BST_OK);

  return PASS;
}

// Allocation-failure scenario driven by memory_test(): builds a 15-node tree
// of heap longs with the create/insert retry pattern, runs a DFS traversal
// (never allocates), then BFS (allocates queue nodes internally, so an
// injected failure yields BST_ERR/ENOMEM and a retry succeeds), then
// destroys the tree. Every path frees the tree; `where` indices 0..18.
static unsigned long scenario(void) {
  static long const vals[] = {8, 4, 12, 2,  6,  10, 14, 1,
                              3, 5, 7,  9,  11, 13, 15};
  unsigned long visited = 0;
  bst_t *tree;

  /* 0: create the tree, retrying once on an injected ENOMEM. */
  errno = 0;
  if ((tree = bst_create(NULL, free)) != NULL)
    visited |= V(1, 0);
  else if (errno == ENOMEM && (tree = bst_create(NULL, free)) != NULL)
    visited |= V(2, 0);
  else
    return visited | V(4, 0); /* This should never execute. */

  /* 1..15: allocate each long and insert it, retrying on ENOMEM. */
  for (size_t i = 0; i < SIZE(vals); ++i) {
    unsigned where = (unsigned) i + 1;
    unsigned long code = 1;

    errno = 0;
    long *p = malloc(sizeof *p);
    if (!p) {
      if (errno == ENOMEM)
        p = malloc(sizeof *p);
      if (!p) {
        bst_destroy(tree);
        return visited | V(4, where); /* This should never execute. */
      }
      code = 2;
    }
    *p = vals[i];

    errno = 0;
    if (bst_insert(tree, p) != BST_OK) {
      if (!(errno == ENOMEM && bst_insert(tree, p) == BST_OK)) {
        free(p);
        bst_destroy(tree);
        return visited | V(4, where); /* This should never execute. */
      }
      code = 2;
    }
    visited |= V(code, where);
  }

  /* 16: a DFS traversal never allocates - it must simply succeed. */
  size_t cnt = 0;
  if (bst_apply(tree, BST_TRAVERSAL_LNR, count_job, &cnt) != BST_OK ||
      cnt != SIZE(vals)) {
    bst_destroy(tree);
    return visited | V(4, 16); /* This should never execute. */
  }
  visited |= V(1, 16);

  /* 17: BFS allocates queue nodes internally - it may fail once with
   * ENOMEM and must succeed when retried. */
  cnt = 0;
  errno = 0;
  if (bst_apply(tree, BST_TRAVERSAL_BFS, count_job, &cnt) == BST_OK) {
    if (cnt != SIZE(vals)) {
      bst_destroy(tree);
      return visited | V(4, 17); /* This should never execute. */
    }
    visited |= V(1, 17);
  } else if (errno == ENOMEM) {
    cnt = 0;
    if (bst_apply(tree, BST_TRAVERSAL_BFS, count_job, &cnt) == BST_OK &&
        cnt == SIZE(vals)) {
      visited |= V(2, 17);
    } else {
      bst_destroy(tree);
      return visited | V(4, 17); /* This should never execute. */
    }
  } else {
    bst_destroy(tree);
    return visited | V(4, 17); /* This should never execute. */
  }

  /* 18: destroy the tree (frees all the longs; never allocates). */
  if (bst_destroy(tree) != BST_OK)
    return visited | V(4, 18); /* This should never execute. */
  visited |= V(1, 18);

  return visited;
}

static int memory(void) {
  memory_tests_check();
  return memory_test(scenario);
}

/* --- Registry ----------------------------------------------------------- */

static const test_list_t test_list[] = {
    TEST(orders), TEST(early_stop), TEST(edge), TEST(bfs), TEST(memory)};

int main(int argc, char *argv[]) {
  return run_test_main(argc, argv, test_list, SIZE(test_list));
}
