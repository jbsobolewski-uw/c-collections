# id_manager — unique identifier allocator

| | |
|---|---|
| Header | `src/id_manager/id_manager.h` |
| Sources | `src/id_manager/id_manager.c` |
| Library targets | `idlib` (static), `idlib_shared` (dynamic) |

Generates and recycles unique 32-bit identifiers. New IDs are issued
sequentially from a counter; released IDs go onto an internal stack and are
reused before any new ID is generated. Both assignment and release are O(1).

## Types

```c
typedef struct IdManager id_manager_t;   /* opaque handle */
```

## API

### `id_manager_t *idm_create(uint32_t first_id)`
Creates a manager whose lowest issued ID is `first_id`.
**Returns:** the new manager, or `NULL` with `errno = ENOMEM`.

### `int idm_destroy(id_manager_t *mgr)`
Destroys the manager, freeing the recycled-ID stack and the node pool.
**Errors:** `EINVAL`.

### `uint32_t idm_assign_id(id_manager_t *mgr)`
Acquires an ID. Recycled IDs are preferred; otherwise the next sequential ID
is issued. O(1), and never allocates.
**Returns:** the ID, or `UINT32_MAX` on error with `errno = EINVAL`
(`NULL` manager) or `ENOSPC` (ID space exhausted).

> **Sentinel caveat:** `UINT32_MAX` is also the *last valid ID*. To tell a
> legitimate `UINT32_MAX` apart from a failure, zero `errno` before the call:
>
> ```c
> errno = 0;
> uint32_t id = idm_assign_id(mgr);
> if (id == UINT32_MAX && errno != 0) { /* failure */ }
> ```

### `int idm_release_id(id_manager_t *mgr, uint32_t id)`
Returns `id` to the manager for reuse. O(1).
**Errors:** `EINVAL` (`NULL` manager, or `id` below `first_id` — an ID this
manager could never have issued), `ENOMEM` (a tracking node had to be
allocated and the allocation failed).

The manager does **not** track which IDs are currently outstanding: releasing
the same ID twice, or an ID that was never assigned (but is `>= first_id`),
is not detected and will lead to duplicate assignments later. Correct pairing
of assign/release is the caller's responsibility.

### `int idm_is_available(id_manager_t *mgr)`
**Returns:** `1` if an ID can currently be acquired, `0` otherwise
(`errno = EINVAL` if `mgr` is `NULL`).

## Implementation notes

- **Two internal stacks:** `recycled_ids` holds released IDs awaiting reuse;
  `node_pool` holds empty list nodes. Popping a recycled ID moves its node to
  the pool instead of freeing it, and releasing an ID grabs a pooled node
  before resorting to `malloc` — so steady-state assign/release cycles
  perform **zero allocator calls**.
- Releasing the most recently issued sequential ID simply decrements the
  counter instead of touching the stacks.
- The sequential counter never wraps: once it reaches `UINT32_MAX` the
  manager is exhausted (`ENOSPC`) until IDs are released.

## Example

```c
id_manager_t *m = idm_create(100);

uint32_t a = idm_assign_id(m);   /* 100 */
uint32_t b = idm_assign_id(m);   /* 101 */

idm_release_id(m, a);
uint32_t c = idm_assign_id(m);   /* 100 again (recycled) */

idm_destroy(m);
```
