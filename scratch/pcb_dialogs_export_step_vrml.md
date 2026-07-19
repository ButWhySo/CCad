# KiCad Source Walk: Dialogs - Export STEP & VRML (Batch 20)

## Overview
This batch continues the exploration of 3D and mechanical export interfaces, focusing on the background process wrapper for the `kicad-cli` STEP export and the legacy VRML 3D export.

- **Files**:
  - `pcbnew/dialogs/dialog_export_step_base.cpp`, `.h` (Base UI for previous batch)
  - `pcbnew/dialogs/dialog_export_step_process.cpp`, `.h`, `_base.cpp`, `_base.h`
  - `pcbnew/dialogs/dialog_export_vrml.cpp`, `.h`

## 1. STEP Export Process Log (`dialog_export_step_process`)
- **Purpose**: Displays the standard out/error stream of the out-of-process `kicad-cli` execution when exporting 3D models.
- **Mechanism**:
  - `DIALOG_EXPORT_STEP_LOG` inherits from `DIALOG_EXPORT_STEP_PROCESS_BASE` and manages a `wxProcess` and a dedicated thread (`STDSTREAM_THREAD`).
  - `STDSTREAM_THREAD` reads from the `wxInputStream` provided by `wxProcess::GetInputStream()` and `wxProcess::GetErrorStream()`. It then fires `wxThreadEvent` events (`wxEVT_THREAD_STDIN`, `wxEVT_THREAD_STDERR`) back to the main UI thread to update the `wxTextCtrl` log safely.
  - Safe thread shutdown is orchestrated via a `wxMessageQueue<STATE_MESSAGE>` sending `PROCESS_COMPLETE` or `REQUEST_EXIT`.
- **CCad Relevance**: 
  - Demonstrates exactly how KiCad avoids freezing the GUI during multi-minute CLI geometry conversions. In CCad, our architecture inherently sidesteps this by treating the GUI as a client of a backend process (or executing async tasks natively). The pattern of pumping CLI `stdout` through events to a read-only text control is standard desktop UI design for long jobs.

## 2. VRML Export (`dialog_export_vrml`)
- **Purpose**: A dialog for exporting boards to the older VRML format (mostly for rendering, less useful for MCAD than STEP).
- **Mechanism**:
  - Uses `m_frame->ExportVRML_File()` to execute the export.
  - Unlike STEP, this appears to run synchronously or delegates to a backend exporter directly, rather than shelling out to `kicad-cli`.
  - Provides options to copy 3D shape files into a relative subdirectory (usually `shapes3D`) alongside the `.wrl` file, rewriting the paths in the VRML file so the archive is portable.
  - Scales can be selected based on target VRML units (mm, inch, 0.1 inch).
