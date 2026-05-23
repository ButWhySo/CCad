# Sprint 43: Clearance DRC

## Progress

Progress: Phase 2/6, Sprint 43, `main`, merged and verified after merge.

## Goal

Sprint 43 adds the first kernel-level copper clearance check. This stays inside Phase 2 because it strengthens physical primitive verification without jumping ahead into routing, placement automation, or full interactive editing.

## Implemented Scope

The DRC now checks a fixed default 0.20 mm copper clearance between different non-empty nets. Same-net copper is allowed to touch. Empty-net objects still produce the existing unconnected warnings instead of clearance errors.

The first clearance pass covers pad-pad, track-track, pad-track, via-via, via-pad, and via-track pairs. Pads use rotated rectangular geometry, tracks use segment geometry plus width, and vias use circular diameter geometry.

Layer handling is intentionally conservative. Pads and tracks must share the same copper layer before they are compared. Vias are treated as exposed across copper layers.

The new diagnostic code is `COPPER_CLEARANCE`. It is emitted as an error on the later checked object ID and names the nearby object in the diagnostic message.

## Verification Evidence

Focused RED result before implementation:

```text
test failure: drc reports different-net pads closer than default clearance
```

Focused GREEN command:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --target ccad_drc_tests && build-qt\ccad_drc_tests.exe"
```

Focused GREEN result:

```text
exit code 0
```

## Remaining Gate

Full clean build and CTest passed:

```cmd
cmd /c "set PATH=C:\Qt\6.11.1\mingw_64\bin;%PATH% && cmake --build build-qt --clean-first && ctest --test-dir build-qt --output-on-failure"
```

Result:

```text
100% tests passed, 0 tests failed out of 11
```

Sprint demo passed:

```cmd
cmd /c "powershell -ExecutionPolicy Bypass -File .\scripts\run_sprint_demo.ps1 -Name sprint43-clearance-drc"
```

Demo artifacts:

```text
artifacts/demos/sprint43-clearance-drc.ccad.json
artifacts/demos/sprint43-clearance-drc.inspect.json
artifacts/demos/sprint43-clearance-drc.validate.json
artifacts/demos/sprint43-clearance-drc.drc.json
artifacts/screenshots/sprint43-clearance-drc-20260523-232959.png
```

Post-gate demo artifacts:

```text
artifacts/demos/sprint43-clearance-drc-verified.ccad.json
artifacts/demos/sprint43-clearance-drc-verified.inspect.json
artifacts/demos/sprint43-clearance-drc-verified.validate.json
artifacts/demos/sprint43-clearance-drc-verified.drc.json
artifacts/screenshots/sprint43-clearance-drc-verified-20260523-233349.png
```

The demo DRC JSON contains:

```text
"code": "COPPER_CLEARANCE"
"object_id": "T2"
```

Process cleanup check passed:

```cmd
cmd /c "tasklist /FI ""IMAGENAME eq ccad_gui.exe"""
```

Expected result:

```text
INFO: No tasks are running which match the specified criteria.
```
