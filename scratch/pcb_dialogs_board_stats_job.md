# KiCad Source Walk: Dialogs - Board Statistics & Job Export (Batch 13)

## Overview
This batch covers the UI and job configuration logic for extracting and presenting PCB statistics (e.g. board area, copper density, via count, drill properties).

- **Files**:
  - `pcbnew/dialogs/dialog_board_setup.h` (Header for Board Setup UI, previously reviewed)
  - `pcbnew/dialogs/dialog_board_statistics.cpp`, `.h`
  - `pcbnew/dialogs/dialog_board_statistics_base.cpp`, `.h`
  - `pcbnew/dialogs/dialog_board_stats_job.cpp`, `.h`
  - `pcbnew/dialogs/dialog_board_stats_job_base.cpp`

## 1. Board Statistics Dialog (`dialog_board_statistics`)
- **Purpose**: Displays a summary of the physical board attributes, components, pads, vias, and drill configurations.
- **Mechanism**:
  - Defines `DIALOG_BOARD_STATISTICS` inheriting from `DIALOG_BOARD_STATISTICS_BASE`.
  - The dialog relies heavily on `BOARD_STATISTICS_DATA` (defined elsewhere) which is populated by `ComputeBoardStatistics()`.
  - It uses multiple `wxGrid` instances to display tabular data:
    - `m_gridComponents`: Footprint counts (Front, Back, Total).
    - `m_gridBoard`: Board Dimensions, Total Area, Copper Area (Front/Back), Footprint Densities.
    - `m_gridPads`: Pad counts grouped by type/property (e.g. Through-hole, SMD).
    - `m_gridVias`: Via counts grouped by type (Through, Blind/Buried, Micro).
    - `m_gridDrills`: A separate drill hole summary table (Size, Shape, Plating, Start/Stop layers).
  - Contains checkboxes to configure the calculation (`excludeNoPins`, `subtractHoles`, `subtractHolesFromCopper`), which immediately trigger a recalculation via `ComputeBoardStatistics()`.
  - Can generate a text report via `FormatBoardStatisticsReport()`.

## 2. Board Statistics Job Export (`dialog_board_stats_job`)
- **Purpose**: Defines the UI configuration panel for exporting board statistics as part of a batch job (e.g. CI/CD pipelines, automated manufacturing outputs).
- **Mechanism**:
  - Configures a `JOB_EXPORT_PCB_STATS` object.
  - Allows selecting the output format (Report / JSON) and unit preference (mm / inches).
  - Also surfaces the same calculation options as the interactive dialog (Exclude footprints without pads, subtract holes).
- **CCad Relevance**: The ability to export board statistics natively to JSON is an excellent hook for AI-driven workflows. The agent can invoke the headless job equivalent of this dialog to quickly digest board complexity, copper usage, and component counts into machine-readable JSON formats, which could inform LLM routing decisions or manufacturing rule checks.
