# bst — binary search tree

| | |
|---|---|
| Header | `src/bst/bst.h` |
| Sources | `src/bst/bst.c` |
| Library targets | `bstlib` (static), `bstlib_shared` (dynamic) |

A binary search tree ordered by a user-supplied comparator. The core module
covers creation, insertion, removal, lookup and size queries; traversal
algorithms live in the separate, optional
[bst_traversals](bst_traversals.md) module.

## Types

```c
typedef struct bst      bst_t;        /* opaque tree handle */
typedef struct bst_node bst_node_t;   /* opaque node handle (traversals only) */

typedef void (*object_destructor_function_t)(void *);
typedef int  (*object_comparator_function_t)(void *obj1, void *obj2);
```

The comparator follows the `strcmp` convention: negative when `obj1 < obj2`,
`0` (`BST_EQ`) when equal, positive when `obj1 > obj2`. The header also
defines the `BST_LEQ`, `BST_LE`, `BST_EQ`, `BST_GR`, `BST_GREQ` macros for
comparators that want to express the result symbolically.

Two ready-made comparators are provided:

```c
int bst_cmp_signed_int(void *a, void *b);    /* *(signed long *)   */
int bst_cmp_unsigned_int(void *a, void *b);  /* *(unsigned long *) */
```

## API

### `bst_t *bst_create(object_comparator_function_t cmp, object_destructor_function_t dtor)`
Creates an empty tree. If `cmp` is `NULL`, `bst_cmp_signed_int` is used.
`dtor` may be `NULL`.
**Returns:** the new tree, or `NULL` with `errno = ENOMEM`.

### `int bst_destroy(bst_t *tree)`
Destroys the tree, invoking the destructor on every remaining element
(post-order) and freeing all nodes including the recycling pool.
**Errors:** `EINVAL`.

### `int bst_insert(bst_t *tree, void *data)`
Inserts `data` according to the comparator. Duplicates (comparator result
`<= BST_EQ`) go to the left branch, so duplicate keys are allowed.
O(h), where h is the tree height.
**Errors:** `EINVAL`, `ENOMEM`.

### `int bst_remove(bst_t *tree, void *data)`
Removes one element comparing equal to `data` and invokes the destructor on
it. Handles all three shapes: leaf, one child, and two children (the
in-order successor's data is moved into place). O(h).
**Errors:** `EINVAL`, `ENOENT` (no matching element).

### `int bst_search(bst_t *tree, void *data, void **out_data)`
Finds an element comparing equal to `data` and stores the stored pointer in
`*out_data`. The element stays in the tree. O(h).
**Errors:** `EINVAL`, `ENOENT`.

### `int bst_size(bst_t *tree, size_t *out_size)` / `int bst_is_empty(bst_t *tree, int *out_is_empty)`
Size / emptiness queries via output parameters. O(1). **Errors:** `EINVAL`.

## Internal accessors

```c
bst_node_t *bst_get_root(bst_t *tree);
bst_node_t *bst_node_get_left(bst_node_t *node);
bst_node_t *bst_node_get_right(bst_node_t *node);
void       *bst_node_get_data(bst_node_t *node);
```

These exist so `bst_traversals.c` can walk the tree without the node layout
being exposed. They are not intended for general use, but they are safe:
each returns `NULL` for a `NULL` input.

## Complexity and caveats

- The tree is **not self-balancing**. Operations are O(h); inserting sorted
  data degrades the tree to a linked list (h = n). Insert in random order or
  use a different structure when adversarial input is possible.
- `bst_remove` and `bst_destroy` use recursion proportional to the tree
  height; extremely degenerate trees can exhaust the call stack.
- Comparators must be consistent (a total order); an inconsistent comparator
  silently corrupts the tree's invariants.

## Implementation notes

- **Node recycling:** removed nodes are pooled (the `left` pointer doubles as
  the pool's "next" link) and reused by later insertions; `bst_destroy`
  releases the pool.
- During a two-child removal the destructor is temporarily disabled while the
  successor node is unlinked, so the data that was just moved into the target
  node is not destroyed with it.

## Example

```c
bst_t *t = bst_create(NULL, free);   /* signed long comparator */

long *v = malloc(sizeof *v);
*v = 42;
bst_insert(t, v);

long key = 42;
void *found;
if (bst_search(t, &key, &found) == BST_OK) {
    /* *(long *)found == 42 */
}

bst_remove(t, &key);   /* frees the element */
bst_destroy(t);
```
