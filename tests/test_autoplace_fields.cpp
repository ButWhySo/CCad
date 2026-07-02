#include "test_support.hpp"
#include "../src/ccad_core/autoplace_fields.hpp"
#include <iostream>

void testTopPlacement() {
  ccad::Schematic sch;
  ccad::SchSymbol sym;
  sym.position = {ccad::millimeters(50), ccad::millimeters(50)};
  sym.symbol = ccad::Symbol();
  
  // Add pins on bottom, left, and right, making top the best place
  // Note: Symbol pins are relative to symbol unrotated origin (0,0 in this case)
  sym.symbol->pins.push_back({.position = {ccad::millimeters(0), ccad::millimeters(10)}}); // bottom
  sym.symbol->pins.push_back({.position = {ccad::millimeters(-10), ccad::millimeters(0)}}); // left
  sym.symbol->pins.push_back({.position = {ccad::millimeters(10), ccad::millimeters(0)}}); // right
  
  // Add some visible fields
  sym.fields.push_back(ccad::SchField{.name = "Reference", .text = "U1", .visible = true});
  sym.fields.push_back(ccad::SchField{.name = "Value", .text = "IC", .visible = true});
  sch.symbols.push_back(sym);
  
  ccad::AutoplaceOptions opts;
  opts.align_to_grid = false; // Disable grid alignment to check exact heuristic offsets
  ccad::autoplaceSchematicFields(sch, opts);
  
  // The first field should be placed above the symbol (Y < 50)
  require(sch.symbols[0].fields[0].position.y.nanometers < ccad::millimeters(50).nanometers, "Field should be placed on Top");
}

void testBottomPlacementWithWires() {
  ccad::Schematic sch;
  ccad::SchSymbol sym;
  sym.position = {ccad::millimeters(50), ccad::millimeters(50)};
  sym.symbol = ccad::Symbol();
  
  // Add pins on top, left, right
  sym.symbol->pins.push_back({.position = {ccad::millimeters(0), ccad::millimeters(-10)}}); // top
  sym.symbol->pins.push_back({.position = {ccad::millimeters(-10), ccad::millimeters(0)}}); // left
  sym.symbol->pins.push_back({.position = {ccad::millimeters(10), ccad::millimeters(0)}}); // right
  
  sym.fields.push_back(ccad::SchField{.name = "Reference", .text = "R1", .visible = true});
  sch.symbols.push_back(sym);
  
  // Add a wire on top to add collision weight
  ccad::SchWire wire;
  wire.start = {ccad::millimeters(40), ccad::millimeters(35)};
  wire.end = {ccad::millimeters(60), ccad::millimeters(35)};
  sch.wires.push_back(wire);
  
  ccad::AutoplaceOptions opts;
  opts.align_to_grid = false;
  ccad::autoplaceSchematicFields(sch, opts);
  
  // Since top has pins and a wire, and bottom has neither, it should choose bottom
  require(sch.symbols[0].fields[0].position.y.nanometers > ccad::millimeters(50).nanometers, "Field should be placed on Bottom due to wire collision on Top");
}

int main() {
  std::cout << "test_autoplace_fields starting\n";
  testTopPlacement();
  testBottomPlacementWithWires();
  std::cout << "test_autoplace_fields passed\n";
  return 0;
}
