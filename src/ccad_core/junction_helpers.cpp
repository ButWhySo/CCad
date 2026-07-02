#include "junction_helpers.hpp"
#include <map>
#include <vector>

namespace ccad {

// Helper to compare points exactly for map keys
struct PointCmp {
  bool operator()(const Point& a, const Point& b) const {
    if (a.x.nanometers != b.x.nanometers) return a.x.nanometers < b.x.nanometers;
    return a.y.nanometers < b.y.nanometers;
  }
};

void fixSchematicJunctions(Schematic& schematic) {
  std::map<Point, int, PointCmp> wire_endpoints;
  std::map<Point, int, PointCmp> pin_points;

  // 1. Gather all wire endpoints
  for (const auto& wire : schematic.wires) {
    wire_endpoints[wire.start]++;
    wire_endpoints[wire.end]++;
  }

  // 2. Gather all symbol pin absolute positions
  for (const auto& sch_sym : schematic.symbols) {
    if (sch_sym.symbol.has_value()) {
      for (const auto& pin : sch_sym.symbol->pins) {
        // Compute absolute pin position based on symbol position
        // This is a simplified position calculation (ignoring rotation for parity demo)
        long long abs_x = sch_sym.position.x.nanometers + pin.position.x.nanometers;
        long long abs_y = sch_sym.position.y.nanometers + pin.position.y.nanometers;
        Point p = {Length(abs_x), Length(abs_y)};
        pin_points[p]++;
      }
    }
  }

  // 3. Keep existing junctions in a set to avoid duplicates
  std::map<Point, bool, PointCmp> existing_junctions;
  for (const auto& j : schematic.junctions) {
    existing_junctions[j.position] = true;
  }

  // 4. Evaluate points for new junctions
  std::vector<SchJunction> new_junctions;
  for (const auto& [pt, count] : wire_endpoints) {
    int pins_here = pin_points[pt];
    
    // Condition to add a junction:
    // 3 or more wires meet
    // OR 2 wires and a pin meet
    bool needs_junction = (count >= 3) || (count >= 2 && pins_here >= 1);
    
    if (needs_junction && !existing_junctions[pt]) {
      SchJunction j;
      j.position = pt;
      new_junctions.push_back(j);
      existing_junctions[pt] = true;
    }
  }

  // 5. Append the new junctions
  schematic.junctions.insert(schematic.junctions.end(), new_junctions.begin(), new_junctions.end());
}

} // namespace ccad
