# Physical DRC Design

## Goal

Add the first kernel-level physical design-rule checks for board primitives so invalid PCB geometry is reported by deterministic diagnostics, independent of the GUI and CLI mutation path.

## Scope

This sprint adds a `ccad_core` physical DRC module for existing board data:

- pad/via/track duplicate IDs within their primitive type
- pad and track unknown layer references
- pad, via, and track geometry outside the board outline
- non-positive pad sizes, via diameter/drill, and track width
- via drill larger than via diameter
- zero-length track segments

The checks are intentionally geometric and structural only. They do not yet cover clearance, annular ring, solder mask expansion, net connectivity, copper pours, impedance, differential pairs, thermal relief, or manufacturer-specific rule decks.

## Architecture

Create a new `ccad_core::runDrc(const Project&)` function returning the existing `Diagnostic` type. Keep diagnostics machine-readable with stable `code`, `severity`, `object_id`, and `message`.

Add CLI command:

```powershell
.\build-qt\ccad.exe drc board.ccad.json
```

The command prints JSON diagnostics using the same diagnostic JSON shape as `validate`. Exit codes:

- `0`: no DRC errors
- `1`: one or more DRC errors
- `2`: usage, file, or parse failure

## Relationship To ERC

`validate` remains logical ERC only for now. DRC gets its own command so callers can choose exact gates. A later signoff command can run ERC + DRC + manufacturing checks together.

## Test Strategy

- Add `tests/test_drc.cpp` for kernel DRC behavior.
- Add CLI test coverage for `ccad drc`.
- Keep tests deterministic and black-box enough that future DRC internals can change without breaking caller contracts.

