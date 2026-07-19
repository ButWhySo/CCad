## pad_utils
- **File**: `pcbnew/pad_utils.cpp`, `pcbnew/pad_utils.h`
- **Purpose**: Helper functions for pads, mainly focusing on rounding geometry logic.
- **Functionality**:
  - `PadHasMeaningfulRoundingRadius`: Validates if a given pad layer shape supports rounding.
  - `GetDefaultIpcRoundingRatio`: Computes a sensible IPC-7351C rounding ratio for pads.
- **Context**: Used to validate properties during footprint parsing, properties dialog editing, and geometry generation.

## pcbexpr_evaluator
- **File**: `pcbnew/pcbexpr_evaluator.cpp`, `pcbnew/pcbexpr_evaluator.h`
- **Purpose**: Integration layer between KiCad's generic expression evaluation library (`libeval`) and the `pcbnew` board object model.
- **Functionality**:
  - Implements `PCBEXPR_CONTEXT` which holds the board state context for evaluating an expression against one or two `BOARD_ITEM`s.
  - Overrides `LIBEVAL::VAR_REF` creation to map variables like "A.NetClass", "A.ComponentClass", "A.NetName", "A.Layer" to C++ methods on `BOARD_ITEM` and `BOARD_CONNECTED_ITEM`.
  - Implements custom `LIBEVAL::VALUE` subclasses for `PCBEXPR_LAYER_VALUE`, `PCBEXPR_PINTYPE_VALUE`, `PCBEXPR_NETCLASS_VALUE`, etc., which define domain-specific equality (e.g. matching wildcard string pin types against enum pin types).
  - Handles string-to-unit conversions natively using `EDA_UNIT_UTILS` logic inside the evaluator.
- **Context**: The engine behind KiCad's custom Design Rule Checker (DRC) query language, allowing users to write complex constraints like `(rule "..." (condition "A.Type == 'Pad' && A.NetClass == 'Power'") (constraint clearance (min 0.5mm)))`.
