# Project Progress Counter

This file is the canonical phase/sprint counter for CCad. Update it after every sprint commit sequence and every merge.

## Current Position

- Phase: 2 / 6
- Phase name: Physical primitives and early board authoring
- Sprint: 9
- Branch: `sprint-9-footprint-placement`
- Last merged sprint: Sprint 8, KiCad footprint import
- Status: Sprint 9 planning

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

## Active Sprint

- Sprint 9: footprint placement from imported library data.

## Reporting Rule

Every commit/merge status update should include:

```text
Progress: Phase 2/6, Sprint 9, <branch-or-main>, <short status>
```

Example:

```text
Progress: Phase 2/6, Sprint 9, main, merged and verified.
```
