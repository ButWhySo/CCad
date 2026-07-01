#include "ccad_core/footprint_losslessness.hpp"
#include "test_support.hpp"

#include <string>

int main() {
  // 1. Identical footprint comparison
  ccad::Footprint f1;
  f1.name = "R_0805";
  f1.exclude_from_bom = false;

  ccad::FootprintPad pad1;
  pad1.number = "1";
  pad1.type = "smd";
  pad1.shape = "roundrect";
  pad1.position = {ccad::millimeters(-0.95), ccad::millimeters(0)};
  pad1.rotation_degrees = 0.0;
  pad1.size = {ccad::millimeters(1.0), ccad::millimeters(1.45)};
  pad1.roundrect_rratio = 0.25;
  pad1.layers = {"F.Cu", "F.Paste", "F.Mask"};
  f1.pads.push_back(pad1);

  ccad::Footprint f2 = f1;

  auto diags1 = ccad::verifyFootprintLosslessness(f1, f2);
  require(diags1.empty(), "identical footprints produce no diagnostics");

  // 2. Mismatched name
  f2.name = "R_0603";
  auto diags2 = ccad::verifyFootprintLosslessness(f1, f2);
  require(diags2.size() == 1, "name mismatch produces 1 diagnostic");
  require(diags2.at(0).field == "name", "field is name");

  // Reset name
  f2.name = f1.name;

  // 3. Mismatched pad count
  f2.pads.clear();
  auto diags3 = ccad::verifyFootprintLosslessness(f1, f2);
  require(diags3.size() == 1, "pad count mismatch produces diagnostic");
  require(diags3.at(0).field == "pads", "field is pads");

  // Reset pads
  f2.pads = f1.pads;

  // 4. Mismatched pad shape
  f2.pads.at(0).shape = "rect";
  auto diags4 = ccad::verifyFootprintLosslessness(f1, f2);
  require(diags4.size() == 1, "pad shape mismatch produces diagnostic");
  require(diags4.at(0).field == "pad.shape", "field is pad.shape");

  // Reset pad shape
  f2.pads.at(0).shape = f1.pads.at(0).shape;

  // 5. Oval drill checks
  f1.pads.at(0).drill = ccad::millimeters(0.8);
  f1.pads.at(0).drill_height = ccad::millimeters(1.2);
  f1.pads.at(0).drill_shape = "oval";

  f2.pads = f1.pads;

  auto diags5 = ccad::verifyFootprintLosslessness(f1, f2);
  require(diags5.empty(), "identical oval drill footprints produce no diagnostics");

  // drill height mismatch
  f2.pads.at(0).drill_height = ccad::millimeters(1.4);
  auto diags6 = ccad::verifyFootprintLosslessness(f1, f2);
  require(diags6.size() == 1, "drill height mismatch produces diagnostic");
  require(diags6.at(0).field == "pad.drill_height", "field is pad.drill_height");

  return 0;
}
