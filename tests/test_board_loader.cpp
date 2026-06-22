#include "ccad_core/board_loader.hpp"

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "test failure: " << message << '\n';
    std::exit(1);
  }
}

ccad::Project fixtureProject() {
  ccad::Project project;
  project.schema_version = 2;
  project.id = "proj-loader";
  project.name = "loader";

  ccad::Board board;
  board.outline = ccad::Rect{.origin = {.x = ccad::nanometers(0),
                                        .y = ccad::nanometers(0)},
                             .size = {.width = ccad::millimeters(42),
                                      .height = ccad::millimeters(28)}};
  board.layers.push_back(
      ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true});
  board.layers.push_back(
      ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true});
  board.layers.push_back(ccad::Layer{
      .id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen", .visible = false});
  board.pads.push_back(ccad::Pad{.id = "P1",
                                 .component_id = "U1",
                                 .pin_name = "1",
                                 .net_id = "N1",
                                 .type = "smd",
                                 .position = {.x = ccad::millimeters(5),
                                              .y = ccad::millimeters(6)},
                                 .padstack = ccad::Padstack{
                                    .layer_set = {"F.Cu"},
                                    .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = {.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.0)}}}}}
                                 }});
  board.vias.push_back(ccad::Via{.id = "V1",
                                 .net_id = "N1",
                                 .position = {.x = ccad::millimeters(8),
                                              .y = ccad::millimeters(8)},
                                 .diameter = ccad::millimeters(0.8),
                                 .drill = ccad::millimeters(0.4)});
  board.tracks.push_back(ccad::TrackSegment{.id = "T1",
                                            .net_id = "N1",
                                            .layer_id = "F.Cu",
                                            .start = {.x = ccad::millimeters(5),
                                                      .y = ccad::millimeters(6)},
                                            .end = {.x = ccad::millimeters(8),
                                                    .y = ccad::millimeters(8)},
                                            .width = ccad::millimeters(0.25)});
  project.boards.push_back(board);

  ccad::Schematic schematic;
  schematic.id = "sch-main";
  schematic.name = "Main";
  schematic.nets.push_back(ccad::Net{.id = "N1", .members = {{"U1", "1"}}});
  project.schematics.push_back(schematic);
  return project;
}

}  // namespace

int main() {
  const ccad::Project project = fixtureProject();
  const ccad::BoardLoadState initialized = ccad::summarizeLoadedBoard(project);

  require(initialized.kicad_class == "BOARD_LOADER",
          "load state exposes KiCad board loader class");
  require(initialized.source_format == "CCAD_JSON", "load state reports CCad JSON source format");
  require(initialized.loaded, "load state reports project loaded");
  require(initialized.board_attached, "load state reports board attached");
  require(initialized.initialize_after_load, "load state defaults to initialized");
  require(initialized.design_rules_ready, "load state reports design rules ready");
  require(initialized.drc_ready, "load state reports DRC ready");
  require(initialized.connectivity_ready, "load state reports connectivity ready");
  require(initialized.netlist_ready, "load state reports netlist ready");
  require(initialized.user_units_ready, "load state reports user units ready");
  require(initialized.persistent_drc_engine == false,
          "load state does not fake a persistent KiCad DRC engine");
  require(initialized.board_count == 1, "load state reports board count");
  require(initialized.schematic_count == 1, "load state reports schematic count");
  require(initialized.layer_count == 3, "load state reports layer count");
  require(initialized.copper_layer_count == 2, "load state reports copper layer count");
  require(initialized.hidden_layer_count == 1, "load state reports hidden layer count");
  require(initialized.pad_count == 1, "load state reports pad count");
  require(initialized.via_count == 1, "load state reports via count");
  require(initialized.track_count == 1, "load state reports track count");
  require(initialized.physical_object_count == 3, "load state reports physical object count");
  require(initialized.board_net_count == 1, "load state reports board net count");
  require(initialized.pending_kicad_loader_steps.size() >= 4,
          "load state reports remaining KiCad loader parity steps");

  ccad::BoardLoadOptions without_init;
  without_init.initialize_after_load = false;
  const ccad::BoardLoadState raw = ccad::summarizeLoadedBoard(project, without_init);
  require(!raw.board_attached, "raw load state mirrors KiCad load without project attach");
  require(!raw.drc_ready, "raw load state reports DRC unavailable without initialization");
  require(!raw.connectivity_ready,
          "raw load state reports connectivity unavailable without initialization");

  return 0;
}
