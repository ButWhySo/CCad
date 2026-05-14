# KiCad Footprint Import Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow `.kicad_mod` footprint importer and CLI export so CCad can reuse KiCad footprint library data as external assets.

**Architecture:** Add a `Footprint` library model in `ccad_core`, parse KiCad s-expressions into that model, and serialize imported footprints to deterministic JSON. Keep footprint library data separate from placed board primitives.

**Tech Stack:** C++20, CMake, CTest.

---

## File Map

- Create `src/ccad_core/footprint.hpp`: footprint model structs.
- Create `src/ccad_core/kicad_footprint_import.hpp`: importer declaration.
- Create `src/ccad_core/kicad_footprint_import.cpp`: scoped s-expression parser/importer and footprint JSON dumper.
- Create `tests/test_kicad_footprint_import.cpp`: importer tests.
- Modify `CMakeLists.txt`: add importer source and test target.
- Modify `src/ccad_cli/main.cpp`: add `ccad lib import-footprint`.
- Modify `tests/test_cli.cpp`: CLI import test.
- Modify docs: README, feature inventory, sprint log, progress counter.

## Task 1: Importer Unit Tests

- [ ] Create `tests/test_kicad_footprint_import.cpp`.
- [ ] Write inline sample footprint:

```lisp
(footprint "R_0805_2012Metric"
  (version 20240101)
  (generator "ccad-test")
  (pad "1" smd roundrect (at -0.95 0 0) (size 1.0 1.45) (layers "F.Cu" "F.Paste" "F.Mask"))
  (pad "2" smd roundrect (at 0.95 0 0) (size 1.0 1.45) (layers "F.Cu" "F.Paste" "F.Mask"))
)
```

- [ ] Assert importer returns footprint name, two pads, pad numbers, type/shape, at, size, and layers.
- [ ] Assert `dumpFootprintJson(imported)` contains deterministic name and `width_nm`.
- [ ] Add malformed/root mismatch rejection tests.
- [ ] Run importer test target and confirm it fails before implementation.

## Task 2: Core Importer

- [ ] Implement footprint structs:

```cpp
struct FootprintPad {
  std::string number;
  std::string type;
  std::string shape;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
  std::optional<Length> drill;
  std::vector<std::string> layers;
};

struct Footprint {
  std::string name;
  std::vector<FootprintPad> pads;
};
```

- [ ] Implement `importKiCadFootprint(std::string_view source)`.
- [ ] Implement `dumpFootprintJson(const Footprint& footprint)`.
- [ ] Add CMake target `ccad_kicad_footprint_import_tests`.
- [ ] Run full Qt gate.
- [ ] Commit:

```powershell
git add CMakeLists.txt src\ccad_core\footprint.hpp src\ccad_core\kicad_footprint_import.hpp src\ccad_core\kicad_footprint_import.cpp tests\test_kicad_footprint_import.cpp
git commit -m "feat: add kicad footprint importer"
```

## Task 3: CLI Import Command

- [ ] Add CLI test for:

```powershell
.\build-qt\ccad.exe lib import-footprint --in sample.kicad_mod --out sample.ccad-footprint.json
```

- [ ] Implement `libCommand` with `import-footprint` subcommand.
- [ ] Return exit code `0` on success and `2` for usage/file/parse failure.
- [ ] Run CLI tests and full Qt gate.
- [ ] Commit:

```powershell
git add src\ccad_cli\main.cpp tests\test_cli.cpp
git commit -m "feat: add cli kicad footprint import"
```

## Task 4: Docs, Demo, Merge

- [ ] Update README with command usage and KiCad reuse policy.
- [ ] Update `docs/features/implemented-features.md`.
- [ ] Update `docs/devops/progress.md` to Sprint 8.
- [ ] Add sprint verification log.
- [ ] Extend demo script or add a small generated demo artifact for imported footprint JSON.
- [ ] Run full Qt gate.
- [ ] Commit docs.
- [ ] Merge to `main`, run full Qt gate again.

