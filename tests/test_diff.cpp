#include "ccad_core/diff.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

#include <string>
#include <vector>

namespace {

ccad::Project baseProject() {
  ccad::Project project;
  project.id = "proj-diff";
  project.name = "diff";
  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].symbols.push_back(ccad::SchSymbol{
      .id = "U1",
      .lib_id = "MCU",
      .pins = {ccad::SchPin{.name = "VDD", .number = "", .electrical_type = ccad::ElectricalPinType::PowerIn}},
  });
  project.schematics[0].nets.push_back(ccad::Net{
      .id = "N_3V3",
      .members = {ccad::NetMember{.component_id = "U1", .pin_name = "VDD"}},
  });
  project.schematics[0].constraints.push_back(ccad::Constraint{
      .id = "C_3V3",
      .kind = "voltage",
      .target = "N_3V3",
      .value = "3.3V",
  });
  project.boards.push_back(ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper",
                             .visible = true}},
      .placement_regions = {ccad::PlacementRegion{
          .id = "PR1",
          .kind = "component",
          .area = ccad::Rect{
              .origin = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(5)},
              .size = ccad::Size{.width = ccad::millimeters(6),
                                  .height = ccad::millimeters(4)}}}},
      .keepouts = {ccad::Keepout{
          .id = "K1",
          .kind = "placement",
          .area = ccad::Rect{
              .origin = ccad::Point{.x = ccad::millimeters(20), .y = ccad::millimeters(10)},
              .size = ccad::Size{.width = ccad::millimeters(3),
                                  .height = ccad::millimeters(2)}}}},
      .pads = {ccad::Pad{.id = "P1",
                         .component_id = "U1",
                         .pin_name = "VDD",
                         .net_id = "N_3V3",
                         .position = ccad::Point{.x = ccad::millimeters(5),
                                                 .y = ccad::millimeters(6)},
                         .padstack = ccad::Padstack{
                            .layer_set = {"F.Cu"},
                            .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
                         }}},
      .vias = {ccad::Via{.id = "V1",
                         .net_id = "N_3V3",
                         .position = ccad::Point{.x = ccad::millimeters(8),
                                                 .y = ccad::millimeters(9)},
                         .diameter = ccad::millimeters(0.8),
                         .drill = ccad::millimeters(0.4)}},
      .tracks = {ccad::TrackSegment{.id = "T1",
                                    .net_id = "N_3V3",
                                    .layer_id = "F.Cu",
                                    .start = ccad::Point{.x = ccad::millimeters(5),
                                                         .y = ccad::millimeters(6)},
                                    .end = ccad::Point{.x = ccad::millimeters(8),
                                                       .y = ccad::millimeters(9)},
                                    .width = ccad::millimeters(0.25)}},
      .graphics = {},
      .texts = {},
      .zones = {},
      .route_requests = {},
  });
  return project;
}

bool hasEntry(const ccad::ProjectDiff& diff, const std::string& change,
              const std::string& object_type, const std::string& object_id) {
  for (const ccad::DiffEntry& entry : diff.entries) {
    if (entry.change == change && entry.object_type == object_type && entry.object_id == object_id) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  const ccad::Project clean_before = baseProject();
  const ccad::Project clean_after = baseProject();
  const ccad::ProjectDiff clean = ccad::diffProjects(clean_before, clean_after);
  require(clean.entries.empty(), "identical projects have no diff entries");
  require(clean.added_count == 0, "clean diff added count zero");
  require(clean.removed_count == 0, "clean diff removed count zero");
  require(clean.changed_count == 0, "clean diff changed count zero");

  ccad::Project board_only_before = baseProject();
  board_only_before.schematics.clear();
  ccad::Project board_only_after = board_only_before;
  board_only_after.boards[0].pads.push_back(ccad::Pad{
      .id = "P_BOARD_ONLY",
      .component_id = "J1",
      .pin_name = "1",
      .net_id = "N_BOARD",
      .position = ccad::Point{.x = ccad::millimeters(11), .y = ccad::millimeters(6)},
      .padstack = ccad::Padstack{
         .layer_set = {"F.Cu"},
         .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
      }});
  const ccad::ProjectDiff board_only_diff =
      ccad::diffProjects(board_only_before, board_only_after);
  require(hasEntry(board_only_diff, "added", "pad", "P_BOARD_ONLY"),
          "board-only diff reports added pad without schematic");

  ccad::Project added = baseProject();
  added.schematics[0].symbols.push_back(ccad::SchSymbol{
      .id = "U2",
      .lib_id = "SENSOR",
      .pins = {ccad::SchPin{.name = "OUT", .number = "", .electrical_type = ccad::ElectricalPinType::Output}},
  });
  const ccad::ProjectDiff added_diff = ccad::diffProjects(baseProject(), added);
  require(added_diff.added_count == 1, "added count set");
  require(hasEntry(added_diff, "added", "component", "U2"), "added component entry");

  ccad::Project removed = baseProject();
  removed.schematics[0].nets.clear();
  const ccad::ProjectDiff removed_diff = ccad::diffProjects(baseProject(), removed);
  require(removed_diff.removed_count == 1, "removed count set");
  require(hasEntry(removed_diff, "removed", "net", "N_3V3"), "removed net entry");

  ccad::Project changed = baseProject();
  changed.schematics[0].constraints.at(0).value = "3.0V";
  const ccad::ProjectDiff changed_diff = ccad::diffProjects(baseProject(), changed);
  require(changed_diff.changed_count == 1, "changed count set");
  require(hasEntry(changed_diff, "changed", "constraint", "C_3V3"), "changed constraint entry");

  ccad::Project added_layer = baseProject();
  added_layer.boards[0].layers.push_back(
      ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true});
  const ccad::ProjectDiff added_layer_diff = ccad::diffProjects(baseProject(), added_layer);
  require(hasEntry(added_layer_diff, "added", "layer", "B.Cu"), "added layer entry");

  ccad::Project changed_layer = baseProject();
  changed_layer.boards[0].layers.at(0).visible = false;
  const ccad::ProjectDiff changed_layer_diff = ccad::diffProjects(baseProject(), changed_layer);
  require(hasEntry(changed_layer_diff, "changed", "layer", "F.Cu"), "changed layer entry");

  ccad::Project removed_layer = baseProject();
  removed_layer.boards[0].layers.clear();
  const ccad::ProjectDiff removed_layer_diff = ccad::diffProjects(baseProject(), removed_layer);
  require(hasEntry(removed_layer_diff, "removed", "layer", "F.Cu"), "removed layer entry");

  ccad::Project changed_outline = baseProject();
  changed_outline.boards[0].outline.size.width = ccad::millimeters(50);
  const ccad::ProjectDiff changed_outline_diff =
      ccad::diffProjects(baseProject(), changed_outline);
  require(hasEntry(changed_outline_diff, "changed", "board_outline", "board"),
          "changed board outline entry");

  ccad::Project changed_rules = baseProject();
  changed_rules.boards[0].design_rules.copper_clearance = ccad::millimeters(0.25);
  const ccad::ProjectDiff changed_rules_diff = ccad::diffProjects(baseProject(), changed_rules);
  require(hasEntry(changed_rules_diff, "changed", "design_rules", "board"),
          "changed design rules entry");

  ccad::Project added_board = baseProject();
  added_board.boards.clear();
  const ccad::ProjectDiff added_board_diff = ccad::diffProjects(added_board, baseProject());
  require(hasEntry(added_board_diff, "added", "board_outline", "board"),
          "added board outline entry");

  ccad::Project removed_board = baseProject();
  removed_board.boards.clear();
  const ccad::ProjectDiff removed_board_diff = ccad::diffProjects(baseProject(), removed_board);
  require(hasEntry(removed_board_diff, "removed", "board_outline", "board"),
          "removed board outline entry");

  ccad::Project added_pad = baseProject();
  added_pad.boards[0].pads.push_back(ccad::Pad{
      .id = "P2",
      .component_id = "U1",
      .pin_name = "VDD",
      .net_id = "N_3V3",
      .position = ccad::Point{.x = ccad::millimeters(7), .y = ccad::millimeters(6)},
      .padstack = ccad::Padstack{
         .layer_set = {"F.Cu"},
         .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
      }});
  const ccad::ProjectDiff added_pad_diff = ccad::diffProjects(baseProject(), added_pad);
  require(hasEntry(added_pad_diff, "added", "pad", "P2"), "added pad entry");

  ccad::Project changed_pad = baseProject();
  changed_pad.boards[0].pads.at(0).position.x = ccad::millimeters(8);
  const ccad::ProjectDiff changed_pad_diff = ccad::diffProjects(baseProject(), changed_pad);
  require(hasEntry(changed_pad_diff, "changed", "pad", "P1"), "changed pad entry");

  ccad::Project removed_pad = baseProject();
  removed_pad.boards[0].pads.clear();
  const ccad::ProjectDiff removed_pad_diff = ccad::diffProjects(baseProject(), removed_pad);
  require(hasEntry(removed_pad_diff, "removed", "pad", "P1"), "removed pad entry");

  ccad::Project added_via = baseProject();
  added_via.boards[0].vias.push_back(ccad::Via{.id = "V2",
                                            .net_id = "N_3V3",
                                            .position = ccad::Point{.x = ccad::millimeters(10),
                                                                    .y = ccad::millimeters(9)},
                                            .diameter = ccad::millimeters(0.8),
                                            .drill = ccad::millimeters(0.4)});
  const ccad::ProjectDiff added_via_diff = ccad::diffProjects(baseProject(), added_via);
  require(hasEntry(added_via_diff, "added", "via", "V2"), "added via entry");

  ccad::Project changed_via = baseProject();
  changed_via.boards[0].vias.at(0).diameter = ccad::millimeters(0.9);
  const ccad::ProjectDiff changed_via_diff = ccad::diffProjects(baseProject(), changed_via);
  require(hasEntry(changed_via_diff, "changed", "via", "V1"), "changed via entry");

  ccad::Project removed_via = baseProject();
  removed_via.boards[0].vias.clear();
  const ccad::ProjectDiff removed_via_diff = ccad::diffProjects(baseProject(), removed_via);
  require(hasEntry(removed_via_diff, "removed", "via", "V1"), "removed via entry");

  ccad::Project added_track = baseProject();
  added_track.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T2",
      .net_id = "N_3V3",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
      .end = ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(9)},
      .width = ccad::millimeters(0.25)});
  const ccad::ProjectDiff added_track_diff = ccad::diffProjects(baseProject(), added_track);
  require(hasEntry(added_track_diff, "added", "track", "T2"), "added track entry");

  ccad::Project changed_track = baseProject();
  changed_track.boards[0].tracks.at(0).width = ccad::millimeters(0.3);
  const ccad::ProjectDiff changed_track_diff = ccad::diffProjects(baseProject(), changed_track);
  require(hasEntry(changed_track_diff, "changed", "track", "T1"), "changed track entry");

  ccad::Project removed_track = baseProject();
  removed_track.boards[0].tracks.clear();
  const ccad::ProjectDiff removed_track_diff = ccad::diffProjects(baseProject(), removed_track);
  require(hasEntry(removed_track_diff, "removed", "track", "T1"), "removed track entry");

  ccad::Project added_graphic = baseProject();
  added_graphic.boards[0].graphics.push_back(ccad::BoardGraphic{
      .id = "G1",
      .kind = "line",
      .layer_id = "Dwgs.User",
      .start = ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(3)},
      .end = ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(3)},
      .width = ccad::millimeters(0.15)});
  const ccad::ProjectDiff added_graphic_diff = ccad::diffProjects(baseProject(), added_graphic);
  require(hasEntry(added_graphic_diff, "added", "graphic", "G1"), "added graphic entry");

  ccad::Project changed_graphic = added_graphic;
  changed_graphic.boards[0].graphics.at(0).width = ccad::millimeters(0.2);
  const ccad::ProjectDiff changed_graphic_diff =
      ccad::diffProjects(added_graphic, changed_graphic);
  require(hasEntry(changed_graphic_diff, "changed", "graphic", "G1"),
          "changed graphic entry");

  ccad::Project removed_graphic = added_graphic;
  removed_graphic.boards[0].graphics.clear();
  const ccad::ProjectDiff removed_graphic_diff =
      ccad::diffProjects(added_graphic, removed_graphic);
  require(hasEntry(removed_graphic_diff, "removed", "graphic", "G1"),
          "removed graphic entry");

  ccad::Project added_text = baseProject();
  added_text.boards[0].texts.push_back(ccad::BoardText{
      .id = "BT1",
      .layer_id = "F.SilkS",
      .text = "Full bridge rectifier",
      .position = ccad::Point{.x = ccad::millimeters(4), .y = ccad::millimeters(5)},
      .rotation_degrees = 0.0,
      .size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.5)}});
  const ccad::ProjectDiff added_text_diff = ccad::diffProjects(baseProject(), added_text);
  require(hasEntry(added_text_diff, "added", "text", "BT1"), "added text entry");

  ccad::Project changed_text = added_text;
  changed_text.boards[0].texts.at(0).text = "DC output";
  const ccad::ProjectDiff changed_text_diff = ccad::diffProjects(added_text, changed_text);
  require(hasEntry(changed_text_diff, "changed", "text", "BT1"), "changed text entry");

  ccad::Project removed_text = added_text;
  removed_text.boards[0].texts.clear();
  const ccad::ProjectDiff removed_text_diff = ccad::diffProjects(added_text, removed_text);
  require(hasEntry(removed_text_diff, "removed", "text", "BT1"), "removed text entry");

  ccad::Project added_zone = baseProject();
  added_zone.boards[0].zones.push_back(ccad::BoardZone{
      .id = "Z1",
      .name = "GND copper",
      .net_id = "N_3V3",
      .layer_ids = {"F.Cu"},
      .outline = {ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(2)},
                  ccad::Point{.x = ccad::millimeters(20), .y = ccad::millimeters(2)},
                  ccad::Point{.x = ccad::millimeters(20), .y = ccad::millimeters(12)},
                  ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(12)}},
      .priority = 1,
      .clearance = ccad::millimeters(0.2),
      .min_thickness = ccad::millimeters(0.25),
      .fill_enabled = true,
      .pad_connection = "thermal",
  });
  const ccad::ProjectDiff added_zone_diff = ccad::diffProjects(baseProject(), added_zone);
  require(hasEntry(added_zone_diff, "added", "zone", "Z1"), "added zone entry");

  ccad::Project changed_zone = added_zone;
  changed_zone.boards[0].zones.at(0).priority = 4;
  const ccad::ProjectDiff changed_zone_diff = ccad::diffProjects(added_zone, changed_zone);
  require(hasEntry(changed_zone_diff, "changed", "zone", "Z1"), "changed zone entry");

  ccad::Project removed_zone = added_zone;
  removed_zone.boards[0].zones.clear();
  const ccad::ProjectDiff removed_zone_diff = ccad::diffProjects(added_zone, removed_zone);
  require(hasEntry(removed_zone_diff, "removed", "zone", "Z1"), "removed zone entry");

  ccad::Project added_route_request = baseProject();
  added_route_request.boards[0].route_requests.push_back(ccad::RouteRequest{
      .id = "RR1",
      .net_id = "N_3V3",
      .from_object_id = "P1",
      .to_object_id = "V1",
      .preferred_layer_id = "F.Cu",
      .policy = "shortest_safe",
      .width = ccad::millimeters(0.25),
  });
  const ccad::ProjectDiff added_route_request_diff =
      ccad::diffProjects(baseProject(), added_route_request);
  require(hasEntry(added_route_request_diff, "added", "route_request", "RR1"),
          "added route request entry");

  ccad::Project changed_route_request = added_route_request;
  changed_route_request.boards[0].route_requests.at(0).policy = "prefer_top";
  const ccad::ProjectDiff changed_route_request_diff =
      ccad::diffProjects(added_route_request, changed_route_request);
  require(hasEntry(changed_route_request_diff, "changed", "route_request", "RR1"),
          "changed route request entry");

  ccad::Project removed_route_request = added_route_request;
  removed_route_request.boards[0].route_requests.clear();
  const ccad::ProjectDiff removed_route_request_diff =
      ccad::diffProjects(added_route_request, removed_route_request);
  require(hasEntry(removed_route_request_diff, "removed", "route_request", "RR1"),
          "removed route request entry");

  ccad::Project added_keepout = baseProject();
  added_keepout.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K2",
      .kind = "routing",
      .area = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(24), .y = ccad::millimeters(10)},
          .size = ccad::Size{.width = ccad::millimeters(3),
                              .height = ccad::millimeters(2)}}});
  const ccad::ProjectDiff added_keepout_diff = ccad::diffProjects(baseProject(), added_keepout);
  require(hasEntry(added_keepout_diff, "added", "keepout", "K2"), "added keepout entry");

  ccad::Project changed_keepout = baseProject();
  changed_keepout.boards[0].keepouts.at(0).area.size.width = ccad::millimeters(4);
  const ccad::ProjectDiff changed_keepout_diff =
      ccad::diffProjects(baseProject(), changed_keepout);
  require(hasEntry(changed_keepout_diff, "changed", "keepout", "K1"),
          "changed keepout entry");

  ccad::Project removed_keepout = baseProject();
  removed_keepout.boards[0].keepouts.clear();
  const ccad::ProjectDiff removed_keepout_diff =
      ccad::diffProjects(baseProject(), removed_keepout);
  require(hasEntry(removed_keepout_diff, "removed", "keepout", "K1"),
          "removed keepout entry");

  ccad::Project added_region = baseProject();
  added_region.boards[0].placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR2",
      .kind = "module",
      .area = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(18), .y = ccad::millimeters(6)},
          .size = ccad::Size{.width = ccad::millimeters(5),
                              .height = ccad::millimeters(4)}}});
  const ccad::ProjectDiff added_region_diff = ccad::diffProjects(baseProject(), added_region);
  require(hasEntry(added_region_diff, "added", "placement_region", "PR2"),
          "added placement region entry");

  ccad::Project changed_region = baseProject();
  changed_region.boards[0].placement_regions.at(0).kind = "module";
  const ccad::ProjectDiff changed_region_diff =
      ccad::diffProjects(baseProject(), changed_region);
  require(hasEntry(changed_region_diff, "changed", "placement_region", "PR1"),
          "changed placement region entry");

  ccad::Project removed_region = baseProject();
  removed_region.boards[0].placement_regions.clear();
  const ccad::ProjectDiff removed_region_diff =
      ccad::diffProjects(baseProject(), removed_region);
  require(hasEntry(removed_region_diff, "removed", "placement_region", "PR1"),
          "removed placement region entry");
}

