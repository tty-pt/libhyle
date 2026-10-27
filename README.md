<p align="center">
  <img src="assets/logo.png" alt="hyle logo" width="220" />
</p>

# libhyle

[![C99](https://img.shields.io/badge/C-C99-555?logo=c)](#)
[![BSD-2-Clause](https://img.shields.io/badge/License-BSD--2--Clause-blue)](#)
[![No UI / transport deps](https://img.shields.io/badge/UI%2Ftransport-none-brightgreen)](#)
[![Data Kernel](https://img.shields.io/badge/kernel-data%20%2B%20query-16A34A)](#)

> Define your data schema once in pure C and get instant full-text search, filtering, sorting, pagination, and row validation without SQL overhead. Canonical header is `<hyle/hyle.h>`.

---

## 1. Quickstart

Register a typed data schema, insert records, and run a paginated full-text search query in under 20 lines:

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
cc catalog_demo.c $(pkg-config --cflags --libs hyle) -o catalog_demo
```

---

## 2. Core API Reference

| Function | Signature | Description |
| :--- | :--- | :--- |
| `hyle_registry_register` | `int hyle_registry_register(name, fields, n, ...)` | Register source and allocate row/field stores |
| `hyle_registry_put` | `int hyle_registry_put(source, id, names, vals, n)` | Insert or update a structured row |
| `hyle_registry_query` | `int hyle_registry_query(source, q, rs, &total)` | Run filter → sort → paginate query |
| `hyle_parse_query` | `int hyle_parse_query(&q, query_str)` | Parse URL-encoded query string directly |
| `hyle_purify_row` | `int hyle_purify_row(source, names, vals, n, errs)` | Enforce schema validation (types, regex, ranges) |
| `hyle_val_to_json` | `char *hyle_val_to_json(hyle_val_t val)` | Serialize 16-byte tagged values to JSON |

---

## 3. Field Types & Schema Attributes

| Field Type | Enum Value | Description |
| :--- | :--- | :--- |
| **String** | `HYLE_FIELD_STRING` | UTF-8 text string; tokenized for FTS when `searchable = 1` |
| **Integer** | `HYLE_FIELD_INT` | 64-bit signed integer; supports numeric range filters |
| **Float** | `HYLE_FIELD_FLOAT` | 64-bit IEEE double; supports numeric range filters |
| **Boolean** | `HYLE_FIELD_BOOL` | Single-byte flag (`0` or `1`) |
| **Reference** | `HYLE_FIELD_REF` | Foreign key referencing another dataset's primary key |
| **Blob** | `HYLE_FIELD_BLOB` | Raw opaque binary data with length prefix |

*Attributes: `.required = 1` (must be present on write), `.writable = 1` (editable by user), `.searchable = 1` (lexically indexed for `q=` queries).*

---

## 4. Query String Grammar (`hyle_parse_query`)

`libhyle` parses URL query strings directly into typed query structs:

| Parameter | Example | Behavior |
| :--- | :--- | :--- |
| `q` | `?q=love` | Lexical full-text search across all searchable string fields |
| `filter_<field>` | `?filter_year=1978` | Exact-match equality filter on a specific field |
| `sort` | `?sort=year` | Schema field name to order results by |
| `order` | `?order=desc` | Ordering direction: `asc` or `desc` (default: `asc`) |
| `page` / `per_page` | `?page=2&per_page=20` | 1-based page number and page size |

---

## 5. Installation & Build

### Prebuilt Packages
Prebuilt packages are distributed on tty.pt for Linux (APT / Alpine / Arch / Fedora-RHEL), macOS (Homebrew), Windows (winget / MSYS2), and OpenBSD. Follow the [installation instructions](https://github.com/tty-pt/ci/blob/main/docs/install.md) and use **libhyle** as the package name.

### Build from Source
```sh
make                  # builds lib/libhyle.so
make test             # run unit, query, and validation tests
sudo make install     # install headers and library to system
```

## License

BSD 2-Clause License. Copyright (c) 2025, tty-pt. See `LICENSE`.
