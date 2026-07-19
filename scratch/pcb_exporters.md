## EXPORTERS
- **File**: `pcbnew/exporters/`
- **Purpose**: A suite of tools to translate board models into 3rd party or manufacturing formats.
- **Key Exporters**:
  - `gendrill_writer_base.h/cpp` (and `gendrill_excellon_writer`, `gendrill_gerber_writer`): Aggregates hole/drill spans, calculates min-stubs for backdrills, separates PTH/NPTH holes, and constructs exact tool diameter allocations for NC manufacturing files.
  - `gerber_jobfile_writer.h/cpp`: Produces a standard `json` `.gbrjob` file containing layer metadata, stackup parameters (masks, silk, finish, dimensions), and a list of Gerber files to guide automated CAM ingest.
  - Generates GenCAD, VRML (for 3D), IPC-D-356 (netlist bareboard testing), Hyperlynx (SI simulation), IDF (3D CAD).
