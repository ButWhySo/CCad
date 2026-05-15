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
