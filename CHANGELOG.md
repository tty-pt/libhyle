# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] (target: v1.7.0)

### Added
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
