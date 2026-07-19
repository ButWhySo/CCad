# KiCad Exploration - Batch 27: Footprint Position File and Get Footprint By Name Dialogs

## Overview
This batch covers the dialog used for generating footprint position (pick and place) files (`dialog_gen_footprint_position`) and the dialog for getting/selecting a footprint by reference name (`dialog_get_footprint_by_name`).

## Key Findings

### Footprint Position File Generation Dialog
- **`dialog_gen_footprint_position.cpp/.h`**: 
  - Dialog to generate component placement (pick and place) files.
  - Used for automated placement machines.
  - Generates either ASCII (Plain Text / CSV) or Gerber X3 position files.
  - Instantiated directly via `BOARD_EDITOR_CONTROL::GeneratePosFile` or via job configuration `JOB_EXPORT_PCB_POS`.
  - **Options**:
    - **Variants**: Support for filtering components by design variant (`m_variantChoiceCtrl`).
    - **Format**: Plain text, CSV, Gerber X3.
    - **Units**: Inches, Millimeters.
    - **Filters**: Only SMD, Exclude TH (Through Hole), Exclude DNP (Do Not Populate), Exclude BOM (Exclude from BOM).
    - **Transforms**: Use negative X coordinates for bottom layer, Use drill/place file origin (vs absolute).
    - **Output files**: Separate Front/Back files or single file combining both.
  - **Implementations**:
    - Gerber files use `PLACEFILE_GERBER_WRITER`.
    - ASCII files use `PLACE_FILE_EXPORTER` and `PCB_EDIT_FRAME::DoGenFootprintsPositionFile`.
    - Footprint Reports use `BOARD_EDITOR_CONTROL::GenFootprintsReport`.

### Get Footprint By Name Dialog
- **`dialog_get_footprint_by_name.cpp/.h`**: 
  - Helper dialog to find and select a footprint by its reference designator (e.g., "R1").
  - Provides an auto-completing search text control (`m_SearchTextCtrl`) and a dropdown choice list (`m_choiceFpList`).
  - **Dynamic filtering**: As the user types in the search box, `OnSearchInputChanged` filters or selects the corresponding footprint in the dropdown.
  - Allows passing an initial list of footprints (`wxArrayString& aFpList`).
  - Returns the selected reference string via `GetValue()`.
