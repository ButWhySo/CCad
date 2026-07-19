# KiCad Source Walk: Dialogs - Export ODB++ & STEP (Batch 19)

## Overview
This batch covers the dialog interfaces for exporting the PCB into two additional industry standard formats: ODB++ (manufacturing) and STEP/GLTF (3D MCAD/Rendering).

- **Files**:
  - `pcbnew/dialogs/dialog_export_idf_base.cpp`, `.h` (Base UI for previous batch)
  - `pcbnew/dialogs/dialog_export_odbpp.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_export_step.cpp`, `.h`

## 1. Export ODB++ (`dialog_export_odbpp`)
- **Purpose**: Generates ODB++ directory structures and optionally archives them (ZIP/TGZ). ODB++ is an intelligent manufacturing format competing with IPC-2581.
- **Mechanism**:
  - Provides configuration for Units, Precision, and Compression (None, ZIP, TGZ).
  - Uses `PCB_IO_MGR::ODBPP` plugin (`pi->SaveBoard`) inside a background thread pool task.
  - The thread pool usage is interesting: it allows the UI to stay responsive and updates a `PROGRESS_REPORTER` by polling `std::future_status`.
  - Manual packaging of the generated directory into `.zip` or `.tgz` using `wxZipOutputStream` / `wxTarOutputStream`.

## 2. Export 3D / STEP (`dialog_export_step`)
- **Purpose**: A comprehensive dialog to export the 3D model of the board (STEP, GLTF, VRML, STL, etc.).
- **Mechanism**:
  - Extremely dense configuration surface. Allows filtering out nets, specific components (selected, regex filtered, DNP, unspecified), and fine-grained toggles for exporting Tracks, Pads, Zones, Inner Copper, Silkscreen, Soldermask.
  - Controls mechanical origin (Grid, Drill, Board Center, User Defined).
  - Validates board outline continuity before exporting (`BuildBoardPolygonOutlines` with chaining epsilon).
  - **Execution**: The actual export is *NOT* done in-process. It spawns the standalone `kicad-cli` binary using `wxProcess` / `wxExecute` through a wrapper dialog `DIALOG_EXPORT_STEP_LOG`.
  - It constructs a massive CLI command string `kicad-cli pcb export step/glb/... --subst-models --include-tracks...` and runs it out-of-process.
- **CCad Relevance**: 
  - The ODB++ background thread export and the STEP out-of-process CLI export both show that heavy geometry/manufacturing computations should not block the main UI thread. CCad's kernel-first architecture handles this natively, as the CLI/headless layer is the source of truth, and the GUI merely submits jobs to the kernel.
