## DRC Test Providers
- **File**: `pcbnew/drc/drc_test_provider*.h/cpp`
- **Purpose**: Specific DRC test implementations (plugins) that run against the board.
- **Key Methods/Structures**: 
  - `DRC_TEST_PROVIDER`: Base class. Implements `Run()`.
  - Registered via static `DRC_REGISTER_TEST_PROVIDER` macro so the engine automatically picks them up.
  - Implementations include clearance tests, connectivity (unrouted nets), track widths, hole sizes, courtyards, silkscreen, thermal reliefs, padstacks.

## DRC Rule Condition & Parser
- **File**: `pcbnew/drc/drc_rule_condition.h/cpp`, `drc_rule_parser.h/cpp`
- **Purpose**: Parses and evaluates s-expression based design rule conditions.
- **Key Methods**:
  - `DRC_RULE_CONDITION::Compile()`: Compiles the text string into `PCBEXPR_UCODE`.
  - `EvaluateFor(ItemA, ItemB)`: Executes the expression using the board items context.
