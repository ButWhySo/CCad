## BOARD_TEXT_VAR_ADAPTER
- **File**: `pcbnew/board_text_var_adapter.cpp`, `pcbnew/board_text_var_adapter.h`
- **Purpose**: Integrates the `BOARD`'s items with the `TEXT_VAR_TRACKER` for dynamic text variable resolution (e.g., `${REFDES:FIELD}`).
- **Functionality**: 
  - Subclasses `BOARD_LISTENER` to automatically hear `OnBoardItemAdded`, `OnBoardItemChanged`, etc.
  - Intercepts items like `EDA_TEXT`, `PCB_BARCODE`, and `FOOTPRINT` (which itself contains fields/graphics) to register them with the tracker if they use `${...}` variables.
  - Can cross-reference values from footprints using `ExtractSourceKeys()`, meaning if `U1`'s Value field changes, all text objects dynamically referencing `${U1:Value}` are invalidated and repainted.
- **Context**: Plugs into the `BOARD` at instantiation to keep the text-variable dependency graph in sync with board geometry edits.
