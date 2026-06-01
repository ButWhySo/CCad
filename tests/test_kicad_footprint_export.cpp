#include "ccad_core/kicad_footprint_export.hpp"
#include "ccad_core/kicad_footprint_import.hpp"

#include <iostream>

using namespace ccad;

void assertEqual(const std::string& expected, const std::string& actual, const std::string& label) {
  if (expected != actual) {
    std::cerr << "FAIL " << label << "\n  expected: " << expected << "\n  actual:   " << actual
              << '\n';
    std::exit(1);
  }
}

void testFootprintExport() {
  Footprint fp;
  fp.name = "Resistor_SMD";
  FootprintPad p1;
  p1.number = "1";
  p1.type = "smd";
  p1.shape = "rect";
  p1.position = Point{nanometers(-1000000), nanometers(0)};
  p1.size = Size{nanometers(1000000), nanometers(1500000)};
  p1.rotation_degrees = 90.0;
  p1.layers = {"F.Cu", "F.Paste", "F.Mask"};
  fp.pads.push_back(p1);
  FootprintPad p2;
  p2.number = "2";
  p2.type = "smd";
  p2.shape = "roundrect";
  p2.position = Point{nanometers(1000000), nanometers(0)};
  p2.size = Size{nanometers(1000000), nanometers(1500000)};
  p2.roundrect_rratio = 0.25;
  p2.layers = {"F.Cu", "F.Paste", "F.Mask"};
  fp.pads.push_back(p2);
  FootprintPad p3;
  p3.number = "3";
  p3.type = "smd";
  p3.shape = "chamfered_rect";
  p3.position = Point{nanometers(3000000), nanometers(0)};
  p3.size = Size{nanometers(1200000), nanometers(1500000)};
  p3.chamfer_ratio = 0.20;
  p3.layers = {"F.Cu", "F.Paste", "F.Mask"};
  fp.pads.push_back(p3);

  std::string exported = exportKiCadFootprint(fp);

  std::string expected = 
      "(footprint \"Resistor_SMD\"\n"
      "  (pad \"1\" smd rect (at -1.000000 0.000000 90.000000) (size 1.000000 1.500000) (layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      "  (pad \"2\" smd roundrect (at 1.000000 0.000000) (size 1.000000 1.500000) (roundrect_rratio 0.250000) (layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      "  (pad \"3\" smd chamfered_rect (at 3.000000 0.000000) (size 1.200000 1.500000) (chamfer_ratio 0.200000) (layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      ")\n";

  assertEqual(expected, exported, "testFootprintExport");
}

int main() {
  try {
    testFootprintExport();
    std::cout << "PASS kicad footprint export\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
