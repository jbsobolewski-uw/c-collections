# Error handling

Every module in **c-collections** follows one error contract, defined in
[`src/collections_errors.h`](../src/collections_errors.h).

## Return codes

```c
#define COLLECTIONS_OK    0
#define COLLECTIONS_ERR (-1)
```

Each module aliases these under its own name, so the numbering is identical
everywhere:

| Module | Macros |
|---|---|
| list | `LIST_OK` / `LIST_ERR` |
| queue | `QUEUE_OK` / `QUEUE_ERR` |
| stack | `STACK_OK` / `STACK_ERR` |
| bst | `BST_OK` / `BST_ERR` |
| hashmap | `HASHMAP_OK` / `HASHMAP_ERR` |
| id_manager | `ID_MANAGER_OK` / `ID_MANAGER_ERR` |

## Conventions

1. **Constructors** (`*_create`) return a pointer; `NULL` means failure.
2. **All other fallible operations** return `*_OK` or `*_ERR`.
3. **Query results** are delivered through output parameters
   (e.g. `list_size(list, &size)`).
4. **`errno` always describes the cause** when a function fails:

| errno | Meaning |
|---|---|
| `EINVAL` | Invalid argument: `NULL` handle, `NULL` output pointer, invalid enum value, ID below the manager's minimum |
| `ENOMEM` | Memory allocation failed (also: hash table full and unable to grow) |
| `ENOENT` | Element not found, or the container is empty |
| `ENOSPC` | Resource exhausted (no identifiers left to assign) |

`collections_errors.h` includes `<errno.h>`, so including any module header is
enough to use the `E*` constants.

## Checking errors

```c
#include "src/list/list.h"

list_t *l = list_create(free);
if (!l) {
    /* errno == ENOMEM */
}

if (list_remove(l, obj) == LIST_ERR) {
    if (errno == ENOENT) { /* obj was not in the list */ }
}
```

One special case: `idm_assign_id` returns `UINT32_MAX` on failure, but
`UINT32_MAX` is also the last valid identifier. Zero `errno` before the call
to tell the two apart — see [id_manager.md](id_manager.md).
