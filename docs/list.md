# list — singly linked list

| | |
|---|---|
| Header | `src/list/list.h` |
| Sources | `src/list/list.c` |
| Library targets | `listlib` (static), `listlib_shared` (dynamic) |

A generic singly linked list storing `void *` elements. Ideal for linear
collections whose final size is not known up front. The node structure is
fully hidden behind the opaque `list_t` type.

## Types

```c
typedef struct list list_t;                              /* opaque handle */
typedef void (*object_destructor_function_t)(void *);    /* element cleanup */
typedef int  (*object_job_function_t)(void *obj, void *argstruct);
```

The destructor is invoked on an element's data whenever the list itself
disposes of the element (`list_remove`, `list_destroy`). Pass `NULL` at
creation if the list does not own its elements.

A job function (used by `list_foreach`) returns `0` to continue iteration and
any non-zero value to stop early.

## API

### `list_t *list_create(object_destructor_function_t dtor)`
Creates an empty list. `dtor` may be `NULL`.
**Returns:** the new list, or `NULL` with `errno = ENOMEM`.

### `int list_destroy(list_t *list)`
Destroys the list. The destructor (if any) runs on every remaining element;
all nodes, including the internal recycling pool, are freed.
**Returns:** `LIST_OK`, or `LIST_ERR` with `errno = EINVAL` if `list` is `NULL`.

### `int list_add(list_t *list, void *data)`
Prepends `data` to the front of the list. O(1).
**Errors:** `EINVAL` (`NULL` list), `ENOMEM` (node allocation failed).

### `int list_remove(list_t *list, void *data)`
Removes the first node whose stored pointer equals `data` (pointer identity,
not content comparison) and invokes the destructor on it. O(n).
**Errors:** `EINVAL` (`NULL` list), `ENOENT` (no such element).

### `int list_size(list_t *list, size_t *out_size)`
Stores the element count in `*out_size`. O(1).
**Errors:** `EINVAL` (`NULL` list or `NULL` out pointer).

### `int list_is_empty(list_t *list, int *out_is_empty)`
Stores `1` in `*out_is_empty` if the list is empty, `0` otherwise.
**Errors:** `EINVAL`.

### `int list_foreach(list_t *list, object_job_function_t job, void *argstruct)`
Calls `job(element, argstruct)` on every element, front to back. A non-zero
return from `job` stops the iteration; `list_foreach` still returns `LIST_OK`
in that case.
**Errors:** `EINVAL` (`NULL` list or `NULL` job).

## Ownership semantics

The list stores raw pointers; it never copies element data. If a destructor
was provided, the list *owns* its elements: `list_remove` and `list_destroy`
free them. Without a destructor the caller keeps ownership.

## Implementation notes

- **Node recycling (free list):** removed nodes are not returned to the
  system with `free()`; they are pushed onto an internal pool and reused by
  subsequent insertions. This trades a little peak memory for fewer allocator
  round-trips. `list_destroy` releases the pool.
- Do not modify the list from inside a `list_foreach` job.

## Example

```c
list_t *l = list_create(free);

char *s = strdup("hello");
list_add(l, s);

size_t n;
list_size(l, &n);          /* n == 1 */

list_remove(l, s);         /* also frees s via the destructor */
list_destroy(l);
```
