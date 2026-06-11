#include "ccad_core/placement.hpp"
#include "ccad_core/canvas.hpp"
#include "ccad_core/layers.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

void test_basic_placement() {
  ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.schematics.push_back(ccad::Schematic{});
  project.boards.clear(); project.boards.push_back(ccad::Board{});
  project.boards[0].outline.origin = {ccad::millimeters(0)};
  project.boards[0].outline.origin.y = ccad::millimeters(0);
  project.boards[0].outline.size = {ccad::millimeters(100), ccad::millimeters(100)};
  project.boards[0].layers.push_back({.id = "F.Cu", .kind = "copper"});

  ccad::Footprint fp;
  fp.pads.push_back({
      .number = "1",
      .position = {ccad::millimeters(0), ccad::millimeters(0)},
      .rotation_degrees = 0,
      .size = {ccad::millimeters(1), ccad::millimeters(1)},
  });

  ccad::placeFootprint(project, fp, "U1", {ccad::millimeters(50), ccad::millimeters(50)}, 0, "F.Cu");

  if (project.boards[0].pads.size() != 1) {
    throw std::runtime_error("Pad count should be 1");
  }
  if (project.boards[0].pads[0].position.x.nanometers != ccad::millimeters(50).nanometers) {
    throw std::runtime_error("Pad X should be 50mm");
  }
}

void test_footprint_placement_does_not_require_schematic() {
  ccad::Project project;
  project.boards.push_back(ccad::Board{});
  project.boards[0].outline.origin = {ccad::millimeters(0), ccad::millimeters(0)};
  project.boards[0].outline.size = {ccad::millimeters(100), ccad::millimeters(100)};
  project.boards[0].layers.push_back({.id = "F.Cu", .kind = "copper"});

  ccad::Footprint fp;
  fp.pads.push_back({
      .number = "1",
      .position = {ccad::millimeters(0), ccad::millimeters(0)},
      .rotation_degrees = 0,
      .size = {ccad::millimeters(1), ccad::millimeters(1)},
      .layers = {"F.Cu"},
  });

  ccad::placeFootprint(project, fp, "J1",
                       {ccad::millimeters(25), ccad::millimeters(25)}, 0, "F.Cu");

  if (!project.schematics.empty()) {
    throw std::runtime_error("Footprint placement should not create a schematic document");
  }
  if (project.boards[0].pads.size() != 1 || project.boards[0].pads[0].id != "J1.1") {
    throw std::runtime_error("Board-only footprint placement should create the pad");
  }
  if (!project.boards[0].pads[0].net_id.empty()) {
    throw std::runtime_error("Board-only footprint placement should leave pad net unassigned");
  }
}

void test_placement_expands_kicad_wildcard_pad_layers() {
  ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.boards.clear();
  project.boards.push_back(ccad::Board{});
  project.boards[0].outline.origin = {ccad::millimeters(0), ccad::millimeters(0)};
  project.boards[0].outline.size = {ccad::millimeters(100), ccad::millimeters(100)};
  project.boards[0].layers = ccad::standardKiCadPcbLayers();

  ccad::Footprint fp;
  fp.pads.push_back({
      .number = "1",
      .type = "thru_hole",
      .shape = "circle",
      .position = {ccad::millimeters(0), ccad::millimeters(0)},
      .rotation_degrees = 0,
      .size = {ccad::millimeters(1.5), ccad::millimeters(1.5)},
      .drill = ccad::millimeters(0.8),
      .layers = {"*.Cu", "*.Mask", "F.SilkS"},
  });

  ccad::placeFootprint(project, fp, "J1", {ccad::millimeters(20), ccad::millimeters(20)}, 0,
                       "F.Cu");

  const std::vector<std::string>& placed_layers = project.boards[0].pads.at(0).layers;
  if (std::find(placed_layers.begin(), placed_layers.end(), "*.Cu") != placed_layers.end()) {
    throw std::runtime_error("Placed pad should not retain raw *.Cu wildcard");
  }
  if (std::find(placed_layers.begin(), placed_layers.end(), "*.Mask") != placed_layers.end()) {
    throw std::runtime_error("Placed pad should not retain raw *.Mask wildcard");
  }
  if (std::find(placed_layers.begin(), placed_layers.end(), "F.Cu") == placed_layers.end() ||
      std::find(placed_layers.begin(), placed_layers.end(), "B.Cu") == placed_layers.end()) {
    throw std::runtime_error("Placed through-hole pad should include front and back copper");
  }
  if (std::find(placed_layers.begin(), placed_layers.end(), "F.Mask") == placed_layers.end() ||
      std::find(placed_layers.begin(), placed_layers.end(), "B.Mask") == placed_layers.end()) {
    throw std::runtime_error("Placed pad should include front and back solder mask openings");
  }
  if (std::find(placed_layers.begin(), placed_layers.end(), "F.SilkS") == placed_layers.end()) {
    throw std::runtime_error("Explicit footprint layer should remain on front placement");
  }
}

void test_bottom_placement_flips_explicit_front_back_layers_before_expansion() {
  ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.boards.clear();
  project.boards.push_back(ccad::Board{});
  project.boards[0].outline.origin = {ccad::millimeters(0), ccad::millimeters(0)};
  project.boards[0].outline.size = {ccad::millimeters(100), ccad::millimeters(100)};
  project.boards[0].layers = ccad::standardKiCadPcbLayers();

  ccad::Footprint fp;
  fp.pads.push_back({
      .number = "1",
      .type = "smd",
      .shape = "rect",
      .position = {ccad::millimeters(0), ccad::millimeters(0)},
      .rotation_degrees = 0,
      .size = {ccad::millimeters(1), ccad::millimeters(1)},
      .layers = {"F.Cu", "F.Paste", "F.Mask"},
  });

  ccad::placeFootprint(project, fp, "U2", {ccad::millimeters(30), ccad::millimeters(30)}, 0,
                       "B.Cu");

  const std::vector<std::string>& placed_layers = project.boards[0].pads.at(0).layers;
  if (std::find(placed_layers.begin(), placed_layers.end(), "B.Cu") == placed_layers.end() ||
      std::find(placed_layers.begin(), placed_layers.end(), "B.Paste") == placed_layers.end() ||
      std::find(placed_layers.begin(), placed_layers.end(), "B.Mask") == placed_layers.end()) {
    throw std::runtime_error("Bottom placement should flip explicit front layers to back layers");
  }
  if (std::find(placed_layers.begin(), placed_layers.end(), "F.Cu") != placed_layers.end() ||
      std::find(placed_layers.begin(), placed_layers.end(), "F.Paste") != placed_layers.end() ||
      std::find(placed_layers.begin(), placed_layers.end(), "F.Mask") != placed_layers.end()) {
    throw std::runtime_error("Bottom placement should not retain original front-only layers");
  }
}

void test_symbol_placement_retains_visible_primitives_after_reload() {
  ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.schematics.push_back(ccad::Schematic{});
  project.id = "symbol-placement";

  ccad::Symbol symbol;
  symbol.name = "D";
  symbol.rectangles.push_back(ccad::SymbolRectangle{
      .start = {ccad::millimeters(-1.0), ccad::millimeters(-1.0)},
      .end = {ccad::millimeters(1.0), ccad::millimeters(1.0)},
      .stroke_width = ccad::millimeters(0.12),
      .fill_type = "none"});
  symbol.lines.push_back(ccad::SymbolLine{
      .start = {ccad::millimeters(-1.0), ccad::millimeters(0.0)},
      .end = {ccad::millimeters(1.0), ccad::millimeters(0.0)},
      .stroke_width = ccad::millimeters(0.12)});
  symbol.pins.push_back(ccad::SymbolPin{
      .name = "A",
      .number = "1",
      .electrical_type = "passive",
      .graphical_style = "line",
      .position = {ccad::millimeters(-3.54), ccad::millimeters(0.0)},
      .rotation_degrees = 0.0,
      .length = ccad::millimeters(2.54)});

  ccad::placeComponent(project, symbol, "D1",
                       {ccad::millimeters(20.0), ccad::millimeters(15.0)}, 0.0);

  const std::string json = ccad::dumpProjectJson(project);
  if (json.find("\"symbol\"") == std::string::npos) {
    throw std::runtime_error("placed component should serialize a symbol snapshot");
  }

  const ccad::Project reloaded = ccad::loadProjectJson(json);
  const ccad::CanvasScene scene = (reloaded.schematics.empty() ? ccad::buildSchematicScene(ccad::Schematic{}) : ccad::buildSchematicScene(reloaded.schematics[0]));

  const auto line_has = [&scene](const std::string& needle) {
    return std::any_of(scene.lines.begin(), scene.lines.end(), [&](const ccad::CanvasLine& line) {
      return line.id.find(needle) != std::string::npos;
    });
  };
  const bool has_body_line = line_has("D1.line");
  const bool has_pin_lead = line_has("D1.pin_1");

  if (!has_body_line) {
    throw std::runtime_error("schematic scene should contain reloaded symbol body graphics");
  }
  if (!has_pin_lead) {
    throw std::runtime_error("schematic scene should contain reloaded symbol pin leads");
  }
  if (scene.polygons.empty()) {
    throw std::runtime_error("schematic scene should render symbol rectangles as primitives");
  }
}

void test_symbol_placement_creates_schematic_document() {
  ccad::Project project;
  project.id = "symbol-placement-empty";

  ccad::Symbol symbol;
  symbol.name = "TP";
  symbol.pins.push_back(ccad::SymbolPin{
      .name = "1",
      .number = "1",
      .electrical_type = "passive",
      .graphical_style = "line",
      .position = {ccad::millimeters(0.0), ccad::millimeters(0.0)},
      .rotation_degrees = 0.0,
      .length = ccad::millimeters(2.54)});

  ccad::placeComponent(project, symbol, "TP1",
                       {ccad::millimeters(10.0), ccad::millimeters(10.0)}, 0.0);

  if (project.schematics.size() != 1) {
    throw std::runtime_error("Symbol placement should create one primary schematic document");
  }
  if (project.schematics[0].components.size() != 1 ||
      project.schematics[0].components[0].id != "TP1") {
    throw std::runtime_error("Symbol placement should write the component into the schematic");
  }
}

int main() {
  try {
    test_basic_placement();
    test_footprint_placement_does_not_require_schematic();
    test_placement_expands_kicad_wildcard_pad_layers();
    test_bottom_placement_flips_explicit_front_back_layers_before_expansion();
    test_symbol_placement_retains_visible_primitives_after_reload();
    test_symbol_placement_creates_schematic_document();
    std::cout << "All tests passed!\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed: " << e.what() << "\n";
    return 1;
  }
}
