# libhyle

[![C99](https://img.shields.io/badge/C-C99-555?logo=c)](#)
[![BSD-2-Clause](https://img.shields.io/badge/License-BSD--2--Clause-blue)](#)
[![No UI / transport deps](https://img.shields.io/badge/UI%2Ftransport-none-brightgreen)](#)

> Pure C data / query kernel.

A pure C data/query kernel. Define your data models once as a schema, then get
query, filter, sort, paginate, full-text search, ordered positional arrays,
derived fields, and row validation — with no UI, no network, and no framework
in the picture. Whatever renders the data is your business.

## Contents

- [Features](#features)
- [Neutral by design](#neutral-by-design)
- [Install](#install)
- [Build from source](#build-from-source)
- [Quickstart](#quickstart)
- [API overview](#api-overview)
- [Query & filtering](#query--filtering)
- [Consumers & bindings](#consumers--bindings)
- [Documentation](#documentation)
- [Testing](#testing)
- [License](#license)

## Features

- **Canonical schema descriptors** — `hyle_schema_desc_t` with ergonomic
  `FIELD_TEXT` / `FIELD_INT` / `FIELD_BOOL` / `FIELD_REF` / `FIELD_INVERSE` /
  `FIELD_ARRAY_*` macros (plus the legacy positional `REC_FIELD` family)
- **Registry** — named sources with row + field stores: `hyle_registry_register`,
  `put`, `del`, `query`, and metadata accessors
- **Query pipeline** — filter → sort → paginate in one pass
  (`hyle_apply_view`), with full-text search over the source's identifier and
  string fields
- **Full-text search** — accent-sensitive substring matching via `stoma`
  (`pão` ≠ `pao`), punctuation-normalized variants, per-field filters
- **Ordered sources** — partitioned positional arrays (`hyle_ordered_append /
  insert_at / remove_at / clear`) with pluggable load/save persistence callbacks
- **Derived fields** — provider functions (`hyle_derive_register` /
  `hyle_derive_call`) that supply searchable values during index rebuild
- **Values** — `hyle_val_t` (16-byte tagged union: null/bool/int/float/string/array/map)
  with array/map operations and JSON serialization
- **Validation** — `hyle_purify_row` enforces required / int range / length /
  regex-pattern rules from the field schema
- **FFI row API** — `hyle_ffi_row_t` flat arrays for Rust and other language
  bindings
- **Query-string helper** — `hyle_qs_param` (URL-decode + key extraction)

## Neutral by design

hyle is a framework-neutral data kernel: `libhyle.so` contains **no bud or UI
symbols** (a maintained boundary gate), it never speaks HTTP, and it is storage
driver-agnostic — the persistence engine lives in the separate `libhyle-source`
library. It knows data, not widgets.

**Dependencies:** the `ttypt/corm` opaque data store (row/field maps) and
`stoma` tokenization/search (FTS). On Windows, the POSIX regex wrapper pulls in
`libpcre2-posix` / `libpcre2-8`. Nothing else.

## Install

Prebuilt packages are distributed on tty.pt for Linux (APT / Alpine / Arch /
Fedora-RHEL), macOS (Homebrew), Windows (winget / MSYS2), and OpenBSD.
Follow the [installation instructions](
https://github.com/tty-pt/ci/blob/main/docs/install.md) and use
**libhyle** as the package name.

## Build from source

The library builds with a plain `make` (the shared [`mk` include.mk](
https://github.com/tty-pt/mk)):

```sh
make          # lib/libhyle.so, lib/libhyle.a (Rust-FFI static), bin/hyle_test
make test     # self-test suite (hyle_test) + Zig bindings test (needs zig)
sudo make install   # lib, headers, and hyle.pc → $(PREFIX), default /usr/local
```

Link it from your own C code:

```sh
cc my_app.c $(pkg-config --cflags --libs hyle)
```

`hyle.pc` also carries the kernel's own dependencies (`-lhyle -lcorm -lstoma`).

## Quickstart

```c
#include <hyle/registry.h>
#include <hyle/query.h>
#include <hyle/field.h>
#include <stdio.h>

static const hyle_field_t song_fields[] = {
	{ .name = "title", .type = HYLE_FIELD_STRING, .writable = 1, .required = 1, .searchable = 1 },
	{ .name = "year",  .type = HYLE_FIELD_INT,    .writable = 1, .searchable = 1 },
};

int main(void)
{
	/* Register a source: the kernel creates row + field stores for it. */
	hyle_registry_register("song", song_fields, 2, 0, 0, NULL);

	/* Write a row (parallel name/value arrays). */
	const char *names[]  = { "title", "year" };
	const char *values[] = { "Ain't Talkin' 'Bout Love", "1995" };
	hyle_registry_put("song", "song_dance", names, values, 2);

	/* Query: full-text search over text fields, paginated. */
	hyle_query_t q = { .q = "love", .per_page = 10 };
	hyle_row_set_t rs;
	size_t total = 0;
	if (hyle_registry_query("song", &q, &rs, &total) == 0)
		printf("%zu rows match\n", total);

	hyle_row_set_free(&rs);
	return 0;
}
```

## API overview

Full signatures live in `include/hyle/*.h`. The umbrella `#include <hyle/hyle.h>`
pulls in the whole kernel.

**Context & values** (`ctx.h`, `value.h`) — `hyle_ctx_t`, `hyle_ctx_new` /
`hyle_ctx_free`; `hyle_val_t` constructors (`hyle_val_null/bool/int/float/string/
array/map`), array ops (`hyle_val_array_push/get/len`), map ops
(`hyle_val_map_set/get`), `hyle_val_to_json`.

**Fields & schemas** (`field.h`, `schema.h`) — `hyle_field_t` +
`hyle_field_type_t`; `hyle_schema_desc_t` (`HYLE_KIND_*`, `HYLE_CM_*`), the
`FIELD_*` / `OVERLAY_*` / legacy `*_FIELD` macros, `hyle_field_by_name`,
`hyle_field_is_reference`.

**Blueprint** (`blueprint.h`) — `hyle_blueprint_t` /
`hyle_blueprint_schema_t`, `hyle_blueprint_manifest`, `hyle_manifest_t`
(select/projects/filters/lookups/inlines per source), `hyle_manifest_clear`.

**Registry** (`registry.h`) — `hyle_registry_register`, `hyle_registry_put`,
`hyle_registry_del`, `hyle_registry_query`, accessors (`get_row_hd`,
`get_fields_hd`, `get_user`, `id_at`, `get_field_*`); FFI rows
(`hyle_ffi_row_t`, `hyle_row_set_to_rows`, `hyle_rows_free`); ordered sources
(`hyle_ordered_register/count/key_at/append/insert_at/remove_at/clear/save` +
`hyle_persist_load_fn` / `hyle_persist_save_fn`); derives
(`hyle_derive_register`, `hyle_derive_call`, `hyle_derive_fn_t`).

**Query** (`query.h`) — `hyle_query_t`, `hyle_parse_query` / `hyle_query_clear`,
`hyle_ci_substr` / `hyle_ci_substr_punct`, `hyle_filter_rows`,
`hyle_sort_rows`, `hyle_paginate`, `hyle_apply_view`.

**Validation** (`purify.h`) — `hyle_purify_row`, `hyle_purify_errors_free`,
`hyle_purify_error_t`.

**URL helpers** (`url.h`) — `hyle_qs_param`.

## Query & filtering

A query is filter → sort → paginate over the source's live maps:

- `q` — full-text substring search across the row id and all string fields
  (accent-sensitive: `pão` ≠ `pao`)
- `filters` — per-field matches, combined with `and` / `or` operators;
  multi-reference fields use newline-boundary token matching when the field
  schema is supplied
- `sort_field` / `sort_asc` — auto-detected numeric vs string sort
- `page` / `per_page` — pagination; `total` reports pre-pagination matches
- `include` — optional projection of fields

`hyle_parse_query` builds a `hyle_query_t` directly from a URL-encoded query
string (the string is tokenized in place; clear it with `hyle_query_clear`).

## Consumers & bindings

| Target | Where |
|--------|-------|
| C persistence engine (drivers, DSV, JSON overlays) | `../libhyle-source/` |
| Bud UI bridge (filters, pickers, state application) | `../libhyle-bud/` |
| Rust mirror (native + WASM) | `crates/hyle/` |
| Zig bindings (static lib + WASM module) | `zig-bindings/` |
| In-tree C self-test | `src/hyle_test.c` |

## Documentation

- [ARCHITECTURE.md](https://github.com/tty-pt/site/blob/main/docs/ARCHITECTURE.md)
  — where the kernel sits in the module graph
- [SCHEMA.md](https://github.com/tty-pt/site/blob/main/docs/SCHEMA.md) — how
  schema hint strings (filters, pickers) attach
- [FILTERS.md](https://github.com/tty-pt/site/blob/main/docs/FILTERS.md) — the
  query/filter contract shared by consumers
- [PUBLISH.md](./docs/PUBLISH.md) — release/install notes for this library

## Testing

```sh
make test       # ./bin/hyle_test (self-test suite) + zig-test via `zig build test`
```

`./bin/hyle_test` exercises the schema/registry/query/ordered/derive/purify/URL
paths and prints `N passed`; the Zig binding tests run under `zig-bindings/`.
From the repository root, `make boundary-check` runs the module-layer gates and
`make test` runs the full platform suite.

## License

BSD 2-Clause License. Copyright (c) 2026, tty-pt. See `LICENSE`.