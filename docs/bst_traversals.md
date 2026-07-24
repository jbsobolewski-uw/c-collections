# bst_traversals — tree traversal algorithms

| | |
|---|---|
| Header | `src/bst/bst_traversals.h` |
| Sources | `src/bst/bst_traversals.c` |
| Library targets | part of `bstlib` / `bstlib_shared` |

An optional add-on to the core [bst](bst.md) module. Include
`bst_traversals.h` only when you need to walk the tree; users of the core
BST API never pull in this header (`bst_traversals.h` includes `bst.h`,
never the other way around).

Although the traversal code is archived inside `bstlib`, it lives in its own
translation unit — a program that never calls `bst_apply` will not have the
traversal code (or its queue dependency) linked into the final binary when
linking statically.

## Types

```c
typedef enum {
    BST_TRAVERSAL_NLR,  /* Pre-order:          Node, Left, Right  (DFS) */
    BST_TRAVERSAL_LNR,  /* In-order:           Left, Node, Right  (DFS) - ascending */
    BST_TRAVERSAL_LRN,  /* Post-order:         Left, Right, Node  (DFS) */
    BST_TRAVERSAL_NRL,  /* Reverse pre-order:  Node, Right, Left  (DFS) */
    BST_TRAVERSAL_RNL,  /* Reverse in-order:   Right, Node, Left  (DFS) - descending */
    BST_TRAVERSAL_RLN,  /* Reverse post-order: Right, Left, Node  (DFS) */
    BST_TRAVERSAL_BFS   /* Level-order (breadth-first) */
} bst_traversal_t;

typedef int (*object_job_function_t)(void *obj, void *argstruct);
```

## API

### `int bst_apply(bst_t *tree, bst_traversal_t order, object_job_function_t job, void *argstruct)`

Walks the tree in the given order, calling `job(element, argstruct)` on every
element. A non-zero return from `job` stops the traversal early and is
propagated as `bst_apply`'s return value (unlike the `*_foreach` functions,
which return `*_OK` after an early stop). An empty tree is a no-op success.

**Errors:**

| errno | Cause |
|---|---|
| `EINVAL` | `tree` or `job` is `NULL`, or `order` is not a valid `bst_traversal_t` value |
| `ENOMEM` | BFS only: the internal queue could not be created or grown |

**Complexity:** O(n) visits. The six DFS orders use recursion proportional to
the tree height; BFS uses an internal [queue](queue.md) (created without a
destructor — the nodes belong to the tree) and additional memory proportional
to the widest tree level.

## Choosing an order

- `BST_TRAVERSAL_LNR` — elements in ascending comparator order (sorting).
- `BST_TRAVERSAL_RNL` — descending order.
- `BST_TRAVERSAL_LRN` — children before parents (safe for tear-down logic).
- `BST_TRAVERSAL_NLR` — parents before children (copying / serializing).
- `BST_TRAVERSAL_BFS` — level by level, left to right within a level.

## Example

```c
static int print_long(void *obj, void *unused) {
    (void) unused;
    printf("%ld\n", *(long *) obj);
    return 0;               /* continue; non-zero would stop the walk */
}

bst_apply(tree, BST_TRAVERSAL_LNR, print_long, NULL);  /* ascending dump */
```

## Design notes

The traversal module accesses the tree exclusively through the accessor
functions exported by `bst.h` (`bst_get_root`, `bst_node_get_left/right/data`),
so `struct bst_node` stays opaque and the two modules remain separable.
