# KiCad Source Walk: Dialogs - Export 2581 & IDF (Batch 18)

## Overview
This batch covers the dialog interfaces for exporting the PCB into two distinct industry standard formats: IPC-2581 (used heavily for CAM and fabrication) and IDF (used for mechanical CAD integration).

- **Files**:
  - `pcbnew/dialogs/dialog_exchange_footprints_base.cpp`, `.h` (Base UI for previous batch)
  - `pcbnew/dialogs/dialog_export_2581.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_export_idf.cpp`, `.h`

## 1. Export IPC-2581 (`dialog_export_2581`)
- **Purpose**: Generates IPC-2581 standard output for manufacturing. Can operate standalone or driven via `JOB_EXPORT_PCB_IPC2581` in batch mode.
- **Mechanism**:
  - Exposes configuration for Units (mm/inch), Precision, Version (B or C), and compression (ZIP).
  - Strongly tied to BOM generation: attempts to infer Distributor and Manufacturer parts by inspecting `PCB_FIELD` instances in the board's footprints.
  - The actual export is executed by locating the `PCB_IO_MGR::IPC2581` plugin, injecting the properties (OEMRef, mpn, mfg, dist, etc.) and calling `pi->SaveBoard()`.
  - Also handles wrapping the generated `.xml` file inside a ZIP archive using `wxZipOutputStream` if compression is requested.

## 2. Export IDF (`dialog_export_idf`)
- **Purpose**: Exports a 3D mechanical approximation of the board (IDFv3 format) mapping footprints to simple geometric shapes for integration into MCAD tools (SolidWorks, FreeCAD, etc.).
- **Mechanism**:
  - Minimal dialog providing an output file picker, unit choice, and the ability to define a custom board reference point (X/Y offset) instead of using the computed bounding box center.
  - Exposes options to ignore footprints missing 3D models or marked as DNP (Do Not Populate).
  - The dialog returns to `BOARD_EDITOR_CONTROL::ExportIDF()`, which delegates the actual work to `m_frame->Export_IDF3(...)`.
- **CCad Relevance**: 
  - Both 2581 and IDF represent essential outputs for a CAD tool. `dialog_export_2581` demonstrates a clean pattern of encapsulating export configuration into a "Job" data structure, executing the plugin, and reporting progress back to the UI. CCad should natively implement export pipelines as background tasks/jobs. The UI should merely be a way to build the job payload.
