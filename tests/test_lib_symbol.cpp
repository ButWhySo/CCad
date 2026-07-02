#include "ccad_core/lib_symbol.hpp"
#include <iostream>
#include <cassert>

using namespace ccad;

void test_lib_symbol_inheritance() {
  auto parent = std::make_shared<LibSymbol>("ParentSymbol");
  Symbol parent_data;
  parent_data.name = "ParentSymbol";
  
  SymbolPin p_pin;
  p_pin.name = "VCC";
  p_pin.number = "1";
  parent_data.pins.push_back(p_pin);
  
  SymbolProperty p_prop;
  p_prop.name = "Reference";
  p_prop.value = "U";
  parent_data.properties.push_back(p_prop);
  
  parent->set_symbol_data(parent_data);

  auto child = std::make_shared<LibSymbol>("ChildSymbol");
  child->set_parent(parent);
  Symbol child_data;
  child_data.name = "ChildSymbol";
  
  SymbolProperty c_prop;
  c_prop.name = "Value";
  c_prop.value = "LM358";
  child_data.properties.push_back(c_prop);

  SymbolProperty c_prop_override;
  c_prop_override.name = "Reference";
  c_prop_override.value = "IC";
  child_data.properties.push_back(c_prop_override);

  child->set_symbol_data(child_data);

  // Test pin inheritance
  auto pins = child->get_all_pins();
  assert(pins.size() == 1);
  assert(pins[0].name == "VCC");

  // Test property inheritance and override
  auto props = child->get_all_properties();
  assert(props.size() == 2);
  
  bool found_value = false;
  bool found_ref = false;
  for (const auto& prop : props) {
    if (prop.name == "Value") {
      assert(prop.value == "LM358");
      found_value = true;
    }
    if (prop.name == "Reference") {
      assert(prop.value == "IC"); // Child override should win
      found_ref = true;
    }
  }
  assert(found_value);
  assert(found_ref);

  std::cout << "test_lib_symbol_inheritance passed!\n";
}

int main() {
  test_lib_symbol_inheritance();
  return 0;
}
