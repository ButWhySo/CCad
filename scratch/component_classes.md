## COMPONENT_CLASS
- **File**: `pcbnew/component_classes/component_class.h`, `pcbnew/component_classes/component_class.cpp`
- **Purpose**: Lightweight representation of a named "component class" tag assigned to footprints. A footprint can belong to multiple classes simultaneously.
- **Three kinds** (determined by `m_constituentClasses.size()`):
  - **Null class** (size 0): No assignment. `m_name` is empty.
  - **Atomic class** (size 1): One named class. The constituent pointer refers to itself.
  - **Composite/Effective class** (size > 1): Aggregation of multiple atomic classes. `m_name` is a comma-delimited concatenation.
- **USAGE enum**: `STATIC` (from netlist/schematic), `DYNAMIC` (from assignment rules), `STATIC_AND_DYNAMIC`, `EFFECTIVE` (computed aggregate).
- **Key Methods**: `GetName()`, `GetHumanReadableName()`, `AddConstituentClass(cls)`, `GetConstituentClass(name)`, `ContainsClassName(name)`, `IsEmpty()`, `GetConstituentClasses()`.

---

## COMPONENT_CLASS_ASSIGNMENT_RULE
- **File**: `pcbnew/component_classes/component_class_assignment_rule.h`, `pcbnew/component_classes/component_class_assignment_rule.cpp`
- **Purpose**: A single dynamic rule that assigns a class name to footprints matching a DRC expression condition.
- **Key Fields**: `m_componentClass` (name to assign), `m_condition` (`DRC_RULE_CONDITION*` — uses the DRC expression language to filter footprints).
- **Key Method**: `Matches(footprint)` — evaluates the DRC condition against a footprint and returns true if it qualifies.
- **Integration**: Rules are compiled from `COMPONENT_CLASS_ASSIGNMENT_DATA` via `COMPONENT_CLASS_MANAGER::CompileAssignmentRule()` and stored in `m_assignmentRules`.

---

## COMPONENT_CLASS_MANAGER
- **File**: `pcbnew/component_classes/component_class_manager.h`, `pcbnew/component_classes/component_class_manager.cpp`
- **Purpose**: Owns all `COMPONENT_CLASS` objects for a board. Guarantees pointer stability for the board lifetime.
- **Two assignment modes**:
  - **Static**: From netlist — footprints have schematic-assigned class names baked in.
  - **Dynamic**: From rules — evaluated at DRC/update time via `COMPONENT_CLASS_ASSIGNMENT_RULE::Matches()`.
- **Key Methods**:
  - `GetEffectiveStaticComponentClass(classNames)`: Returns the composite class for a set of static class names (creates one if not present).
  - `GetDynamicComponentClassesForFootprint(fp)`: Evaluates all dynamic rules against a footprint and returns the resulting composite dynamic class.
  - `GetCombinedComponentClass(staticClass, dynamicClass)`: Merges static and dynamic into one effective class.
  - `SyncDynamicComponentClassAssignments(assignments, generateSheetClasses, sheetPaths)`: Parses and registers new dynamic rules. Returns false if any rule fails to parse.
  - `InitNetlistUpdate()` / `FinishNetlistUpdate()`: Bracket a netlist synchronization to safely GC unused classes.
  - `ForceComponentClassRecalculation()`: Pre-DRC force-recalculate all component class caches (thread-safe gate before DRC parallelism).
  - `InvalidateComponentClasses()`: Increment ticker so all cached per-footprint component classes are stale.
  - `GetTicker()`: Read the validity counter used by `COMPONENT_CLASS_CACHE_PROXY`.
  - `RebuildRequiredCaches(fp)`: Rebuild any per-footprint caches needed by custom rules (e.g. polygon containment).
- **Storage**: `m_constituentClasses` (atomic classes), `m_effectiveClasses` (composite classes), `m_assignmentRules`, `m_ticker`.
- **Context**: Component classes enable DRC rules and netclass overrides that target footprint subsets rather than nets (e.g. "apply tight clearance only to DDR4 components").

---

## COMPONENT_CLASS_CACHE_PROXY
- **File**: `pcbnew/component_classes/component_class_cache_proxy.h`, `pcbnew/component_classes/component_class_cache_proxy.cpp`
- **Purpose**: Per-footprint cache of the resolved effective `COMPONENT_CLASS*`. Avoids recomputing dynamic rules on every access. Invalidated when `COMPONENT_CLASS_MANAGER::GetTicker()` advances.
- **Key Design**: Stores the last-known ticker value. On `GetComponentClass()`, compares current ticker with stored; if stale, requests recalculation from the manager. This design is thread-safe for reads (check ticker first), but recalculation is not thread-safe (hence `ForceComponentClassRecalculation()` before parallel DRC).
