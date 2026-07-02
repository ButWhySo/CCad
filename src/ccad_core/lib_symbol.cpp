#include "ccad_core/lib_symbol.hpp"

namespace ccad {

LibSymbol::LibSymbol(const std::string& name) : name_(name) {}

void LibSymbol::set_parent(std::shared_ptr<LibSymbol> parent) {
  parent_ = parent;
}

std::shared_ptr<LibSymbol> LibSymbol::get_parent() const {
  return parent_.lock();
}

bool LibSymbol::is_root() const {
  return parent_.expired();
}

bool LibSymbol::is_derived() const {
  return !parent_.expired();
}

void LibSymbol::set_symbol_data(const Symbol& symbol) {
  data_ = symbol;
}

const Symbol& LibSymbol::get_symbol_data() const {
  return data_;
}

std::vector<SymbolPin> LibSymbol::get_all_pins() const {
  if (is_root()) {
    return data_.pins;
  }
  
  auto parent = get_parent();
  if (!parent) return data_.pins;

  auto pins = parent->get_all_pins();
  pins.insert(pins.end(), data_.pins.begin(), data_.pins.end());
  return pins;
}

std::vector<SymbolProperty> LibSymbol::get_all_properties() const {
  if (is_root()) {
    return data_.properties;
  }

  auto parent = get_parent();
  if (!parent) return data_.properties;

  auto props = parent->get_all_properties();
  // Override parent properties with child properties that have the same name
  for (const auto& child_prop : data_.properties) {
    bool found = false;
    for (auto& parent_prop : props) {
      if (parent_prop.name == child_prop.name) {
        parent_prop = child_prop;
        found = true;
        break;
      }
    }
    if (!found) {
      props.push_back(child_prop);
    }
  }
  return props;
}

std::vector<SymbolRectangle> LibSymbol::get_all_rectangles() const {
  if (is_root()) return data_.rectangles;
  auto parent = get_parent();
  if (!parent) return data_.rectangles;
  auto items = parent->get_all_rectangles();
  items.insert(items.end(), data_.rectangles.begin(), data_.rectangles.end());
  return items;
}

std::vector<SymbolLine> LibSymbol::get_all_lines() const {
  if (is_root()) return data_.lines;
  auto parent = get_parent();
  if (!parent) return data_.lines;
  auto items = parent->get_all_lines();
  items.insert(items.end(), data_.lines.begin(), data_.lines.end());
  return items;
}

std::vector<SymbolArc> LibSymbol::get_all_arcs() const {
  if (is_root()) return data_.arcs;
  auto parent = get_parent();
  if (!parent) return data_.arcs;
  auto items = parent->get_all_arcs();
  items.insert(items.end(), data_.arcs.begin(), data_.arcs.end());
  return items;
}

std::vector<SymbolCircle> LibSymbol::get_all_circles() const {
  if (is_root()) return data_.circles;
  auto parent = get_parent();
  if (!parent) return data_.circles;
  auto items = parent->get_all_circles();
  items.insert(items.end(), data_.circles.begin(), data_.circles.end());
  return items;
}

std::vector<SymbolPolyline> LibSymbol::get_all_polylines() const {
  if (is_root()) return data_.polylines;
  auto parent = get_parent();
  if (!parent) return data_.polylines;
  auto items = parent->get_all_polylines();
  items.insert(items.end(), data_.polylines.begin(), data_.polylines.end());
  return items;
}

std::vector<SymbolText> LibSymbol::get_all_texts() const {
  if (is_root()) return data_.texts;
  auto parent = get_parent();
  if (!parent) return data_.texts;
  auto items = parent->get_all_texts();
  items.insert(items.end(), data_.texts.begin(), data_.texts.end());
  return items;
}

}  // namespace ccad
