#include "test_support.hpp"
#include "../src/ccad_core/junction_helpers.hpp"
#include <iostream>

void testThreeWiresJunction() {
  ccad::Schematic sch;
  
  // Three wires meeting at (10, 10)
  ccad::SchWire w1;
  w1.start = {ccad::millimeters(0), ccad::millimeters(10)};
  w1.end = {ccad::millimeters(10), ccad::millimeters(10)};
  sch.wires.push_back(w1);
  
  ccad::SchWire w2;
  w2.start = {ccad::millimeters(10), ccad::millimeters(10)};
  w2.end = {ccad::millimeters(20), ccad::millimeters(10)};
  sch.wires.push_back(w2);
  
  ccad::SchWire w3;
  w3.start = {ccad::millimeters(10), ccad::millimeters(10)};
  w3.end = {ccad::millimeters(10), ccad::millimeters(20)};
  sch.wires.push_back(w3);
  
  ccad::fixSchematicJunctions(sch);
  
  require(sch.junctions.size() == 1, "Should create exactly 1 junction");
  require(sch.junctions[0].position.x.nanometers == ccad::millimeters(10).nanometers, "Junction X should be 10");
  require(sch.junctions[0].position.y.nanometers == ccad::millimeters(10).nanometers, "Junction Y should be 10");
}

void testTwoWiresNoJunction() {
  ccad::Schematic sch;
  
  // Two wires meeting at (10, 10), no pin
  ccad::SchWire w1;
  w1.start = {ccad::millimeters(0), ccad::millimeters(10)};
  w1.end = {ccad::millimeters(10), ccad::millimeters(10)};
  sch.wires.push_back(w1);
  
  ccad::SchWire w2;
  w2.start = {ccad::millimeters(10), ccad::millimeters(10)};
  w2.end = {ccad::millimeters(10), ccad::millimeters(20)};
  sch.wires.push_back(w2);
  
  ccad::fixSchematicJunctions(sch);
  
  require(sch.junctions.size() == 0, "Should not create a junction for just 2 wires");
}

void testTwoWiresAndPinJunction() {
  ccad::Schematic sch;
  
  // Two wires meeting at (10, 10)
  ccad::SchWire w1;
  w1.start = {ccad::millimeters(0), ccad::millimeters(10)};
  w1.end = {ccad::millimeters(10), ccad::millimeters(10)};
  sch.wires.push_back(w1);
  
  ccad::SchWire w2;
  w2.start = {ccad::millimeters(10), ccad::millimeters(10)};
  w2.end = {ccad::millimeters(10), ccad::millimeters(20)};
  sch.wires.push_back(w2);
  
  // A symbol with a pin at (10, 10)
  ccad::SchSymbol sym;
  sym.position = {ccad::millimeters(10), ccad::millimeters(10)};
  sym.symbol = ccad::Symbol();
  sym.symbol->pins.push_back({.position = {ccad::millimeters(0), ccad::millimeters(0)}}); // Relative 0,0 -> Absolute 10,10
  sch.symbols.push_back(sym);
  
  ccad::fixSchematicJunctions(sch);
  
  require(sch.junctions.size() == 1, "Should create a junction when 2 wires hit a pin");
}

int main() {
  std::cout << "test_junction_helpers starting\n";
  testThreeWiresJunction();
  testTwoWiresNoJunction();
  testTwoWiresAndPinJunction();
  std::cout << "test_junction_helpers passed\n";
  return 0;
}
