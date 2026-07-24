# queue — FIFO queue

| | |
|---|---|
| Header | `src/queue/queue.h` |
| Sources | `src/queue/queue.c` |
| Library targets | `queuelib` (static), `queuelib_shared` (dynamic) |

A classic **First-In, First-Out** queue storing `void *` elements. Both ends
are tracked internally (`head` for removal, `tail` for insertion), so
`queue_enqueue` and `queue_dequeue` are O(1).

## Types

```c
typedef struct queue queue_t;                            /* opaque handle */
typedef void (*object_destructor_function_t)(void *);
typedef int  (*object_job_function_t)(void *obj, void *argstruct);
```

## API

### `queue_t *queue_create(object_destructor_function_t dtor)`
Creates an empty queue. `dtor` may be `NULL`.
**Returns:** the new queue, or `NULL` with `errno = ENOMEM`.

### `int queue_destroy(queue_t *q)`
Destroys the queue, invoking the destructor on every remaining element and
freeing all nodes including the recycling pool.
**Errors:** `EINVAL`.

### `int queue_enqueue(queue_t *q, void *data)`
Appends `data` to the back. O(1).
**Errors:** `EINVAL`, `ENOMEM`.

### `int queue_dequeue(queue_t *q, void **out_data)`
Removes the element at the front. O(1).

- If `out_data` is non-`NULL`, the caller receives the pointer and takes
  ownership of it.
- If `out_data` is `NULL`, the element is destroyed with the destructor
  (if one was provided) — a convenient "pop and drop".

**Errors:** `EINVAL` (`NULL` queue), `ENOENT` (queue is empty).

### `int queue_size(queue_t *q, size_t *out_size)`
Stores the element count in `*out_size`. O(1). **Errors:** `EINVAL`.

### `int queue_is_empty(queue_t *q, int *out_is_empty)`
Stores `1`/`0` in `*out_is_empty`. **Errors:** `EINVAL`.

### `int queue_foreach(queue_t *q, object_job_function_t job, void *argstruct)`
Calls `job` on every element from front to back. Non-zero from `job` stops
the walk; the function still returns `QUEUE_OK`.
**Errors:** `EINVAL`.

## Ownership semantics

Same as the list: raw pointers, no copies. With a destructor the queue owns
its elements; `queue_dequeue(q, NULL)` and `queue_destroy` free them. When an
element is handed out through `out_data`, ownership transfers to the caller.

## Implementation notes

- **Node recycling (free list):** dequeued nodes go onto an internal pool and
  are reused by later enqueues; the pool is released by `queue_destroy`.
- The queue is used internally by the BST's breadth-first traversal
  (see [bst_traversals.md](bst_traversals.md)), which is why `bstlib` links
  against `queuelib`.

## Example

```c
queue_t *q = queue_create(free);
queue_enqueue(q, strdup("first"));
queue_enqueue(q, strdup("second"));

char *s;
queue_dequeue(q, (void **) &s);   /* s == "first", caller owns it */
free(s);

queue_dequeue(q, NULL);           /* "second" destroyed internally */
queue_destroy(q);
```
