#include "autoplace_fields.hpp"
#include <algorithm>
#include <cmath>

namespace ccad {

void autoplaceSymbolFields(Schematic& schematic, SchSymbol& symbol, const AutoplaceOptions& options) {
  // Bounding box heuristic for symbol. Since CCad's Symbol struct might not have a direct bbox yet,
  // we assume a nominal body size of 10x10 mm (10000000 nm).
  const long long half_size = 5000000; // 5 mm
  
  long long cx = symbol.position.x.nanometers;
  long long cy = symbol.position.y.nanometers;

  // Determine which sides have the most pins.
  int pins_top = 0, pins_bottom = 0, pins_left = 0, pins_right = 0;
  
  if (symbol.symbol.has_value()) {
    for (const auto& pin : symbol.symbol->pins) {
      // Symbol pin positions are relative to the symbol's unrotated origin.
      // But for simplicity in parity logic, we just use the relative Y/X.
      long long py = pin.position.y.nanometers;
      long long px = pin.position.x.nanometers;
      if (py < 0) pins_top++;
      if (py > 0) pins_bottom++;
      if (px < 0) pins_left++;
      if (px > 0) pins_right++;
    }
  }

  // Count adjacent wires (collision avoidance) if options.avoid_collisions is true
  if (options.avoid_collisions) {
    for (const auto& wire : schematic.wires) {
      long long start_y = wire.start.y.nanometers;
      long long start_x = wire.start.x.nanometers;
      
      // Rough proximity check
      if (std::abs(start_x - cx) < half_size * 2 && std::abs(start_y - cy) < half_size * 2) {
        if (start_y < cy) pins_top += 2; // Weight wires heavily
        if (start_y > cy) pins_bottom += 2;
        if (start_x < cx) pins_left += 2;
        if (start_x > cx) pins_right += 2;
      }
    }
  }

  // Choose the side with the minimum interference. Default preference order: Top, Bottom, Right, Left.
  AutoplaceSide chosen_side = AutoplaceSide::Top;
  int min_interference = pins_top;
  
  if (pins_bottom < min_interference) {
    min_interference = pins_bottom;
    chosen_side = AutoplaceSide::Bottom;
  }
  if (pins_right < min_interference) {
    min_interference = pins_right;
    chosen_side = AutoplaceSide::Right;
  }
  if (pins_left < min_interference) {
    min_interference = pins_left;
    chosen_side = AutoplaceSide::Left;
  }

  // Place fields
  long long offset_x = 0;
  long long offset_y = 0;
  long long step_y = 2540000; // 100 mils
  
  switch (chosen_side) {
    case AutoplaceSide::Top:
      offset_y = -half_size - step_y;
      break;
    case AutoplaceSide::Bottom:
      offset_y = half_size + step_y;
      break;
    case AutoplaceSide::Left:
      offset_x = -half_size - step_y;
      break;
    case AutoplaceSide::Right:
      offset_x = half_size + step_y;
      break;
  }

  // Sort visible fields by some standard order (Reference first, then Value)
  // For CCad, we just iterate them.
  int idx = 0;
  for (auto& field : symbol.fields) {
    if (!field.visible) continue;
    
    // Set position
    field.position.x.nanometers = cx + offset_x;
    field.position.y.nanometers = cy + offset_y + (idx * step_y);
    
    // Align to grid (50 mils = 1270000 nm)
    if (options.align_to_grid) {
      const long long grid = 1270000;
      field.position.x.nanometers = std::round((double)field.position.x.nanometers / grid) * grid;
      field.position.y.nanometers = std::round((double)field.position.y.nanometers / grid) * grid;
    }
    
    idx++;
  }
}

void autoplaceSchematicFields(Schematic& schematic, const AutoplaceOptions& options) {
  for (auto& symbol : schematic.symbols) {
    autoplaceSymbolFields(schematic, symbol, options);
  }
}

} // namespace ccad
