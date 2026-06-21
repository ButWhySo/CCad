#include "ccad_core/cross_probing.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

ccad::Project fixtureProject() {
  ccad::Project project;
  project.id = "proj-cross-probe";
  project.name = "cross-probe";

  ccad::Board board;
  board.outline = ccad::Rect{
      .origin = {.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
      .size = {.width = ccad::millimeters(40), .height = ccad::millimeters(30)}};
  board.layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper"},
                  ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper"}};
  board.footprints.push_back(ccad::BoardFootprint{
      .reference = "U1",
      .value = "MCU",
      .footprint_name = "Package_SO:SOIC-8",
      .layer_id = "F.Cu",
      .position = {.x = ccad::millimeters(10), .y = ccad::millimeters(10)}});
  board.pads.push_back(ccad::Pad{.id = "U1.1",
                                 .component_id = "U1",
                                 .pin_name = "1",
                                 .net_id = "N1",
                                 .layers = {"F.Cu"},
                                 .type = "smd",
                                 .shape = "rect",
                                 .position = {.x = ccad::millimeters(9),
                                              .y = ccad::millimeters(10)},
                                 .size = {.width = ccad::millimeters(1),
                                          .height = ccad::millimeters(1)}});
  board.pads.push_back(ccad::Pad{.id = "U1.2",
                                 .component_id = "U1",
                                 .pin_name = "2",
                                 .net_id = "N2",
                                 .layers = {"F.Cu"},
                                 .type = "smd",
                                 .shape = "rect",
                                 .position = {.x = ccad::millimeters(11),
                                              .y = ccad::millimeters(10)},
                                 .size = {.width = ccad::millimeters(1),
                                          .height = ccad::millimeters(1)}});
  board.vias.push_back(ccad::Via{.id = "V1",
                                 .net_id = "N1",
                                 .position = {.x = ccad::millimeters(14),
                                              .y = ccad::millimeters(10)},
                                 .diameter = ccad::millimeters(0.8),
                                 .drill = ccad::millimeters(0.4)});
  board.tracks.push_back(ccad::TrackSegment{.id = "T1",
                                            .net_id = "N1",
                                            .layer_id = "F.Cu",
                                            .start = {.x = ccad::millimeters(9),
                                                      .y = ccad::millimeters(10)},
                                            .end = {.x = ccad::millimeters(14),
                                                    .y = ccad::millimeters(10)},
                                            .width = ccad::millimeters(0.25)});
  project.boards.push_back(board);

  ccad::Schematic schematic;
  schematic.id = "root";
  schematic.name = "Root";
  schematic.components.push_back(ccad::Component{.id = "U1", .part = "MCU"});
  schematic.nets.push_back(
      ccad::Net{.id = "N1", .members = {ccad::NetMember{.component_id = "U1",
                                                        .pin_name = "1"}}});
  project.schematics.push_back(schematic);

  return project;
}

void test_format_packets_match_kicad_shape() {
  require(ccad::formatCrossProbeClear() == "$CLEAR: \"HIGHLIGHTED\"",
          "clear packet matches KiCad highlighted clear shape");
  require(ccad::formatCrossProbeNet("N1") == "$NET: \"N1\"",
          "net packet matches KiCad shape");
  require(ccad::formatCrossProbePart("U1") == "$PART: \"U1\"",
          "part packet matches KiCad shape");
  require(ccad::formatCrossProbePad("U1", "1") == "$PART: \"U1\" $PAD: \"1\"",
          "pad packet matches KiCad shape");
}

void test_net_packet_resolves_board_and_schematic_targets() {
  const ccad::Project project = fixtureProject();
  const ccad::CrossProbeReport report = ccad::resolveCrossProbePacket(project, "$NET: \"N1\"");

  require(report.kicad_source == "pcbnew/cross-probing.cpp", "report records KiCad source");
  require(report.packet_kind == "net", "packet kind is net");
  require(report.requested_nets.size() == 1 && report.requested_nets.front() == "N1",
          "report records requested net");
  require(report.targets.size() == 4, "net resolves schematic net plus pad, via, and track");
  require(report.targets.at(0).type == "schematic_net", "schematic target is first");
  require(report.targets.at(1).id == "U1.1", "pad target is present");
  require(report.targets.at(2).id == "V1", "via target is present");
  require(report.targets.at(3).id == "T1", "track target is present");
  require(!report.pending_kicad_features.empty(), "report documents pending KiCad IPC features");
}

void test_part_and_select_packets_resolve_ordered_items() {
  const ccad::Project project = fixtureProject();
  const ccad::CrossProbeReport pad_report =
      ccad::resolveCrossProbePacket(project, "$PART: \"U1\" $PAD: \"1\"");
  require(pad_report.packet_kind == "pad", "part plus pad packet kind is pad");
  require(pad_report.targets.size() == 1, "pad packet resolves one pad");
  require(pad_report.targets.front().id == "U1.1", "pad packet resolves requested pad");

  const ccad::CrossProbeReport select_report =
      ccad::resolveCrossProbePacket(project, "$SELECT: 1,FU1,PU1/1");
  require(select_report.packet_kind == "select", "select packet kind is select");
  require(select_report.select_connections, "select mode requests connection sync");
  require(select_report.targets.size() == 3,
          "select resolves board footprint, schematic component, and pad");
  require(select_report.targets.at(0).id == "U1", "footprint remains first selected target");
  require(select_report.targets.at(0).focus, "first select target is focus when mode is one");
  require(select_report.targets.at(2).id == "U1.1", "pad remains ordered after footprint");
}

void test_clear_packet_is_explicit() {
  const ccad::Project project = fixtureProject();
  const ccad::CrossProbeReport report =
      ccad::resolveCrossProbePacket(project, "$CLEAR: \"HIGHLIGHTED\"");
  require(report.packet_kind == "clear", "clear packet kind is clear");
  require(report.clear_highlight, "clear packet asks caller to clear highlight");
  require(report.targets.empty(), "clear packet has no physical targets");
}

}  // namespace

int main() {
  try {
    test_format_packets_match_kicad_shape();
    test_net_packet_resolves_board_and_schematic_targets();
    test_part_and_select_packets_resolve_ordered_items();
    test_clear_packet_is_explicit();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
