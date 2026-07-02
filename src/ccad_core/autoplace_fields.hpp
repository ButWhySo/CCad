#ifndef CCAD_CORE_AUTOPLACE_FIELDS_HPP
#define CCAD_CORE_AUTOPLACE_FIELDS_HPP

#include "model.hpp"

namespace ccad {

enum class AutoplaceSide {
  Top,
  Bottom,
  Left,
  Right
};

// Represents configuration options for schematic field autoplacement
struct AutoplaceOptions {
  bool allow_rejustify = true;
  bool align_to_grid = true; // usually 50 mils (1270000 nm)
  bool avoid_collisions = true;
};

// Autoplace fields for a single symbol
void autoplaceSymbolFields(Schematic& schematic, SchSymbol& symbol, const AutoplaceOptions& options);

// Autoplace fields for all symbols in a schematic
void autoplaceSchematicFields(Schematic& schematic, const AutoplaceOptions& options);

} // namespace ccad

#endif // CCAD_CORE_AUTOPLACE_FIELDS_HPP
