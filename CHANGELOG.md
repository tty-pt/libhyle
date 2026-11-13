# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.7.3] - 2026-11-13

- **Packaging**:
  - Requires `@tty-pt/stele` >=1.6.2 (`^1.6.2`).
  - The npm tarball is built once by CI, with a self-contained native addon per platform (`prebuilds/<os>-<cpu>/`) next to `hyle.wasm`, and published as verified.

## [1.7.2] - 2026-11-13

- **Disjoint Global-Base (`hyle.wasm`)**: rebuilt the shared-memory reactor at a disjoint `--global-base` so it cannot collide with other co-loaded WASM modules.

## [1.7.1] - 2026-11-13

### Fixed
- **`hyle.wasm` imported no shared memory (`Makefile`)**: the 1.7.0 tarball shipped a `hyle.wasm` that
  defined its own private linear memory. `validateRow` copies strings into the Stele substrate memory, so
  the module read every value as empty and rejected valid rows with `required` whenever the native addon
  was absent (browsers, and Node without a prebuilt addon). Restored the `--import-memory` reactor link
  (`env.memory`), byte-identical to the 1.6.x binary.
- Added a WASM-only regression test (`tests/hyle.test.mjs`) that runs `validateRow` with no native addon.

## [1.7.0] - 2026-11-13

### Added
- **Field-Weighted Search Ranking (`src/registry.c`, `src/view.c`)**:
  - Implemented field relevance scoring in `prefilter_q()` prioritizing exact and prefix matches on primary display fields (`title`, `name`, `id`) over secondary references and content/chords.
  - Added `hyle_ci_match_score()` to compute precision tiers (exact: 10x, prefix: 4x, word-boundary: 2x, substring: 1x).
  - Added optional `search_weight` to `hyle_field_t` and `hyle_schema_desc_t` for custom field weight configuration.
- **Sovereign Design System Styles (`htdocs/hyle.css`)**:
  - Added comprehensive CSS styling rules for Hyle data tables, filters, forms, and pickers.

## [1.6.1] - 2026-11-03

- **Binary Token Array Storage & Rotating Ring Buffer (`src/value.c`, `src/field.c`)**:
  - Implemented binary token array representations for compact field storage.
  - Added rotating ring buffer for lazy zero-copy SSR field decoding and serialization.
- **Standalone `@tty-pt/hyle` Package & WebAssembly Schema Validation**:
  - Published `@tty-pt/hyle` providing TypeScript-first schema definitions and WASM-backed row validation (`hyle.wasm`).
  - Added `npm/wasm/hyle_wasm.c` exporting `hyle_wasm_validate_field` via `ISO_EXPORT`.
  - Adopted strict `peerDependencies` (`@tty-pt/stele: ">=1.5.0"`) and `devDependencies` (`@tty-pt/stele: "^1.5.2"`).
  - Migrated build to centralized `make wasm` in `external/mk/include.mk` with post-link `wasm-opt -O3` optimization.

## [1.4.0]

- **Lemma Full-Text Search Integration**: Updated dataset registry and search query pipelines to consume `liblemma` instead of `libstoma`.
- **Corm 2.0 Compatibility**: Verified full compatibility with modernized Corm hashtable and record schema primitives.
- **Documentation & Visual Assets**: Updated README and added project branding assets.
