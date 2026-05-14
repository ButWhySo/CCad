# Sprint 1: Qt Review GUI Foundation

## Sprint Goal

Give humans a native desktop review surface for CCad project files while keeping the kernel and CLI as source of truth.

## Branch

`feature-qt-review-gui`

## Backlog

1. Add GUI spec and implementation plan.
2. Add tested `ccad_core` project review view-model.
3. Add optional Qt 6 Widgets GUI target.
4. Update handover and build documentation.
5. Run full verification and merge to `main`.

## Definition Of Done

- CMake configure succeeds with `CCAD_WARNINGS_AS_ERRORS=ON`.
- Full build succeeds.
- CTest passes.
- Qt GUI target is optional and does not break non-Qt environments.
- Docs explain how to build and what the GUI can and cannot do.
- Branch is merged to `main` with a merge commit.

## Risks

- Qt may not be installed on local or CI machines. Mitigation: build `ccad_gui` only when Qt 6 Widgets is found.
- GUI logic can drift from CLI behavior. Mitigation: keep review logic in `ccad_core` and test it with CTest.
- Review GUI might be mistaken for editor. Mitigation: no editing controls until transactions exist.

## DevOps Notes

- No remote exists yet, so GitHub Actions is configured but not executed remotely.
- Local CI equivalent is:

```bash
cmake -S . -B build -DCCAD_WARNINGS_AS_ERRORS=ON
cmake --build build --clean-first
ctest --test-dir build --output-on-failure
```

