# Large Design And Component Knowledge Pipeline

This file captures the architecture direction for large PCB projects and component knowledge. It exists because huge boards cannot be created safely by sending thousands of tiny CLI commands.

## Problem

A large schematic or PCB may have thousands of objects. If an agent creates or edits that design one resistor, pad, trace, or coordinate at a time, the workflow becomes slow, expensive, hard to review, and easy to break.

The LLM should not be responsible for every exact coordinate. The LLM should work mostly with intent, modules, constraints, reports, and exceptions. The kernel and local tools should do the mechanical geometry work.

## Main Rule

LLM calls should be expensive and semantic. Kernel loops should be cheap and mechanical.

That means:

- The LLM gives high-level intent.
- CCad performs many local operations internally.
- CCad returns a compact report, diagnostics, and a transaction diff.
- The LLM reviews only exceptions and important decisions.
- GUI screenshots are used at checkpoints, not after every object.

## Large Design Authoring Model

Primitive commands such as `pcb add-pad` and `pcb add-track` are useful for testing and small edits. They are not the final way to build large designs.

Large designs should use:

- reusable modules
- structured design files
- constraints files
- placement policies
- route policies
- batch transactions
- compact validation reports
- visual review checkpoints

Example intent:

```text
create USB-C power input block
create 3.3V buck regulator block
connect MCU minimum system
place power cluster
route obvious short nets
run ERC and DRC
show the transaction diff
```

The result should be one meaningful transaction, not hundreds of noisy commands.

## Import And Modify Existing Large Designs

Existing KiCad or other EDA projects should be imported as whole designs.

The import pipeline should:

1. Read the source project as data.
2. Preserve raw source files and provenance.
3. Convert schematic, PCB, symbols, footprints, and library links into CCad internal representation.
4. Normalize IDs, nets, layers, units, and object references.
5. Validate imported state with ERC, DRC, library checks, and schematic-layout parity checks.
6. Produce a compact import report.
7. Open the GUI for human and agent visual review.

Large modifications should happen as named batches:

```text
transaction begin "replace USB connector and reroute pair"
apply semantic edit
run local checks
show diff
human or agent approves
transaction commit
```

## Component Data Layers

Footprint geometry alone is not enough. A useful agent needs part knowledge.

CCad should preserve and build three layers of component data.

### 1. Raw Imported Library Data

Preserve as much upstream data as possible:

- symbol
- footprint
- 3D model link
- pin names and pin numbers
- fields and properties
- manufacturer part number
- value
- package
- keywords
- datasheet URL
- license
- provenance
- checksum

Raw data should stay traceable back to source libraries such as KiCad official libraries.

### 2. Normalized CCad Component Catalog

Convert raw records into a searchable CCad format:

- electrical type
- ratings
- package and assembly data
- pin roles
- supply requirements
- interface type
- recommended decoupling
- layout warnings
- compatible footprints
- sourcing info
- lifecycle and risk tags
- datasheet and document links
- provenance and checksum

Agents should search this local normalized catalog first.

### 3. Agent-Ready Knowledge

Some facts should be preprocessed into compact summaries. These summaries help the agent choose and use parts correctly without reading a datasheet every time.

Examples:

- how to use the part
- important limits
- required external components
- common circuit patterns
- layout notes
- forbidden mistakes
- pin role summary
- suggested constraints
- source references and confidence level

Example shape:

```json
{
  "id": "part:example:buck-regulator",
  "usage_summary": "3.3V buck regulator. Needs input cap close to VIN/GND, inductor on SW, output cap near VOUT/GND.",
  "layout_notes": [
    "Minimize VIN-SW-GND hot loop",
    "Keep SW copper small",
    "Place feedback divider away from SW node"
  ],
  "required_passives": ["input_cap", "inductor", "output_cap", "feedback_divider"],
  "source_confidence": "datasheet"
}
```

## Human And AI Expert Labeling

Not every useful component fact can be imported perfectly. Some knowledge must be labeled or reviewed.

CCad should support a curation workflow where humans and AI experts can add, review, and approve:

- pin role labels
- usage summaries
- layout warnings
- common application circuits
- required external parts
- electrical limits
- sourcing risk notes
- package difficulty notes
- links to datasheets, app notes, and reference designs
- confidence levels

Every curated field should record:

- who or what produced it
- source document or source URL
- date
- confidence level
- review status

Review status should be simple:

- `generated`
- `needs_review`
- `reviewed`
- `rejected`

The agent may use generated data, but high-risk design decisions should prefer reviewed data.

## Advanced Search

Component search should use many fields, not only names.

Useful searches:

```text
low power 3.3V LDO
VIN >= 12V and IOUT >= 500mA
USB-C connector with shield pins
buck converter with easy layout
SOT-23 ESD diode for USB
MCU with SPI, I2C, USB, and 64KB flash
```

Search should span:

- normalized fields
- pin roles
- ratings
- package
- usage summaries
- layout notes
- datasheet-derived facts
- provenance
- local document cache

Internet search should be a controlled catalog update path, not a normal design-time dependency.

## Text And Image To Circuit

Long term, CCad should be able to turn a datasheet application circuit, block diagram, photo, or text description into a candidate schematic.

The safe flow is:

1. Extract text or image content.
2. Identify likely topology.
3. Identify candidate components.
4. Map pins and nets.
5. Cite sources and confidence.
6. Generate a candidate schematic module.
7. Run ERC and module checks.
8. Ask for human review when confidence is low.

This should not silently create production-ready circuits. It should create candidates with evidence.

## Broad Prompt To Finished Electronics Flow

CCad should eventually handle broad product prompts, not only narrow PCB edits.

Examples:

```text
create a Class D amplifier
create a TV set-top box
create an HDMI display controller
create an STM32-based FPV drone with edge AI
```

These prompts are too broad to become one direct PCB command. They need an end-to-end engineering pipeline.

The flow should be:

1. Turn the prompt into requirements.
2. Ask clarifying questions only where the answer changes the design.
3. Decompose the product into subsystems.
4. Choose candidate architectures.
5. Choose components from the enriched local catalog.
6. Generate schematic modules.
7. Generate constraints for power, signals, mechanics, thermal, radio, and manufacturing.
8. Place modules and clusters.
9. Route using policies and internal solver loops.
10. Run ERC, DRC, sourcing checks, thermal checks, signal checks, and manufacturing checks.
11. Produce compact reports and transaction diffs.
12. Use GUI visual checkpoints for human and agent review.
13. Iterate only on failed checks or weak-confidence decisions.
14. Export manufacturing and handover artifacts.

The agent should not pretend one broad prompt is enough information for a finished board. It should create an evidence-backed design plan, expose assumptions, and then build in verified batches.

## Product-Level Decomposition

Broad prompts should be converted into a product tree.

Example for an STM32 FPV drone with edge AI:

```text
product
  power
    battery input
    regulators
    current sensing
  compute
    STM32 flight controller
    edge AI module
    memory
  sensing
    IMU
    barometer
    camera input
  communications
    RC receiver
    telemetry
    video link
  motor control
    ESC interfaces
    current paths
  programming and debug
    SWD
    boot controls
  mechanical
    mounting holes
    connector placement
  manufacturing
    layer count
    assembly constraints
```

Each subsystem should become one or more schematic modules with explicit interfaces, constraints, risks, and review status.

## Architecture Choice Records

For broad prompts, CCad should record why major choices were made.

Examples:

- why this MCU was selected
- why this regulator topology was selected
- why a connector was placed on one edge
- why a differential pair layer was chosen
- why a part was rejected

These records help humans review the design and help future agents avoid repeating bad searches.

## Confidence And Human Review Gates

End-to-end generation must include confidence levels.

High-confidence work can proceed automatically after validation. Low-confidence work must pause for human review.

Human review gates should appear for:

- unclear requirements
- safety-critical power decisions
- RF or high-speed interfaces
- thermal assumptions
- sourcing substitutions
- datasheet facts not yet reviewed
- generated circuits based on image or text extraction
- any rule violation that the agent proposes to waive

The goal is not blind automation. The goal is fast generation with visible evidence and controlled approval points.

## Feedback Loop Without Too Many API Calls

Avoid:

```text
think -> tiny command -> validate -> screenshot -> inspect -> repeat forever
```

Prefer:

```text
semantic batch -> internal solver loop -> compact report -> diff -> screenshot only at checkpoint
```

Use screenshots for:

- imported board loaded
- module placement completed
- route batch completed
- DRC failure review
- before/after comparison
- human approval point

Use JSON reports for normal feedback:

```json
{
  "errors": 0,
  "warnings": 3,
  "changed_objects": 120,
  "unrouted_nets": 2,
  "worst_clearance_um": 110
}
```

## GUI Role

The GUI is not the source of truth. The GUI is the review and intent surface.

The GUI should support:

- selecting stable objects
- inspecting object properties
- highlighting nets
- showing DRC/ERC overlays
- showing transaction diffs
- showing before/after snapshots
- allowing human approval or rejection

If a human drags a trace or component later, the GUI should convert that action into a structured command. The kernel should snap, validate, accept or reject, then re-render.

## Themes, Plugins, And Custom Component Creators

CCad should stay open to future user, team, and vendor customization without letting customization become hidden source-of-truth logic. Themes, plugins, custom component creators, and schematic/PCB assistants should be clients of typed kernel APIs and transaction APIs.

Themes should control presentation only. A theme may define colors, stroke weights, selected-object highlights, marker styles, grid density, contrast modes, and per-layer rendering palettes. A theme must not change geometry, electrical meaning, DRC rules, net membership, component identity, or project serialization. Selection highlights should derive from the object's normal display color, usually as a lighter or higher-contrast variant, so custom themes remain visually coherent.

Plugins should use explicit capability boundaries. A plugin may provide component creation wizards, schematic module generators, footprint selection helpers, placement policies, routing policies, review panels, custom DRC checks, manufacturing checks, importers, exporters, or visualization overlays. Plugins must communicate through documented schemas, must preserve provenance, and must not execute project or library data as code.

Custom component creators should work across schematic and PCB phases. A creator may define symbol pins, pin roles, package choices, footprint pads, 3D references, electrical ratings, recommended passives, layout warnings, and manufacturing notes. The output should be a proposed typed component/library record with provenance, confidence, and review status, not an opaque GUI-only object. The same component record should support schematic placement, PCB footprint placement, DRC/ERC checks, catalog search, and later manufacturing export.

The long-term plugin model should separate trusted kernel code from extension code. Core validation, serialization, transactions, and security gates stay in `ccad_core`. Extensions run through local process boundaries, signed packages, restricted scripting hosts, or future RPC/MCP capabilities with explicit permissions. Every extension-created design change should become a transaction with a diff and diagnostics.

## Routing Risk

Routing traces is dangerous if the LLM controls raw coordinates directly.

The LLM should request routing by intent:

```text
route USB_D+ and USB_D- as differential pair
keep skew below 0.5 mm
avoid power switch node
prefer F.Cu then B.Cu
```

The routing engine should generate geometry, run checks, and return violations. The LLM should fix exceptions, not micromanage every segment.

## Near-Term Project Work

The next architecture work should add:

- component knowledge schema
- richer catalog item fields
- local document and datasheet cache model
- curation/review status fields
- semantic module representation
- batch transaction command model
- compact validation report format
- visual checkpoint script conventions

This is required before CCad can credibly handle large existing projects or large generated designs.
