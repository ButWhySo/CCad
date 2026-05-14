#include "ccad_core/kicad_footprint_import.hpp"
#include "test_support.hpp"

#include <stdexcept>
#include <string>

int main() {
  const std::string source =
      "(footprint \"R_0805_2012Metric\"\n"
      "  (version 20240101)\n"
      "  (generator \"ccad-test\")\n"
      "  (pad \"1\" smd roundrect (at -0.95 0 0) (size 1.0 1.45) "
      "(layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      "  (pad \"2\" smd roundrect (at 0.95 0 0) (size 1.0 1.45) "
      "(layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      ")\n";

  const ccad::Footprint footprint = ccad::importKiCadFootprint(source);
  require(footprint.name == "R_0805_2012Metric", "footprint name imports");
  require(footprint.pads.size() == 2, "pads import");
  require(footprint.pads.at(0).number == "1", "pad number imports");
  require(footprint.pads.at(0).type == "smd", "pad type imports");
  require(footprint.pads.at(0).shape == "roundrect", "pad shape imports");
  require(footprint.pads.at(0).position.x.nanometers == -950000, "pad x imports");
  require(footprint.pads.at(0).position.y.nanometers == 0, "pad y imports");
  require(footprint.pads.at(0).rotation_degrees == 0.0, "pad rotation imports");
  require(footprint.pads.at(0).size.width.nanometers == 1000000, "pad width imports");
  require(footprint.pads.at(0).size.height.nanometers == 1450000, "pad height imports");
  require(footprint.pads.at(0).layers.size() == 3, "pad layers import");
  require(footprint.pads.at(0).layers.at(0) == "F.Cu", "first pad layer imports");

  const std::string json = ccad::dumpFootprintJson(footprint);
  require(json.find("\"name\": \"R_0805_2012Metric\"") != std::string::npos,
          "footprint json includes name");
  require(json.find("\"width_nm\": 1000000") != std::string::npos,
          "footprint json includes pad width");
  require(ccad::dumpFootprintJson(footprint) == json, "footprint json deterministic");

  bool rejected_root = false;
  try {
    (void)ccad::importKiCadFootprint("(symbol \"R\")");
  } catch (const std::runtime_error&) {
    rejected_root = true;
  }
  require(rejected_root, "importer rejects non-footprint root");

  bool rejected_malformed = false;
  try {
    (void)ccad::importKiCadFootprint("(footprint \"bad\"");
  } catch (const std::runtime_error&) {
    rejected_malformed = true;
  }
  require(rejected_malformed, "importer rejects malformed sexpr");
}
