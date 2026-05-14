# KiCad Reverse-Engineering Feature Map For CCad

Date: 2026-05-14

## Purpose

This document turns KiCad-style expectations into a concrete CCad roadmap. The goal is not to clone KiCad or monkey-patch it. The goal is to understand the workflow surface, data model, UI affordances, and verification stack that a serious PCB tool needs, then rebuild those capabilities around a machine-callable native kernel.

## Source Areas To Study

Primary references:

- KiCad source root: https://gitlab.com/kicad/code/kicad
- `pcbnew` source tree: https://gitlab.com/kicad/code/kicad/-/tree/master/pcbnew
- `eeschema` source tree: https://gitlab.com/kicad/code/kicad/-/tree/master/eeschema
- KiCad tool framework: https://dev-docs.kicad.org/en/components/tool-framework/index.html
- KiCad CLI: https://docs.kicad.org/10.0/en/cli/cli.html
- KiCad IPC API: https://dev-docs.kicad.org/en/apis-and-binding/ipc-api/for-kicad-developers/
- KiCad s-expression file format: https://dev-docs.kicad.org/en/file-formats/sexpr-intro/
- KiCad board file format: https://dev-docs.kicad.org/en/file-formats/sexpr-pcb/
- KiCad PCB editor manual: https://docs.kicad.org/master/en/pcbnew/pcbnew.html
- KiCad Python PCB bindings: https://dev-docs.kicad.org/en/apis-and-binding/pcbnew/index.html

Useful non-KiCad references:

- OpenROAD architecture: https://openroad.readthedocs.io/en/latest/main/README.html
- OpenROAD API docs: https://openroad.readthedocs.io/en/latest/main/src/README.html
- Magic VLSI: http://opencircuitdesign.com/magic/
- gEDA `gnetlist`: https://web.mit.edu/geda/arch/sun4x_59/versions/current/share/doc/geda-doc/man/gnetlist.html
- `gsch2pcb`: https://manpages.debian.org/unstable/geda-utils/gsch2pcb.1.en.html
- pcb-rnd batch UI: https://repo.hu/projects/pcb-rnd/user/05_ui/03_batch/index.html
- tscircuit/Circuit JSON: https://github.com/tscircuit/circuit-json
- SKiDL: https://github.com/devbisme/skidl
- Atopile layout docs: https://docs.atopile.io/atopile/essentials/6-layout
- Freerouting: https://github.com/freerouting/freerouting

## UI Lessons From KiCad Screenshots

KiCad's schematic and PCB editors have a consistent professional CAD layout:

- Menu bar: file/edit/view/place/route/inspect/tools/preferences/help.
- Main toolbar: save, print, undo/redo, zoom, update/sync, DRC/ERC, calculators, console.
- Left tool rail: grid/unit/snap and editing mode toggles.
- Right tool rail: active drawing/editing commands.
- Right dock: layers/objects/nets or design block browser.
- Left dock in schematic: properties, hierarchy, selection filters.
- Central canvas: schematic sheet or PCB board is dominant, not decorative.
- Bottom status bar: selected tool, coordinates, grid, units, counts, distances, current layer.
- Selection filters are first-class because CAD scenes contain many overlapping object types.
- Layer visibility and net/object filtering are first-class because board review depends on slicing dense state.

CCad GUI direction:

- Keep center canvas dominant.
- Keep side docks dense and operational, not marketing-like.
- Use panels for properties, hierarchy, layers, nets, diagnostics, and transactions.
- Use status bar for coordinates, grid, selected object, current command, current revision, and verification state.
- Use modern visual style, but CAD density matters more than decorative whitespace.

## Functionality Map

### Project And Workspace

KiCad-like expectation:

- Project file and project-local settings.
- Multiple sheets.
- Schematic/PCB relationship.
- Library tables.
- Recent files.

CCad target:

- `project init`
- project metadata and settings in typed kernel
- deterministic schema
- transaction log
- project-local libraries and constraints
- `ccad inspect` for machine-readable project status

### Schematic Capture

KiCad-like expectation:

- Symbols, pins, wires, buses, labels, hierarchical sheets.
- Properties panel for selected symbols.
- Annotation and reference assignment.
- ERC.
- Netlist and PCB synchronization.

CCad target:

- logical object model for symbols/components/pins/nets/buses
- semantic commands:
  - `sch add-component`
  - `sch connect`
  - `sch add-label`
  - `sch add-sheet`
  - `sch annotate`
- no separate hidden sync step; schematic and PCB derive from one kernel state
- ERC as core service and CLI/RPC tool

### PCB Layout

KiCad-like expectation:

- Board outline.
- Stackup/layers.
- Footprints, pads, vias, traces, zones, keepouts, graphics, dimensions.
- Selection filters.
- Layer manager.
- Interactive routing.
- DRC.

CCad target:

- physical object model:
  - board outline
  - layers
  - footprints
  - pads
  - vias
  - tracks
  - zones
  - keepouts
  - text/graphics/dimensions
- semantic commands:
  - `pcb set-stackup`
  - `pcb set-outline`
  - `place component`
  - `route net`
  - `zone add`
  - `verify drc`
- GUI canvas must render these objects from kernel state only

### Libraries

KiCad-like expectation:

- Symbol libraries.
- Footprint libraries.
- 3D models.
- Project-local and global library tables.

CCad target:

- read-only KiCad library import first
- internal normalized library schema
- symbol-footprint-pin mapping validation
- later native library editor

### Verification

KiCad-like expectation:

- ERC.
- DRC.
- Unconnected nets.
- Courtyards.
- Clearance.
- Hole/pad constraints.
- Custom design rules.

CCad target:

- rule engine in kernel
- typed diagnostics with object IDs and locations
- `verify erc`
- `verify drc`
- `verify manufacturing`
- diagnostic explanations and suggested fixes
- GUI diagnostic table linked to canvas selection

### Simulation And Calculation

KiCad-like expectation:

- SPICE/ngspice integration.
- Electrical calculators.
- Track width/current.
- Differential pair and impedance helpers.

CCad target:

- SPICE export boundary first
- ngspice subprocess integration later
- calculation commands:
  - `calc trace-width`
  - `calc impedance`
  - `calc divider`
  - `calc thermal`
- simulation outputs as artifacts with provenance

### Routing

KiCad-like expectation:

- Push-and-shove router.
- Differential pairs.
- Length tuning.
- Highlight nets.
- External autorouter path.

CCad target:

- semantic route requests before freeform geometry:
  - `route net --class ...`
  - `route pair --match-length ...`
  - `route bus --skew ...`
- DSN/SES boundary for Freerouting
- internal validation after route import
- later interactive router in GUI

### Manufacturing Outputs

KiCad-like expectation:

- Gerber.
- Drill.
- Pick and place.
- BOM.
- STEP/3D.
- IPC-2581/ODB++ style exports.

CCad target:

- artifact layer in kernel
- deterministic export manifests
- provenance metadata
- first outputs after physical layout primitives exist

## Agent-Native Feature Map

Every GUI workflow needs a command/API equivalent:

| GUI Feature | Agent-Native Surface |
| --- | --- |
| Open project | `ccad inspect <project>` |
| Review changes | `ccad diff <before> <after>` |
| Human status table | `review build` / `inspect` JSON |
| Add schematic object | `sch add-*` transaction |
| Place part | `place component` transaction |
| Route net | `route net` transaction |
| DRC/ERC | `verify erc/drc` JSON diagnostics |
| Export fabrication | `export gerber/drill/bom` manifest |
| GUI selection | object ID and revision ID, never pixel-only |

## Sprint Roadmap From Here

### Sprint 2: Diff And Review

Current sprint.

- project diff
- transaction entry
- CLI inspect/diff
- modern review GUI polish

### Sprint 3: Physical Units And Board Primitives

- units: nm/um/mm/mil conversion
- points, boxes, layers
- board outline
- primitive tracks/pads/vias as data only
- JSON serialization
- DRC skeleton for board outline containment

### Sprint 4: Native Canvas Prototype

- Qt canvas widget
- pan/zoom/grid
- render board outline and primitive objects
- selection by object ID
- no editing yet

### Sprint 5: Schematic Logical Expansion

- symbols and hierarchical sheets
- labels/buses/interfaces
- annotation rules
- richer ERC

### Sprint 6: Library Import Spike

- KiCad symbol/footprint read-only parser or adapter
- normalized CCad library model
- pin-map validation

### Sprint 7: Transaction Apply/Revert

- transaction schema
- apply/revert command
- transaction log file
- GUI transaction timeline

### Sprint 8: Basic Layout Editing

- add/move footprint transaction
- add/remove track transaction
- GUI selection/properties panel
- CLI parity for each operation

## No-Technical-Debt Rules

- No GUI-only behavior.
- No untested kernel behavior.
- No raw string JSON assembly outside shared helpers unless covered by tests.
- No KiCad dependency inside the source-of-truth kernel.
- No GPL code import without explicit license decision.
- No external subprocess without documented trust and file boundary.
- No editing feature before transaction model can record it.
- No renderer feature without object IDs and deterministic scene state.

