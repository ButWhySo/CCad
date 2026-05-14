# LLM-Native PCB Tooling Research Report

Date: 2026-05-14

## Executive Position

The new command-line trend is not nostalgia for terminals. It is a shift toward machine-callable software. Modern agents need stable commands, typed arguments, structured results, replayable state, and executable verification. A GUI can still exist, but it should become a renderer and reviewer over a deterministic kernel, not the only operational surface.

For PCB design, this matters more than in ordinary office software. GUI-driven PCB editing depends on selection state, mode state, geometry, layer context, and object identity. A language model driving KiCad through screenshots or cursor coordinates can reason correctly and still make a mechanical error: wrong pad, wrong net, wrong layer, wrong dialog option, or a missed schematic-layout sync. That failure class is inherent to screenshot-first control. The project direction should therefore be a ground-up native PCB kernel with CLI/RPC/MCP surfaces and a native GUI client, not a KiCad hot patch.

## Why Command-Line Tools Matter To LLMs

Language models are strongest when they can call tools with crisp contracts. ReAct frames agent work as interleaved reasoning and action against an environment, and Toolformer shows the broader pattern of models learning when and how to call external APIs. MCP formalizes this same direction: tools expose named capabilities with schemas and structured results. OpenAI's function calling and Structured Outputs documentation points to the same production lesson: schema-constrained calls are more reliable than prose-only interaction.

Terminal-Bench 2.0 reinforces the point from a benchmark angle. It evaluates agents in command-line environments with human-written solutions and executable tests. The important lesson for CCad is not that CLIs are easy; it is that CLIs make work reproducible, auditable, and testable in a way pixel-driven GUI operation rarely does.

Sources:

- ReAct paper: https://arxiv.org/abs/2210.03629
- Toolformer paper: https://arxiv.org/abs/2302.04761
- MCP tools spec: https://modelcontextprotocol.io/specification/2025-06-18/server/tools
- OpenAI function calling: https://help.openai.com/en/articles/8555517-function-calling-in-the-openai-api
- OpenAI Structured Outputs: https://openai.com/index/introducing-structured-outputs-in-the-api/
- Terminal-Bench 2.0: https://arxiv.org/abs/2601.11868

## Why GUI Control Is Structurally Weak For PCB Agents

Computer-use agents have improved, but the control surface remains expensive and fragile. OpenAI reported 38.1% on OSWorld for its Computer-Using Agent against a 72.4% human baseline at that time. Anthropic's computer-use documentation describes screenshot and mouse/keyboard actions as the interface. This means the agent pays perception cost before it can act, then pays again when it must verify what happened.

PCB work compounds this weakness:

- A trace, pad, via, footprint, zone, and net are semantic objects, but screenshots expose pixels.
- The same click can mean different things depending on active tool, layer, selection filter, grid, snap, and modal dialog state.
- A schematic-layout mismatch can survive several later edits before surfacing.
- Visual similarity between pins or pads makes coordinate-level control hazardous.
- A board may look right while being electrically wrong.

The right mitigation is not better screenshots. The right mitigation is to remove screenshot control from the critical path.

Sources:

- OpenAI Computer-Using Agent: https://openai.com/index/computer-using-agent/
- Anthropic computer-use tool docs: https://docs.anthropic.com/en/docs/agents-and-tools/tool-use/computer-use-tool
- OpenAI vision limitations and structured tool guidance should be treated as a design warning for geometry-sensitive workflows: https://platform.openai.com/docs/guides/function-calling

## KiCad Analysis

KiCad is an excellent human-led EDA suite and should be treated with respect. It is also not the right control plane for CCad.

What KiCad already gives:

- `kicad-cli` can automate verification and export: schematic ERC, PCB DRC, Gerber, drill, STEP, SVG, PDF, IPC-2581, ODB++, netlists, BOM, and other outputs.
- KiCad files are documented s-expressions, human-readable, UTF-8, with coordinates in millimeters and stable UUID-style object identifiers.
- KiCad's IPC API is protobuf-based and communicates over NNG sockets.

Why KiCad is still awkward for an LLM-native tool:

- The documented CLI is primarily validation/export/import/upgrade, not a complete typed authoring API for every schematic and layout operation.
- The IPC API is attached to a running KiCad instance and routes work through the GUI application's event model. KiCad docs say request processing crosses to the main UI thread.
- The file format is low-level geometry. Directly editing it asks the model to manage coordinates, UUIDs, layers, and consistency relationships rather than intent.
- KiCad's PCB editor workflow includes cross-probing and explicit update-from-schematic actions. That is normal for a GUI product, but it is not a transaction-first machine ontology.

Conclusion: KiCad should be an interoperability and verification target, not CCad's internal truth model.

Sources:

- KiCad CLI docs: https://docs.kicad.org/10.0/en/cli/cli.html
- KiCad IPC API docs: https://dev-docs.kicad.org/en/apis-and-binding/ipc-api/for-kicad-developers/
- KiCad s-expression format: https://dev-docs.kicad.org/en/file-formats/sexpr-intro/
- KiCad PCB editor docs: https://docs.kicad.org/master/en/pcbnew/pcbnew.html

## Prior Art Worth Learning From

### Legacy PCB And gEDA Flow

The old `pcb` tool lineage is important because it treated PCB files and netlists as textual, processable artifacts. Thomas Nau's `pcb` dates to 1990 and was ported to UNIX/X11 in 1994. It used ASCII layout and element files, command-line launch options, and command/action concepts. gEDA split schematic capture, netlisting, and PCB update into explicit stages. `gnetlist` emitted many netlist formats, and `gsch2pcb` generated or updated board files from schematics.

Lesson for CCad: old EDA already understood pipeline boundaries. Bring that back, but with typed objects, transactions, deterministic JSON/protobuf, and modern verification.

Sources:

- PCB history/docs: https://pcb.sourceforge.net/pcb-cvs/pcb.html
- gEDA `gnetlist`: https://web.mit.edu/geda/arch/sun4x_59/versions/current/share/doc/geda-doc/man/gnetlist.html
- `gsch2pcb` manpage: https://manpages.debian.org/unstable/geda-utils/gsch2pcb.1.en.html

### pcb-rnd / Ringdove

`pcb-rnd` is a useful reference for scriptable and batch-capable PCB editing. It has GUI and batch modes, scripting, multiple file formats, and a broader Ringdove ecosystem. Licensing must be reviewed before code reuse, but the architecture is relevant.

Sources:

- pcb-rnd batch UI: https://repo.hu/projects/pcb-rnd/user/05_ui/03_batch/index.html
- pcb-rnd overview: https://en.wikipedia.org/wiki/Pcb-rnd

### OpenROAD And Magic

OpenROAD is the best modern EDA architecture analogy: one process, one database, command-driven flow, Tcl/Python APIs, and a GUI as a visualization/debugging client. Magic is older but relevant because it made layout tool behavior command-native and scriptable through Tcl/Tk.

Lesson for CCad: use one authoritative native database and expose commands over CLI/RPC. The GUI should consume the same command bus.

Sources:

- OpenROAD docs: https://openroad.readthedocs.io/en/latest/main/README.html
- OpenROAD API docs: https://openroad.readthedocs.io/en/latest/main/src/README.html
- Magic overview: http://opencircuitdesign.com/magic/

## Reuse Candidates And Compatibility Strategy

Do not reinvent proven subsystems unless license, architecture, or correctness demands it.

### Strong candidates for ideas or adapters

- `tscircuit` and Circuit JSON: good reference for programmatic electronics, machine-readable schematic/PCB/fabrication representation, warnings, previews, and AI-adjacent workflows. Its TypeScript/React stack should not define CCad's native core, but Circuit JSON is worth supporting as import/export.
- SKiDL: strong Python-based logical circuit capture, ERC, netlist generation, hierarchy, reuse, and text-first design ergonomics. Good for logical-layer inspiration and possible Python bridge.
- Atopile: useful declarative electronics language and validation pipeline. It still uses KiCad for layout, so it is not CCad's kernel, but its DSL and constraints are relevant.
- Freerouting: useful as an optional off-process routing backend through DSN/SES. Keep GPL/licensing boundaries clear.
- KiCad: useful for import/export compatibility, libraries, file-format study, and external validation/export. Do not make it the control plane.

Sources:

- tscircuit GitHub: https://github.com/tscircuit
- Circuit JSON repo: https://github.com/tscircuit/circuit-json
- SKiDL GitHub: https://github.com/devbisme/skidl
- SKiDL docs: https://devbisme.github.io/skidl/
- Atopile layout docs: https://docs.atopile.io/atopile/essentials/6-layout
- Freerouting GitHub releases: https://github.com/freerouting/freerouting/releases

### Reuse boundary rules

- MIT/permissive code can be considered for direct reuse after license review.
- GPL tools should be used as subprocesses or optional integrations unless CCad intentionally adopts GPL compatibility.
- External tools must have explicit input/output contracts and reproducible command wrappers.
- All imported data must be normalized into CCad's own typed kernel objects.

## Required GUI-To-CLI Feature Mapping

The CLI/RPC API must expose the same capability classes a human expects from a GUI, but as semantic operations:

| Human GUI Action | LLM-Native Command Shape |
| --- | --- |
| Create project | `project init --id ... --stackup ...` |
| Add symbol/component | `sch add-component --ref U1 --part ...` |
| Connect pins | `sch connect U1.VDD net:3V3` |
| Assign footprint | `lib assign-footprint U1 Package_QFN:QFN-48` |
| Update PCB from schematic | automatic transaction over one database, no separate GUI sync |
| Place footprint | `place component U1 --region MCU --policy compact` |
| Place functional block | `place cluster --members U1,C1,C2 --objective minimize_loop_area` |
| Route net | `route net USB_D+ --class usb_diff --pair USB_D-` |
| Review diff | `txn diff --from revA --to revB --json` |
| DRC/ERC | `verify erc`, `verify drc`, `verify manufacturing` |
| Export | `export gerber`, `export step`, `export bom` |
| Explain issue | `diagnose explain DRC_CLEARANCE_17 --json` |

The command contract should be stable:

- Commands accept typed IDs, not screen coordinates.
- Mutating commands return transaction IDs.
- Every transaction is reversible or superseded by a compensating transaction.
- Every command returns structured JSON/protobuf with diagnostics.
- Commands are deterministic by default, with seed/version metadata for heuristics.
- GUI and CLI use the same kernel APIs.

## Proposed CCad Architecture

### Layer 1: Requirements

Captures intent before geometry:

- Board size, stackup, manufacturer limits.
- Voltage/current domains.
- Impedance and differential-pair targets.
- Thermal rules.
- Sourcing and lifecycle constraints.
- Assembly limits.

### Layer 2: Logical Circuit

Owns schematic-level truth:

- Components, symbols, pins, gates, units.
- Nets, buses, interfaces.
- Hierarchy and reusable modules.
- Part selection constraints.
- ERC and logical equivalence checks.

### Layer 3: Physical Layout

Owns board realization:

- Board outline and stackup.
- Footprints, pads, vias, traces, zones.
- Placement regions, keepouts, courtyards.
- Net classes, layer rules, return-path policies.
- DRC and manufacturing constraints.

### Layer 4: Artifacts

Owns outputs:

- Gerber/X2/X3 and drill.
- STEP/VRML/glTF for mechanical review.
- BOM and pick-and-place.
- IPC-2581/ODB++ if implemented.
- Reports, signoff bundles, replay logs.

### Kernel And API

The C++ kernel should remain the source of truth:

- `ccad_core`: typed object model, validation, transactions, serialization.
- `ccad_cli`: command-line client for humans, CI, and agents.
- Future `ccad_rpc`: JSON-RPC and MCP server over local transport.
- Future `ccad_gui`: native desktop viewer/reviewer over the same command bus.

The GUI should show state, diffs, heatmaps, violations, and previews. It should not contain hidden design-authoring logic that the CLI cannot reach.

## Implementation Roadmap

### Phase 1: Logical Kernel

Status: started in current repo.

- Project, component, pin, net, constraint model.
- Deterministic JSON.
- ERC diagnostics.
- Native CLI.
- CMake/CTest CI.

### Phase 2: Transactions And Schema

- Stable semantic IDs.
- Revision log and transaction replay.
- JSON schema or protobuf schema.
- `txn apply`, `txn revert`, `txn diff`.
- Golden replay tests.

### Phase 3: Library And Parts

- Symbol/footprint metadata model.
- KiCad library import read-only adapter.
- Part variants and sourcing metadata.
- Package/pin mapping checks.

### Phase 4: Physical Layout Primitives

- Units and geometry kernel.
- Board outline, layers, pads, vias, tracks, arcs, zones.
- Spatial index.
- Basic DRC: clearance, width, hole, courtyard, outline containment.

### Phase 5: Placement

- Regions and functional clusters.
- Constraint-driven placement commands.
- Deterministic heuristics with explicit seeds.
- Native GUI preview.

### Phase 6: Routing Boundary

- Manual semantic route operations.
- DSN/SES export/import.
- Optional Freerouting subprocess integration.
- Internal route validation after import.

### Phase 7: Interop And Fabrication

- KiCad import/export.
- Circuit JSON import/export.
- Gerber/drill/BOM/PnP.
- STEP or glTF preview path.

### Phase 8: Agent Protocol

- Local JSON-RPC.
- MCP tools for project, schematic, placement, routing, verification, export.
- Permission model for destructive/export/network operations.
- Audit log of all tool calls and file writes.

## Testing And Evaluation

CCad needs tests that resemble Terminal-Bench more than demos:

- Requirement-to-schematic tasks.
- Schematic-to-layout tasks.
- Placement constraint tasks.
- Routing import/export tasks.
- ERC/DRC repair tasks.
- KiCad/Circuit JSON round-trip tasks.
- Deterministic replay tasks.
- Human-in-the-loop correction tasks.

Metrics:

- ERC errors: must reach zero for clean designs.
- DRC errors: must reach zero for signoff designs.
- Schematic-layout parity: no unresolved mismatch.
- Determinism: same seed/version produces same object graph.
- Agent efficiency: fewer tool calls/tokens/time than GUI screenshot operation.
- Repair quality: issue count decreases monotonically after fix transactions.
- Interop: import/export round trips preserve connectivity and geometry within tolerance.
- Safety: no project file can trigger code execution.

## Security Requirements

LLM-native tooling increases automation risk. Treat every external file and model instruction as untrusted.

- Project files are data only.
- No shell execution from project data.
- MCP/RPC tools must have least-privilege capability scopes.
- Mutating commands should write transactions and audit logs.
- Destructive commands need explicit confirmation or policy gate.
- External routers/importers run in subprocess boundaries with sanitized paths.
- CI must run without secrets.
- Generated fabrication outputs should include provenance metadata.

## Decision For CCad

CCad should remain native-first and kernel-first:

- Keep C++ core.
- Keep CLI as first machine surface.
- Add RPC/MCP later, not before the transaction model is sound.
- Add native GUI as a reviewer/renderer after physical primitives exist.
- Reuse proven tools through adapters and file boundaries.
- Do not wrap KiCad as the core workflow.

The product bet is not "CLI instead of GUI." The product bet is "typed kernel first, machine API and GUI as equal clients." That is the only architecture that removes GUI mechanical error from the agent's critical path while still giving humans visual inspection and control.

