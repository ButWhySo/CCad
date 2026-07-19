## DRC_ENGINE
- **File**: `pcbnew/drc/drc_engine.h/cpp`
- **Purpose**: The central design rule check engine.
- **Key Methods/Structures**: 
  - `InitEngine()`: Compiles custom and implicit rules.
  - `RunTests()`: Executes tests via `DRC_TEST_PROVIDER`s.
  - `EvalRules()`, `EvalClearanceBatch()`: Query the worst-case constraints (e.g. clearance, holes, edge) between two items on a specific layer.
  - Maintains `m_ownClearanceCache` and `m_netclassClearances` for rendering acceleration, meaning `GetCachedOwnClearance()` is called during board rendering to draw clearance halos around traces/pads.
  - Generates violations through a `DRC_VIOLATION_HANDLER` callback.

## DRC_RTREE
- **File**: `pcbnew/drc/drc_rtree.h`
- **Purpose**: A spatial index (packed R-tree) designed specifically for testing board item collisions and distance queries on specific layers.
- **Key Methods**: 
  - Groups items by `PCB_LAYER_ID`.
  - Exposes `QueryColliding()`, `CheckColliding()` for shape and bounding-box tests.
  - Contains `DRC_LAYER` iterators for per-layer traversal.

## DRC_RULE & DRC_CONSTRAINT
- **File**: `pcbnew/drc/drc_rule.h/cpp`
- **Purpose**: Defines custom and implicit rule models.
- **Key Methods**:
  - `DRC_RULE`: Groups conditions, layers, and constraints together.
  - `DRC_CONSTRAINT`: Holds `DRC_CONSTRAINT_T` (the constraint enum like CLEARANCE, CREEPAGE, HOLE, DISALLOW), the `MINOPTMAX<int>` boundary value, and specific settings.
  - `DRC_DISALLOW_T` mask used for identifying forbidden item types in zones/keepouts.

## DRC_CACHE_GENERATOR
- **File**: `pcbnew/drc/drc_cache_generator.h/cpp`
- **Purpose**: A `DRC_TEST_PROVIDER` that doesn't actually flag errors but is used to prime the `DRC_ENGINE`'s spatial or clearance caches before the heavy real tests begin.
