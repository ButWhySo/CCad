# Sprint 26 Design: Library Catalog File Checks

## Purpose

Local catalogs must not only carry provenance metadata; they must also prove that referenced CCad-native library artifacts exist locally and match recorded checksums. This keeps future KiCad/Gitee/GitHub bulk ingestion offline-friendly and tamper-detectable.

## Scope

- Add a root-aware overload of `validateLibraryCatalog`.
- Add optional `--root <native-library-root>` to `ccad lib catalog-validate`.
- Verify that every non-empty `native_path` exists under the root.
- Verify SHA-256 of the local native artifact against the catalog `sha256` field.
- Keep validation local-only and deterministic.

## Diagnostics

- `MISSING_NATIVE_FILE`: item `native_path` does not exist under the supplied root.
- `ITEM_SHA256_MISMATCH`: file exists, but its SHA-256 differs from catalog metadata.

## Non-Goals

- No internet fetching.
- No mirror selection.
- No raw KiCad library import.
- No catalog repair or artifact rewriting.
