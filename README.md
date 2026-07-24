# c-collections

> A java-style collections package for C.

Generic (`void *`), encapsulated (opaque pointers) data structures with a
uniform error contract and destructor-based memory management.

## Quick start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

See [docs/building.md](docs/building.md) for targets, flags and build types.

## Modules

| Module | Header | Docs |
|---|---|---|
| Singly linked list | `src/list/list.h` | [docs/list.md](docs/list.md) |
| FIFO queue | `src/queue/queue.h` | [docs/queue.md](docs/queue.md) |
| LIFO stack | `src/stack/stack.h` | [docs/stack.md](docs/stack.md) |
| Binary search tree | `src/bst/bst.h` | [docs/bst.md](docs/bst.md) |
| BST traversals (optional add-on) | `src/bst/bst_traversals.h` | [docs/bst_traversals.md](docs/bst_traversals.md) |
| Hash map (uint32 keys) | `src/hashmap/hashmap.h` | [docs/hashmap.md](docs/hashmap.md) |
| ID manager | `src/id_manager/id_manager.h` | [docs/id_manager.md](docs/id_manager.md) |

`src/collections.h` is an umbrella header including the core modules.
Error handling (shared return codes and `errno` semantics) is described in
[docs/error-handling.md](docs/error-handling.md).

## Layout

```
src/            library sources, one directory per module
src/*.cmake     compiler and linker flag definitions
docs/           per-module documentation
test/           tests
```

## License

See [LICENSE.md](LICENSE.md).
