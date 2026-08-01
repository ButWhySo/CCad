## BOARD_STACKUP_MANAGER
- **File**: `pcbnew/board_stackup_manager/`
- **Purpose**: UI panels, settings, dialogs, and reports for physical board stackup (copper, dielectrics, finishes).
- **Key Files**: 
  - `board_stackup_reporter.h/cpp`: Builds string-based reports of the stackup.
  - `dielectric_material.h/cpp`: `DIELECTRIC_SUBSTRATE_LIST` manages pre-defined and custom substrates (EpsilonR, Loss Tangent).
  - Various UI panel handlers (Dialog Dielectric List, Panel Board Finish, Panel Board Stackup).

## BOARD_TABLES
- **File**: `pcbnew/board_tables/`
- **Purpose**: Generators that transform board state or stackup data into drawn graphical `PCB_TABLE` items on the board.
- **Key Methods**:
  - `Build_Board_Characteristics_Table( BOARD* )`: Inspects the board statistics (from `ComputeBoardStatistics()`) and design settings to build a table of layer counts, min tracks, drills, finishes, impedance constraints.
  - `Build_Board_Stackup_Table( BOARD* )`: Reads the `BOARD_STACKUP` descriptor and outputs a `PCB_TABLE` documenting the physical layers, their thicknesses, materials, and colors.
