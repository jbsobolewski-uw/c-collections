# hashmap — open-addressing hash map

| | |
|---|---|
| Header | `src/hashmap/hashmap.h` |
| Sources | `src/hashmap/hashmap.c` |
| Library targets | `hashmaplib` (static), `hashmaplib_shared` (dynamic) |

A hash map keyed by **32-bit unsigned integers** (`uint32_t`) storing
`void *` values. Uses open addressing with linear probing and a bit-mixing
integer hash for key distribution. Not a generic-key map: if you need string
or struct keys, hash them to a `uint32_t` first.

## Types

```c
typedef struct HashMap hash_map_t;                   /* opaque handle */
typedef void (hm_object_destroyer_func_t)(void *value);
```

Note the destroyer typedef is a *function type* (not a pointer type);
parameters take `hm_object_destroyer_func_t *`.

## API

### `hash_map_t *hm_create(size_t capacity, hm_object_destroyer_func_t *destroyer)`
Creates a map with at least `capacity` slots (raised to the internal minimum
of 64 if smaller). `destroyer` may be `NULL`.
**Returns:** the new map, or `NULL` with `errno = ENOMEM`.

### `int hm_insert(hash_map_t *map, uint32_t key, void *value)`
Inserts or updates the pair. If the key already exists, the old value is
destroyed (when a destroyer is set) and replaced. Amortized O(1).
**Errors:** `EINVAL` (`NULL` map), `ENOMEM` (table completely full and unable
to grow).

### `void *hm_get(hash_map_t *map, uint32_t key)`
Looks up the value for `key`. The entry stays in the map. O(1) expected.
**Returns:** the stored pointer, or `NULL` with `errno = EINVAL` (`NULL` map)
or `ENOENT` (key not present). Since a stored value may itself be `NULL`,
check `errno` to distinguish "stored NULL" from "not found".

### `int hm_remove(hash_map_t *map, uint32_t key)`
Removes the entry, destroying its value if a destroyer is set, then rehashes
the following probe cluster so linear-probing chains stay intact.
**Errors:** `EINVAL`, `ENOENT` (key not present).

### `int hm_destroy(hash_map_t *map)`
Destroys the map. With a destroyer set, all stored values are destroyed too.
**Errors:** `EINVAL`.

### `int hm_size(hash_map_t *map, size_t *out_size)`
Stores the element count in `*out_size`. O(1). **Errors:** `EINVAL`.

## Growth and load factor

- The table doubles in capacity when an insert finds the load factor at or
  above **3/4**, rehashing every entry into the new table.
- A failed growth (allocation failure) is **tolerated**: the map stays intact
  and inserts continue to succeed while free slots remain. Only when the
  table is completely full and cannot grow does `hm_insert` fail with
  `ENOMEM`. Expect degraded (long-probe) performance in that state.

## Ownership semantics

With a destroyer set, the map owns its values: they are destroyed on
overwrite (`hm_insert` on an existing key), on `hm_remove`, and on
`hm_destroy`. Keys are plain integers and are simply copied.

## Example

```c
hash_map_t *m = hm_create(0, free);   /* default capacity, owns values */

int *v = malloc(sizeof *v);
*v = 7;
hm_insert(m, 1234, v);

errno = 0;
int *got = hm_get(m, 1234);           /* *got == 7 */

hm_remove(m, 1234);                   /* frees v */
hm_destroy(m);
```
