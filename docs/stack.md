# stack — LIFO stack

| | |
|---|---|
| Header | `src/stack/stack.h` |
| Sources | `src/stack/stack.c` |
| Library targets | `stacklib` (static), `stacklib_shared` (dynamic) |

A **Last-In, First-Out** stack storing `void *` elements. All operations work
on the top of the structure, so `stack_push` and `stack_pop` are O(1).
Typical uses: backtracking algorithms, state machines, parsers.

## Types

```c
typedef struct stack stack_t;                            /* opaque handle */
typedef void (*object_destructor_function_t)(void *);
typedef int  (*object_job_function_t)(void *obj, void *argstruct);
```

## API

### `stack_t *stack_create(object_destructor_function_t dtor)`
Creates an empty stack. `dtor` may be `NULL`.
**Returns:** the new stack, or `NULL` with `errno = ENOMEM`.

### `int stack_destroy(stack_t *stack)`
Destroys the stack, invoking the destructor on every remaining element and
freeing all nodes including the recycling pool.
**Errors:** `EINVAL`.

### `int stack_push(stack_t *stack, void *data)`
Pushes `data` onto the top. O(1).
**Errors:** `EINVAL`, `ENOMEM`.

### `int stack_pop(stack_t *stack, void **out_data)`
Pops the top element. O(1).

- If `out_data` is non-`NULL`, the caller receives the pointer and takes
  ownership.
- If `out_data` is `NULL`, the element is destroyed with the destructor
  (if one was provided).

**Errors:** `EINVAL` (`NULL` stack), `ENOENT` (stack is empty).

### `int stack_size(stack_t *stack, size_t *out_size)`
Stores the element count in `*out_size`. O(1). **Errors:** `EINVAL`.

### `int stack_is_empty(stack_t *stack, int *out_is_empty)`
Stores `1`/`0` in `*out_is_empty`. **Errors:** `EINVAL`.

### `int stack_foreach(stack_t *stack, object_job_function_t job, void *argstruct)`
Calls `job` on every element, starting at the top and walking down. Non-zero
from `job` stops the walk; the function still returns `STACK_OK`.
**Errors:** `EINVAL`.

## Ownership semantics

Identical to the queue: with a destructor the stack owns its elements;
`stack_pop(s, NULL)` and `stack_destroy` free them; handing an element out
through `out_data` transfers ownership to the caller.

## Implementation notes

- **Node recycling (free list):** popped nodes are pooled and reused by later
  pushes; the pool is released by `stack_destroy`.

## Example

```c
stack_t *s = stack_create(free);
stack_push(s, strdup("bottom"));
stack_push(s, strdup("top"));

char *top;
stack_pop(s, (void **) &top);   /* top == "top" */
free(top);

stack_destroy(s);               /* frees "bottom" */
```
