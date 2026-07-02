#pragma once

#include "ccad_core/symbol.hpp"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

namespace ccad {

// Represents an instantiated library symbol which can inherit properties and pins from a parent.
class LibSymbol {
public:
  LibSymbol(const std::string& name);
  ~LibSymbol() = default;

  // Set the parent symbol from which this symbol inherits
  void set_parent(std::shared_ptr<LibSymbol> parent);
  std::shared_ptr<LibSymbol> get_parent() const;

  bool is_root() const;
  bool is_derived() const;

  // The base data for this symbol
  void set_symbol_data(const Symbol& symbol);
  const Symbol& get_symbol_data() const;

  // Resolves the flat list of pins (including inherited ones)
  std::vector<SymbolPin> get_all_pins() const;

  // Resolves the flat list of properties (derived properties override parent ones)
  std::vector<SymbolProperty> get_all_properties() const;

  // Resolves all graphics (rectangles, lines, etc)
  std::vector<SymbolRectangle> get_all_rectangles() const;
  std::vector<SymbolLine> get_all_lines() const;
  std::vector<SymbolArc> get_all_arcs() const;
  std::vector<SymbolCircle> get_all_circles() const;
  std::vector<SymbolPolyline> get_all_polylines() const;
  std::vector<SymbolText> get_all_texts() const;

  const std::string& get_name() const { return name_; }

private:
  std::string name_;
  std::weak_ptr<LibSymbol> parent_;
  Symbol data_;
};

}  // namespace ccad
