#include "test_support.hpp"
#include "../src/ccad_core/annotate.hpp"
#include <iostream>

void testKeepExisting() {
  ccad::Schematic sch;
  sch.symbols.push_back(ccad::SchSymbol{.reference = "R1", .position = {ccad::millimeters(10), ccad::millimeters(10)}});
  sch.symbols.push_back(ccad::SchSymbol{.reference = "R?", .position = {ccad::millimeters(20), ccad::millimeters(10)}});
  sch.symbols.push_back(ccad::SchSymbol{.reference = "R?", .position = {ccad::millimeters(30), ccad::millimeters(10)}});
  
  ccad::AnnotateOptions opts;
  opts.algo = ccad::AnnotateAlgo::KeepExisting;
  ccad::annotateSchematic(sch, opts);
  
  require(sch.symbols[0].reference == "R1", "R1 should remain R1");
  require(sch.symbols[1].reference == "R2", "R? should become R2");
  require(sch.symbols[2].reference == "R3", "R? should become R3");
}

void testResetAll() {
  ccad::Schematic sch;
  sch.symbols.push_back(ccad::SchSymbol{.reference = "R5", .position = {ccad::millimeters(10), ccad::millimeters(10)}});
  sch.symbols.push_back(ccad::SchSymbol{.reference = "R10", .position = {ccad::millimeters(20), ccad::millimeters(10)}});
  
  ccad::AnnotateOptions opts;
  opts.algo = ccad::AnnotateAlgo::ResetAll;
  ccad::annotateSchematic(sch, opts);
  
  require(sch.symbols[0].reference == "R1", "R5 should reset and become R1");
  require(sch.symbols[1].reference == "R2", "R10 should reset and become R2");
}

void testSortOrder() {
  ccad::Schematic sch;
  // U? at X=50, Y=10
  sch.symbols.push_back(ccad::SchSymbol{.reference = "U?", .position = {ccad::millimeters(50), ccad::millimeters(10)}});
  // U? at X=10, Y=50
  sch.symbols.push_back(ccad::SchSymbol{.reference = "U?", .position = {ccad::millimeters(10), ccad::millimeters(50)}});
  
  // Sort by X: (10, 50) comes before (50, 10) because X is primary
  ccad::AnnotateOptions opts_x;
  opts_x.order = ccad::AnnotateOrder::SortX;
  ccad::annotateSchematic(sch, opts_x);
  
  require(sch.symbols[0].reference == "U2", "U? at 50,10 should be U2");
  require(sch.symbols[1].reference == "U1", "U? at 10,50 should be U1");

  // Reset for Y sort
  sch.symbols[0].reference = "U?";
  sch.symbols[1].reference = "U?";
  
  // Sort by Y: (50, 10) comes before (10, 50) because Y is primary
  ccad::AnnotateOptions opts_y;
  opts_y.order = ccad::AnnotateOrder::SortY;
  ccad::annotateSchematic(sch, opts_y);
  
  require(sch.symbols[0].reference == "U1", "U? at 50,10 should be U1");
  require(sch.symbols[1].reference == "U2", "U? at 10,50 should be U2");
}

void testProjectAnnotation() {
  ccad::Project proj;
  proj.schematics.push_back(ccad::Schematic());
  proj.schematics.push_back(ccad::Schematic());
  
  proj.schematics[0].symbols.push_back(ccad::SchSymbol{.reference = "C?", .position = {ccad::millimeters(10), ccad::millimeters(10)}});
  proj.schematics[1].symbols.push_back(ccad::SchSymbol{.reference = "C?", .position = {ccad::millimeters(20), ccad::millimeters(10)}});
  
  ccad::AnnotateOptions opts;
  ccad::annotateProject(proj, opts);
  
  require(proj.schematics[0].symbols[0].reference == "C1", "First sheet gets C1");
  require(proj.schematics[1].symbols[0].reference == "C2", "Second sheet gets C2");
}

int main() {
  std::cout << "test_annotate starting\n";
  testKeepExisting();
  testResetAll();
  testSortOrder();
  testProjectAnnotation();
  std::cout << "test_annotate passed\n";
  return 0;
}
