# Project Progress Counter

This file is the canonical phase/sprint counter for CCad. Update it after every sprint commit sequence and every merge.

## Current Position

- Phase: 2 / 6
- Phase name: Physical primitives and early board authoring
- Sprint: 43
- Branch: `sprint-43-clearance-drc`
- Last merged sprint: Sprint 42, GUI canvas MVP review
- Status: Sprint 43 verified on feature branch; commit and merge pending

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
- Sprint 28: GUI module split.
- Sprint 29: GUI selection groundwork.
- Sprint 30: GUI inspector panel.
- Sprint 31: layer and object browser.
- Sprint 32: diagnostic selection link.
- Sprint 33: review DRC diagnostics.
- Sprint 34: diagnostic overlay markers.
- Sprint 35: object browser selection link.
- Sprint 36: canvas render theme groundwork.
- Sprint 37: transaction timeline panel.
- Sprint 38: net highlight groundwork.
- Sprint 39: net browser highlight.
- Sprint 40: component knowledge schema.
- Sprint 41: catalog knowledge CLI visibility.
- Sprint 42: GUI canvas MVP review and app-owned screenshot harness.

## Active Sprint

- Sprint 43: clearance DRC. Use moderate sprint sizing: several related tasks per compile, not one tiny branch per small GUI affordance.

## Reporting Rule

Every commit/merge status update should include:

```text
Progress: Phase 2/6, Sprint 25, <branch-or-main>, <short status>
```

Example:

```text
Progress: Phase 2/6, Sprint 29, main, merged and verified.
```
