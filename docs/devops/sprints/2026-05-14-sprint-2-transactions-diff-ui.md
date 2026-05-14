# Sprint 2: Transactions, Diff, And Review UI Polish

## Sprint Goal

Add the first transaction/diff foundation for agent-safe work, expose project inspection/diff through the CLI, and make the Qt review GUI feel like a modern review tool instead of a bare debug window.

## Branch

`sprint-2-transactions-diff`

## Backlog

1. Add tested project diff model in `ccad_core`.
2. Add tested transaction journal entry model in `ccad_core`.
3. Add `ccad inspect` and `ccad diff` CLI commands with JSON output.
4. Polish Qt review GUI layout and styling:
   - modern spacing and typography
   - summary cards/chips
   - colored diagnostic severity
   - clearer empty state
   - better table sizing
5. Update feature docs and handover.
6. Run Qt and core verification, then merge to `main`.

## Definition Of Done

- Full CMake build succeeds with Qt enabled on local Windows Qt kit.
- CTest passes.
- `ccad inspect <project>` prints project summary JSON.
- `ccad diff <before> <after>` prints machine-readable diff JSON.
- GUI remains native Qt Widgets and uses `ccad_core`; no business logic is hidden in GUI.
- GUI looks suitable for human review: not a bare 2008-style default table-only window.
- Feature guide documents how to use and test all new features.

## Local CI

```powershell
cmake -S . -B build-qt -DCCAD_WARNINGS_AS_ERRORS=ON -DCCAD_BUILD_GUI=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.11.1\mingw_64
cmake --build build-qt --clean-first
ctest --test-dir build-qt --output-on-failure
```

