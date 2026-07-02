#ifndef CCAD_CORE_JUNCTION_HELPERS_HPP
#define CCAD_CORE_JUNCTION_HELPERS_HPP

#include "model.hpp"

namespace ccad {

// Helper to automatically add SchJunction elements to schematic where needed.
// This implements a basic heuristic to place a junction:
// 1. Where 3 or more wire endpoints meet.
// 2. Where 2 or more wire endpoints meet at a symbol pin.
// 3. Removes any redundant overlapping junctions.
void fixSchematicJunctions(Schematic& schematic);

} // namespace ccad

#endif // CCAD_CORE_JUNCTION_HELPERS_HPP
