# Project Progress Counter

This file is the canonical phase/sprint counter for CCad. Update it after every sprint commit sequence and every merge.

## Current Position

- Phase: 2 / 6
- Phase name: Physical primitives and early board authoring
- Sprint: 24
- Branch: `main`
- Last merged sprint: Sprint 27, GUI CAD editor shell foundation
- Status: Sprint 27 merged and verified on `main`; Sprint 28 planning next

## Phase Roadmap

1. Logical kernel: project, components, pins, nets, constraints, ERC, CLI.
2. Physical primitives: board outline, layers, pads, vias, tracks, keepouts, placement regions, early DRC.
3. Routing assistance: constrained route requests and external router boundary.
4. Native GUI/editor: schematic and PCB review/edit surfaces, diffs, diagnostics, and transaction review.
5. Interop: KiCad, Circuit JSON, DSN/SES, SPICE/simulation hooks, manufacturing exports.
6. Agent protocol: JSON-RPC/MCP over the transaction bus, audit logs, permission gates, benchmark harness.

## Completed Sprints

- Sprint 1: Qt review GUI foundation.
- Sprint 2: transactions, diff, and review UI polish.
- Sprint 3: physical units and board outline/layers.
- Sprint 4: native board canvas prototype.
- Sprint 5: PCB drawable primitives in kernel, JSON, canvas, and GUI.
- Sprint 6: CLI PCB primitive authoring.
- Sprint 7: physical DRC diagnostics and `ccad drc`.
- Sprint 8: KiCad footprint import groundwork.
- Sprint 9: footprint placement from imported library data.
- Sprint 10: footprint rotation and GUI module split.
- Sprint 11: CLI module split.
- Sprint 12: machine-readable CLI help.
- Sprint 13: footprint net mapping.
- Sprint 14: DRC unconnected pad warning.
- Sprint 15: DRC unknown physical net references.
- Sprint 16: DRC track endpoint connectivity.
- Sprint 17: DRC empty via/track net warnings.
- Sprint 18: rectangular keepouts.
- Sprint 19: keepout authoring and visibility.
- Sprint 20: track crossing keepout DRC.
- Sprint 21: native library catalog foundation.
- Sprint 22: library catalog CLI.
- Sprint 23: library catalog search.
- Sprint 24: library catalog kind filter.
- Sprint 25: library catalog validation.
- Sprint 26: library catalog file checks.
- Sprint 27: GUI CAD editor shell foundation.

## Active Sprint

- Sprint 28: GUI module split and selection groundwork.

## Reporting Rule

Every commit/merge status update should include:

```text
Progress: Phase 2/6, Sprint 25, <branch-or-main>, <short status>
```

Example:

```text
Progress: Phase 2/6, Sprint 27, main, merged and verified.
```
