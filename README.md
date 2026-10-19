<p align="center">
  <img src="assets/logo.png" alt="hyle logo" width="220" />
</p>

# libhyle

[![C99](https://img.shields.io/badge/C-C99-555?logo=c)](#)
[![BSD-2-Clause](https://img.shields.io/badge/License-BSD--2--Clause-blue)](#)
[![No UI / transport deps](https://img.shields.io/badge/UI%2Ftransport-none-brightgreen)](#)
[![Data Kernel](https://img.shields.io/badge/kernel-data%20%2B%20query-16A34A)](#)

> Define your data schema once in pure C and get instant full-text search, filtering, sorting, pagination, and row validation without SQL overhead.

---

## Architecture & Storage Integration

`libhyle` connects low-level hash maps and tokenizers to high-level application schemas:
- **Storage Engine**: Built on [libcorm](https://github.com/tty-pt/libcorm) for opaque fast row and field hash stores.
- **Search Tokenizer**: Uses [liblemma](https://github.com/tty-pt/liblemma) for accent-preserving full-text substring and phrase search.
- **UI Bridge**: Bridges directly to [libbud](https://github.com/tty-pt/libbud) via `libhyle-bud` to bind schema fields to isomorphic DOM tables and filters.

---

## Quickstart

Register a typed data schema, insert records, and run a paginated full-text search query in 20 lines:

```c
#include <stdio.h>
#include <hyle/registry.h>
#include <hyle/query.h>
#include <hyle/field.h>

static const hyle_field_t song_fields[] = {
    { .name = "title", .type = HYLE_FIELD_STRING, .writable = 1, .required = 1, .searchable = 1 },
    { .name = "year",  .type = HYLE_FIELD_INT,    .writable = 1, .searchable = 1 },
};

int main(void) {
    // 1. Register data source with schema
    hyle_registry_register("song", song_fields, 2, 0, 0, NULL);

    // 2. Insert record
    const char *names[] = { "title", "year" };
    const char *values[] = { "Ain't Talkin' 'Bout Love", "1978" };
    hyle_registry_put("song", "song_1", names, values, 2);

    // 3. Query: full-text search across string fields, paginated
    hyle_query_t q = { .q = "Love", .per_page = 10 };
    hyle_row_set_t rs;
    size_t total = 0;
    if (hyle_registry_query("song", &q, &rs, &total) == 0) {
        printf("Matched %zu song(s)\n", total);
        hyle_row_set_free(&rs);
    }
    return 0;
}
```

Compile and link:
```sh
cc catalog_demo.c $(pkg-config --cflags --libs hyle)
```

---

## Core API (The Essential 20%)

| Function | Signature | Description |
| :--- | :--- | :--- |
| `hyle_registry_register` | `int hyle_registry_register(name, fields, n, ...)` | Register source and allocate row/field stores |
| `hyle_registry_put` | `int hyle_registry_put(source, id, names, vals, n)` | Insert or update a structured row |
| `hyle_registry_query` | `int hyle_registry_query(source, q, rs, &total)` | Run filter → sort → paginate query |
| `hyle_parse_query` | `int hyle_parse_query(&q, query_str)` | Parse URL-encoded query string directly |
| `hyle_purify_row` | `int hyle_purify_row(source, names, vals, n, errs)` | Enforce schema validation (types, regex, ranges) |
| `hyle_val_to_json` | `char *hyle_val_to_json(hyle_val_t val)` | Serialize 16-byte tagged values to JSON |

👉 *For blueprint manifests, ordered positional arrays, and Rust/Zig FFI bindings, see `include/hyle/hyle.h` and [docs/PUBLISH.md](docs/PUBLISH.md).*

---

## Design Philosophy: Neutral by Design

* **Zero UI or Network Coupling**: `libhyle.so` contains zero bud or HTTP symbols. It manages pure data and schemas, letting any transport or renderer handle presentation.
* **Accent-Sensitive Correctness**: Search powered by [liblemma](https://github.com/tty-pt/liblemma) guarantees `pão ≠ pao` while keeping queries fast and predictable.
* **Multi-Language Bindings**: First-class FFI row formats support Rust (`crates/hyle/`) and Zig (`zig-bindings/`) mirrors without memory copies.

---

## Installation & Build

### Prebuilt Packages
Prebuilt packages are distributed on tty.pt for Linux (APT / Alpine / Arch / Fedora-RHEL), macOS (Homebrew), Windows (winget / MSYS2), and OpenBSD. Follow the [installation instructions](https://github.com/tty-pt/ci/blob/main/docs/install.md) and use **libhyle** as the package name.

### Build from Source
```sh
make                  # builds lib/libhyle.so + bin/hyle_test
make test             # run in-tree unit and schema tests
sudo make install     # install headers and library to system
```

## License

BSD 2-Clause License. Copyright (c) 2026, tty-pt. See `LICENSE`.
