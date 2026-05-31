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

  std::string exported = exportKiCadFootprint(fp);

  std::string expected = 
      "(footprint \"Resistor_SMD\"\n"
      "  (pad \"1\" smd rect (at -1.000000 0.000000 90.000000) (size 1.000000 1.500000) (layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
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
