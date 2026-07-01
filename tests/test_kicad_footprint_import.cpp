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
      "(roundrect_rratio 0.25) (layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      "  (pad \"2\" smd roundrect (at 0.95 0 0) (size 1.0 1.45) "
      "(layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      "  (pad \"3\" thru_hole trapezoid (at 0 2 90) (size 2.0 1.2) "
      "(drill 0.7) (layers \"*.Cu\" \"*.Mask\"))\n"
      "  (pad \"4\" smd chamfered_rect (at 3 2) (size 2.0 1.2) "
      "(chamfer_ratio 0.20) (layers \"F.Cu\" \"F.Paste\" \"F.Mask\"))\n"
      ")\n";

  const ccad::Footprint footprint = ccad::importKiCadFootprint(source);
  require(footprint.name == "R_0805_2012Metric", "footprint name imports");
  require(footprint.pads.size() == 4, "pads import");
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
  require(footprint.pads.at(0).roundrect_rratio.has_value(), "roundrect ratio imports");
  require(*footprint.pads.at(0).roundrect_rratio == 0.25, "roundrect ratio value imports");
  require(footprint.pads.at(2).shape == "trapezoid", "trapezoid pad shape imports");
  require(footprint.pads.at(2).drill.has_value(), "through-hole drill imports");
  require(footprint.pads.at(3).shape == "chamfered_rect", "chamfered pad shape imports");
  require(footprint.pads.at(3).chamfer_ratio.has_value(), "chamfer ratio imports");
  require(*footprint.pads.at(3).chamfer_ratio == 0.20, "chamfer ratio value imports");

  const std::string json = ccad::dumpFootprintJson(footprint);
  require(json.find("\"name\": \"R_0805_2012Metric\"") != std::string::npos,
          "footprint json includes name");
  require(json.find("\"width_nm\": 1000000") != std::string::npos,
          "footprint json includes pad width");
  require(ccad::dumpFootprintJson(footprint) == json, "footprint json deterministic");

  const ccad::Footprint loaded = ccad::loadFootprintJson(json);
  require(loaded.name == footprint.name, "footprint json loads name");
  require(loaded.pads.size() == 4, "footprint json loads all pads");
  require(loaded.pads.at(1).number == "2", "footprint json loads pad number");
  require(loaded.pads.at(1).position.x.nanometers == 950000, "footprint json loads pad x");
  require(loaded.pads.at(1).size.height.nanometers == 1450000,
          "footprint json loads pad height");
  require(loaded.pads.at(1).layers.at(2) == "F.Mask", "footprint json loads layers");
  require(loaded.pads.at(0).roundrect_rratio.has_value(), "footprint json loads roundrect ratio");
  require(loaded.pads.at(3).chamfer_ratio.has_value(), "footprint json loads chamfer ratio");

  const std::string bom_source = std::string("\xEF\xBB\xBF") + source;
  require(ccad::importKiCadFootprint(bom_source).name == "R_0805_2012Metric",
          "importer accepts utf8 bom");

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

  // Oval drill verification
  const std::string oval_source =
      "(footprint \"Oval_Drill_Footprint\"\n"
      "  (version 20240101)\n"
      "  (generator \"ccad-test\")\n"
      "  (pad \"1\" thru_hole oval (at 0 0) (size 1.6 3.5) "
      "(drill oval 0.7 2.5) (layers \"*.Cu\" \"*.Mask\"))\n"
      ")\n";

  const ccad::Footprint footprint_oval = ccad::importKiCadFootprint(oval_source);
  require(footprint_oval.pads.size() == 1, "oval drill footprint imports 1 pad");
  const auto& oval_pad = footprint_oval.pads.at(0);
  require(oval_pad.drill.has_value(), "oval drill has value");
  require(oval_pad.drill->nanometers == 700000, "oval drill width imports");
  require(oval_pad.drill_height.has_value(), "oval drill height has value");
  require(oval_pad.drill_height->nanometers == 2500000, "oval drill height imports");
  require(oval_pad.drill_shape.has_value() && *oval_pad.drill_shape == "oval", "oval drill shape imports");

  const std::string json_oval = ccad::dumpFootprintJson(footprint_oval);
  require(json_oval.find("\"drill_height_nm\": 2500000") != std::string::npos, "json outputs drill_height_nm");
  require(json_oval.find("\"drill_shape\": \"oval\"") != std::string::npos, "json outputs drill_shape");

  const ccad::Footprint loaded_oval = ccad::loadFootprintJson(json_oval);
  require(loaded_oval.pads.at(0).drill_height.has_value() && loaded_oval.pads.at(0).drill_height->nanometers == 2500000, "json load loads oval drill height");
  require(loaded_oval.pads.at(0).drill_shape.has_value() && *loaded_oval.pads.at(0).drill_shape == "oval", "json load loads oval drill shape");
}
