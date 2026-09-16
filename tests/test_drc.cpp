#include "ccad_core/drc.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

#include <string>
#include <vector>

namespace {

ccad::Project validBoardProject() {
  ccad::Project project;
  project.id = "proj-drc";
  project.name = "drc";
  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].symbols = {ccad::SchSymbol{
      .id = "U1",
      .lib_id = "MCU",
      .pins = {ccad::SchPin{.name = "1", .number = "", .electrical_type = ccad::ElectricalPinType::Passive}},
  }};
  project.schematics[0].nets = {ccad::Net{.id = "N1",
                            .members = {ccad::NetMember{.component_id = "U1", .pin_name = "1"}}},
                  ccad::Net{.id = "N2",
                            .members = {ccad::NetMember{.component_id = "U2", .pin_name = "1"}}}};
  project.boards.push_back(ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                 ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true}},
      .placement_regions = {},
      .keepouts = {},
      .pads = {ccad::Pad{.id = "P1",
                         .component_id = "U1",
                         .pin_name = "1",
                         .net_id = "N1",
                         .type = "smd",
                         .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                         .padstack = ccad::Padstack{
                            .layer_set = {"F.Cu"},
                            .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.0)}}}}}
                         }}},
      .vias = {ccad::Via{.id = "V1",
                         .net_id = "N1",
                         .position = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
                         .diameter = ccad::millimeters(0.8),
                         .drill = ccad::millimeters(0.4)}},
      .tracks = {ccad::TrackSegment{.id = "T1",
                                    .net_id = "N1",
                                    .layer_id = "F.Cu",
                                    .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                                    .end = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
                                    .width = ccad::millimeters(0.25)}},
      .graphics = {},
      .texts = {},
      .zones = {ccad::BoardZone{
          .id = "Z1",
          .name = "GND copper",
          .net_id = "N1",
          .layer_ids = {"F.Cu"},
          .outline = {ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(2)},
                      ccad::Point{.x = ccad::millimeters(18), .y = ccad::millimeters(2)},
                      ccad::Point{.x = ccad::millimeters(18), .y = ccad::millimeters(12)},
                      ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(12)}},
          .priority = 1,
          .clearance = ccad::millimeters(0.2),
          .min_thickness = ccad::millimeters(0.25),
          .fill_enabled = true,
          .pad_connection = "thermal",
      }},
      .route_requests = {},
  });
  return project;
}

ccad::Project boardOnlyProject() {
  ccad::Project project = validBoardProject();
  project.schematics.clear();
  return project;
}

bool hasCode(const std::vector<ccad::Diagnostic>& diagnostics, const std::string& code) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.code == code) {
      return true;
    }
  }
  return false;
}

bool hasDiagnostic(const std::vector<ccad::Diagnostic>& diagnostics, const std::string& code,
                   const std::string& severity) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.code == code && diagnostic.severity == severity) {
      return true;
    }
  }
  return false;
}

bool hasDiagnosticForObject(const std::vector<ccad::Diagnostic>& diagnostics,
                            const std::string& code, const std::string& object_id) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.code == code && diagnostic.object_id == object_id) {
      return true;
    }
  }
  return false;
}

bool hasDiagnosticMessageContaining(const std::vector<ccad::Diagnostic>& diagnostics,
                                    const std::string& code,
                                    const std::string& message_fragment) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.code == code &&
        diagnostic.message.find(message_fragment) != std::string::npos) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  require(ccad::runDrc(validBoardProject()).empty(), "valid board has no drc diagnostics");

  require(ccad::runDrc(boardOnlyProject()).empty(),
          "board-only project runs physical DRC without requiring a linked schematic");

  ccad::Project board_only_route = boardOnlyProject();
  board_only_route.boards[0].route_requests.push_back(ccad::RouteRequest{
      .id = "RR_BOARD_ONLY",
      .net_id = "N1",
      .from_object_id = "P1",
      .to_object_id = "V1",
      .preferred_layer_id = "F.Cu",
      .policy = "shortest_safe",
      .width = ccad::millimeters(0.25)});
  require(ccad::runDrc(board_only_route).empty(),
          "board-only route request validates from board-local copper connectivity");

  ccad::Project invalid_board_outline = validBoardProject();
  invalid_board_outline.boards[0].outline.size.width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_board_outline), "INVALID_BOARD_OUTLINE"),
          "drc reports invalid board outline size");

  ccad::Project zero_rule_minima = validBoardProject();
  zero_rule_minima.boards[0].design_rules.copper_clearance = ccad::nanometers(0);
  zero_rule_minima.boards[0].design_rules.min_track_width = ccad::nanometers(0);
  zero_rule_minima.boards[0].design_rules.min_via_annular_ring = ccad::nanometers(0);
  require(!hasCode(ccad::runDrc(zero_rule_minima), "INVALID_COPPER_CLEARANCE"),
          "drc accepts KiCad-compatible zero copper clearance rule");
  require(!hasCode(ccad::runDrc(zero_rule_minima), "INVALID_MIN_TRACK_WIDTH"),
          "drc accepts KiCad-compatible zero minimum track width rule");
  require(!hasCode(ccad::runDrc(zero_rule_minima), "INVALID_MIN_VIA_ANNULAR_RING"),
          "drc accepts KiCad-compatible zero minimum via annular ring rule");

  ccad::Project invalid_copper_clearance = validBoardProject();
  invalid_copper_clearance.boards[0].design_rules.copper_clearance = ccad::millimeters(25.01);
  require(hasCode(ccad::runDrc(invalid_copper_clearance), "INVALID_COPPER_CLEARANCE"),
          "drc reports copper clearance outside KiCad range");

  ccad::Project invalid_min_track_width = validBoardProject();
  invalid_min_track_width.boards[0].design_rules.min_track_width = ccad::millimeters(25.01);
  require(hasCode(ccad::runDrc(invalid_min_track_width), "INVALID_MIN_TRACK_WIDTH"),
          "drc reports minimum track width outside KiCad range");

  ccad::Project invalid_min_via_annular_ring = validBoardProject();
  invalid_min_via_annular_ring.boards[0].design_rules.min_via_annular_ring =
      ccad::millimeters(25.01);
  require(hasCode(ccad::runDrc(invalid_min_via_annular_ring), "INVALID_MIN_VIA_ANNULAR_RING"),
          "drc reports minimum via annular ring outside KiCad range");

  ccad::Project invalid_min_connection = validBoardProject();
  invalid_min_connection.boards[0].design_rules.min_connection = ccad::nanometers(-1);
  require(hasCode(ccad::runDrc(invalid_min_connection), "INVALID_MIN_CONNECTION"),
          "drc reports negative minimum connection rule");

  ccad::Project invalid_min_via_diameter = validBoardProject();
  invalid_min_via_diameter.boards[0].design_rules.min_via_diameter = ccad::nanometers(-1);
  require(hasCode(ccad::runDrc(invalid_min_via_diameter), "INVALID_MIN_VIA_DIAMETER"),
          "drc reports negative minimum via diameter rule");

  ccad::Project invalid_min_through_hole_drill = validBoardProject();
  invalid_min_through_hole_drill.boards[0].design_rules.min_through_hole_drill =
      ccad::nanometers(-1);
  require(
      hasCode(ccad::runDrc(invalid_min_through_hole_drill), "INVALID_MIN_THROUGH_HOLE_DRILL"),
      "drc reports negative minimum through hole drill rule");

  ccad::Project invalid_solder_paste_ratio = validBoardProject();
  invalid_solder_paste_ratio.boards[0].design_rules.solder_paste_margin_ratio = 1.25;
  require(hasCode(ccad::runDrc(invalid_solder_paste_ratio),
                  "INVALID_SOLDER_PASTE_MARGIN_RATIO"),
          "drc reports solder paste margin ratio outside KiCad-style range");

  ccad::Project pad_outside = validBoardProject();
  pad_outside.boards[0].pads.at(0).position.x = ccad::millimeters(99);
  require(hasCode(ccad::runDrc(pad_outside), "PAD_OUTSIDE_BOARD"),
          "drc reports pad outside board");

  ccad::Project unknown_track_layer = validBoardProject();
  unknown_track_layer.boards[0].tracks.at(0).layer_id = "Inner.Cu";
  require(hasCode(ccad::runDrc(unknown_track_layer), "UNKNOWN_TRACK_LAYER"),
          "drc reports unknown track layer");

  ccad::Project non_copper_pad_layer = validBoardProject();
  non_copper_pad_layer.boards[0].layers.push_back(
      ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen"});
  non_copper_pad_layer.boards[0].pads.at(0).padstack.layer_set = {"F.SilkS"};
  require(hasCode(ccad::runDrc(non_copper_pad_layer), "PAD_NON_COPPER_LAYER"),
          "drc reports pad on non-copper layer");

  ccad::Project non_copper_track_layer = validBoardProject();
  non_copper_track_layer.boards[0].layers.push_back(
      ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen"});
  non_copper_track_layer.boards[0].tracks.at(0).layer_id = "F.SilkS";
  require(hasCode(ccad::runDrc(non_copper_track_layer), "TRACK_NON_COPPER_LAYER"),
          "drc reports track on non-copper layer");

  ccad::Project unknown_zone_layer = validBoardProject();
  unknown_zone_layer.boards[0].zones.at(0).layer_ids = {"Inner.Cu"};
  require(hasCode(ccad::runDrc(unknown_zone_layer), "UNKNOWN_ZONE_LAYER"),
          "drc reports unknown zone layer");

  ccad::Project non_copper_zone_layer = validBoardProject();
  non_copper_zone_layer.boards[0].layers.push_back(
      ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen"});
  non_copper_zone_layer.boards[0].zones.at(0).layer_ids = {"F.SilkS"};
  require(hasCode(ccad::runDrc(non_copper_zone_layer), "ZONE_NON_COPPER_LAYER"),
          "drc reports zone on non-copper layer");

  ccad::Project unknown_zone_net = validBoardProject();
  unknown_zone_net.boards[0].zones.at(0).net_id = "NO_NET";
  require(hasCode(ccad::runDrc(unknown_zone_net), "UNKNOWN_ZONE_NET"),
          "drc reports unknown zone net");

  ccad::Project invalid_zone_outline = validBoardProject();
  invalid_zone_outline.boards[0].zones.at(0).outline.pop_back();
  invalid_zone_outline.boards[0].zones.at(0).outline.pop_back();
  require(hasCode(ccad::runDrc(invalid_zone_outline), "INVALID_ZONE_OUTLINE"),
          "drc reports zone outline with fewer than three corners");

  ccad::Project zone_outside = validBoardProject();
  zone_outside.boards[0].zones.at(0).outline.at(1).x = ccad::millimeters(99);
  require(hasCode(ccad::runDrc(zone_outside), "ZONE_OUTSIDE_BOARD"),
          "drc reports zone outside board");

  ccad::Project invalid_zone_clearance = validBoardProject();
  invalid_zone_clearance.boards[0].zones.at(0).clearance = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_zone_clearance), "INVALID_ZONE_CLEARANCE"),
          "drc reports non-positive zone clearance");

  ccad::Project invalid_zone_min_thickness = validBoardProject();
  invalid_zone_min_thickness.boards[0].zones.at(0).min_thickness = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_zone_min_thickness), "INVALID_ZONE_MIN_THICKNESS"),
          "drc reports non-positive zone minimum thickness");

  ccad::Project invalid_zone_pad_connection = validBoardProject();
  invalid_zone_pad_connection.boards[0].zones.at(0).pad_connection = "mystery";
  require(hasCode(ccad::runDrc(invalid_zone_pad_connection), "INVALID_ZONE_PAD_CONNECTION"),
          "drc reports unsupported zone pad connection mode");

  ccad::Project duplicate_layer = validBoardProject();
  duplicate_layer.boards[0].layers.push_back(duplicate_layer.boards[0].layers.front());
  require(hasCode(ccad::runDrc(duplicate_layer), "DUPLICATE_LAYER_ID"),
          "drc reports duplicate layer id");

  ccad::Project empty_layer_id = validBoardProject();
  empty_layer_id.boards[0].layers.push_back(
      ccad::Layer{.id = "", .name = "Invalid", .kind = "copper", .visible = true});
  require(hasCode(ccad::runDrc(empty_layer_id), "INVALID_LAYER_ID"),
          "drc reports empty layer id");

  ccad::Project empty_layer_name = validBoardProject();
  empty_layer_name.boards[0].layers.at(0).name.clear();
  require(hasCode(ccad::runDrc(empty_layer_name), "INVALID_LAYER_NAME"),
          "drc reports empty layer name");

  ccad::Project empty_layer_kind = validBoardProject();
  empty_layer_kind.boards[0].layers.at(0).kind.clear();
  require(hasCode(ccad::runDrc(empty_layer_kind), "INVALID_LAYER_KIND"),
          "drc reports empty layer kind");

  ccad::Project duplicate_net = validBoardProject();
  duplicate_net.schematics[0].nets.push_back(duplicate_net.schematics[0].nets.front());
  require(hasCode(ccad::runDrc(duplicate_net), "DUPLICATE_NET_ID"),
          "drc reports duplicate net id");

  ccad::Project empty_net_id = validBoardProject();
  empty_net_id.schematics[0].nets.push_back(
      ccad::Net{.id = "", .members = {ccad::NetMember{.component_id = "U3", .pin_name = "1"}}});
  require(hasCode(ccad::runDrc(empty_net_id), "INVALID_NET_ID"),
          "drc reports empty net id");

  ccad::Project invalid_net_member = validBoardProject();
  invalid_net_member.schematics[0].nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "", .pin_name = "2"});
  require(hasCode(ccad::runDrc(invalid_net_member), "INVALID_NET_MEMBER"),
          "drc reports invalid net member fields");

  ccad::Project duplicate_net_member = validBoardProject();
  duplicate_net_member.schematics[0].nets.at(0).members.push_back(
      duplicate_net_member.schematics[0].nets.at(0).members.front());
  require(hasCode(ccad::runDrc(duplicate_net_member), "DUPLICATE_NET_MEMBER"),
          "drc reports duplicate net member mapping");

  ccad::Project via_drill_too_large = validBoardProject();
  via_drill_too_large.boards[0].vias.at(0).drill = ccad::millimeters(1.0);
  require(hasCode(ccad::runDrc(via_drill_too_large), "VIA_DRILL_TOO_LARGE"),
          "drc reports via drill too large");
  require(!hasCode(ccad::runDrc(via_drill_too_large), "VIA_ANNULAR_RING_TOO_SMALL"),
          "drc does not report derived annular ring error when via drill exceeds diameter");

  ccad::Project via_geometry_outside = validBoardProject();
  via_geometry_outside.boards[0].vias.at(0).position =
      ccad::Point{.x = ccad::millimeters(0.1), .y = ccad::millimeters(0.1)};
  require(hasCode(ccad::runDrc(via_geometry_outside), "VIA_GEOMETRY_OUTSIDE_BOARD"),
          "drc reports via copper geometry outside board");

  ccad::Project via_edge_clearance = validBoardProject();
  via_edge_clearance.boards[0].vias.at(0).position =
      ccad::Point{.x = ccad::millimeters(0.8), .y = ccad::millimeters(5.0)};
  require(hasCode(ccad::runDrc(via_edge_clearance), "VIA_EDGE_CLEARANCE"),
          "drc reports via copper too close to board edge");

  ccad::Project via_small_ring = validBoardProject();
  via_small_ring.boards[0].vias.at(0).diameter = ccad::millimeters(0.45);
  via_small_ring.boards[0].vias.at(0).drill = ccad::millimeters(0.4);
  require(hasCode(ccad::runDrc(via_small_ring), "VIA_ANNULAR_RING_TOO_SMALL"),
          "drc reports via annular ring below configured minimum");
  require(hasDiagnosticMessageContaining(ccad::runDrc(via_small_ring),
                                         "VIA_ANNULAR_RING_TOO_SMALL",
                                         "configured minimum 100000 nm"),
          "drc via annular ring diagnostic includes configured minimum value");

  ccad::Project relaxed_via_ring = via_small_ring;
  relaxed_via_ring.boards[0].design_rules.min_via_annular_ring = ccad::millimeters(0.02);
  require(!hasCode(ccad::runDrc(relaxed_via_ring), "VIA_ANNULAR_RING_TOO_SMALL"),
          "drc obeys configured minimum via annular ring");

  ccad::Project via_diameter_below_minimum = validBoardProject();
  via_diameter_below_minimum.boards[0].design_rules.min_via_diameter = ccad::millimeters(0.90);
  require(hasCode(ccad::runDrc(via_diameter_below_minimum), "VIA_DIAMETER_BELOW_MINIMUM"),
          "drc reports via diameter below configured minimum");
  require(hasDiagnosticMessageContaining(ccad::runDrc(via_diameter_below_minimum),
                                         "VIA_DIAMETER_BELOW_MINIMUM",
                                         "configured minimum 900000 nm"),
          "drc via diameter diagnostic includes configured minimum value");

  ccad::Project relaxed_via_diameter = via_diameter_below_minimum;
  relaxed_via_diameter.boards[0].design_rules.min_via_diameter = ccad::millimeters(0.70);
  require(!hasCode(ccad::runDrc(relaxed_via_diameter), "VIA_DIAMETER_BELOW_MINIMUM"),
          "drc obeys configured minimum via diameter");

  ccad::Project wide_via = validBoardProject();
  wide_via.boards[0].design_rules.max_via_diameter = ccad::millimeters(0.70);
  wide_via.boards[0].vias.at(0).diameter = ccad::millimeters(0.90);
  require(hasCode(ccad::runDrc(wide_via), "VIA_DIAMETER_ABOVE_MAXIMUM"),
          "drc reports via above configured maximum");

  ccad::Project via_drill_below_minimum = validBoardProject();
  via_drill_below_minimum.boards[0].design_rules.min_through_hole_drill =
      ccad::millimeters(0.50);
  require(hasCode(ccad::runDrc(via_drill_below_minimum), "VIA_DRILL_BELOW_MINIMUM"),
          "drc reports via drill below configured minimum");
  require(hasDiagnosticMessageContaining(ccad::runDrc(via_drill_below_minimum),
                                         "VIA_DRILL_BELOW_MINIMUM",
                                         "configured minimum 500000 nm"),
          "drc via drill diagnostic includes configured minimum value");

  ccad::Project relaxed_via_drill = via_drill_below_minimum;
  relaxed_via_drill.boards[0].design_rules.min_through_hole_drill = ccad::millimeters(0.30);
  require(!hasCode(ccad::runDrc(relaxed_via_drill), "VIA_DRILL_BELOW_MINIMUM"),
          "drc obeys configured minimum through hole drill");

  ccad::Project zero_length_track = validBoardProject();
  zero_length_track.boards[0].tracks.at(0).end = zero_length_track.boards[0].tracks.at(0).start;
  require(hasCode(ccad::runDrc(zero_length_track), "ZERO_LENGTH_TRACK"),
          "drc reports zero length track");

  ccad::Project narrow_track = validBoardProject();
  narrow_track.boards[0].tracks.at(0).width = ccad::millimeters(0.10);
  require(hasCode(ccad::runDrc(narrow_track), "TRACK_TOO_NARROW"),
          "drc reports track width below configured minimum");
  require(hasDiagnosticMessageContaining(ccad::runDrc(narrow_track), "TRACK_TOO_NARROW",
                                         "configured minimum 150000 nm"),
          "drc track width diagnostic includes configured minimum value");

  ccad::Project relaxed_track_width = narrow_track;
  relaxed_track_width.boards[0].design_rules.min_track_width = ccad::millimeters(0.08);
  require(!hasCode(ccad::runDrc(relaxed_track_width), "TRACK_TOO_NARROW"),
          "drc obeys configured minimum track width");

  ccad::Project wide_track = validBoardProject();
  wide_track.boards[0].design_rules.max_track_width = ccad::millimeters(0.30);
  wide_track.boards[0].tracks.at(0).width = ccad::millimeters(0.40);
  require(hasCode(ccad::runDrc(wide_track), "TRACK_TOO_WIDE"),
          "drc reports track above configured maximum");

  ccad::Project track_geometry_outside = validBoardProject();
  track_geometry_outside.boards[0].tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(0.05), .y = ccad::millimeters(6)};
  track_geometry_outside.boards[0].tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)};
  require(hasCode(ccad::runDrc(track_geometry_outside), "TRACK_GEOMETRY_OUTSIDE_BOARD"),
          "drc reports track copper geometry outside board");

  ccad::Project duplicate_pad = validBoardProject();
  duplicate_pad.boards[0].pads.push_back(duplicate_pad.boards[0].pads.at(0));
  require(hasCode(ccad::runDrc(duplicate_pad), "DUPLICATE_PAD_ID"),
          "drc reports duplicate pad id");

  ccad::Project duplicate_physical_object_id = validBoardProject();
  duplicate_physical_object_id.boards[0].vias.at(0).id = "P1";
  require(hasCode(ccad::runDrc(duplicate_physical_object_id), "DUPLICATE_PHYSICAL_OBJECT_ID"),
          "drc reports physical object id reused across types");

  ccad::Project valid_board_graphic_text = validBoardProject();
  valid_board_graphic_text.boards[0].layers.push_back(
      ccad::Layer{.id = "Dwgs.User", .name = "User drawings", .kind = "user"});
  valid_board_graphic_text.boards[0].layers.push_back(
      ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen"});
  valid_board_graphic_text.boards[0].graphics.push_back(ccad::BoardGraphic{
      .id = "G1",
      .kind = "line",
      .layer_id = "Dwgs.User",
      .start = ccad::Point{.x = ccad::millimeters(3), .y = ccad::millimeters(4)},
      .end = ccad::Point{.x = ccad::millimeters(15), .y = ccad::millimeters(4)},
      .width = ccad::millimeters(0.15)});
  valid_board_graphic_text.boards[0].texts.push_back(ccad::BoardText{
      .id = "BT1",
      .layer_id = "F.SilkS",
      .text = "RECTIFIER",
      .position = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(20)},
      .size = ccad::Size{.width = ccad::millimeters(1.5),
                         .height = ccad::millimeters(1.5)}});
  require(ccad::runDrc(valid_board_graphic_text).empty(),
          "valid board graphic and text have no drc diagnostics");
  ccad::Project short_text = boardOnlyProject();
  short_text.boards[0].design_rules.min_text_height = ccad::millimeters(1.0);
  short_text.boards[0].texts.push_back(ccad::BoardText{
      .id = "TXT_MIN", .layer_id = "F.SilkS", .text = "A",
      .position = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(10)},
      .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(0.5)}});
  require(hasCode(ccad::runDrc(short_text), "TEXT_HEIGHT_BELOW_MINIMUM"),
          "text below configured minimum height is diagnosed");
  ccad::Project thin_text = valid_board_graphic_text;
  thin_text.boards[0].design_rules.min_text_thickness = ccad::millimeters(0.15);
  thin_text.boards[0].texts.at(0).stroke_width = ccad::millimeters(0.10);
  require(hasCode(ccad::runDrc(thin_text), "TEXT_THICKNESS_BELOW_MINIMUM"),
          "text below configured minimum thickness is diagnosed");
  ccad::Project front_mirrored = valid_board_graphic_text;
  front_mirrored.boards[0].texts.at(0).mirrored = true;
  require(hasCode(ccad::runDrc(front_mirrored), "MIRRORED_TEXT_ON_FRONT_LAYER"),
          "mirrored front text is diagnosed");
  ccad::Project back_unmirrored = valid_board_graphic_text;
  back_unmirrored.boards[0].texts.at(0).layer_id = "B.SilkS";
  require(hasCode(ccad::runDrc(back_unmirrored), "NONMIRRORED_TEXT_ON_BACK_LAYER"),
          "unmirrored back text is diagnosed");
  ccad::Project acute_track_angle = boardOnlyProject();
  acute_track_angle.boards[0].design_rules.min_track_angle_degrees = 150.0;
  acute_track_angle.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T_ANGLE", .net_id = "N1", .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
      .end = ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(9)},
      .width = ccad::millimeters(0.25)});
  require(hasCode(ccad::runDrc(acute_track_angle), "TRACK_ANGLE"),
          "connected track angle outside minimum is diagnosed");
  ccad::Project short_segment = boardOnlyProject();
  short_segment.boards[0].design_rules.min_track_segment_length = ccad::millimeters(4.0);
  short_segment.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T_SHORT", .net_id = "N1", .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(15), .y = ccad::millimeters(15)},
      .end = ccad::Point{.x = ccad::millimeters(16), .y = ccad::millimeters(15)},
      .width = ccad::millimeters(0.25)});
  require(hasCode(ccad::runDrc(short_segment), "TRACK_SEGMENT_LENGTH"),
          "short track segment is diagnosed");
  ccad::Project long_arc = boardOnlyProject();
  long_arc.boards[0].design_rules.max_track_segment_length = ccad::millimeters(2.0);
  long_arc.boards[0].track_arcs.push_back(ccad::TrackArc{
      .id = "A_LONG", .net_id = "N1", .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(10)},
      .mid = ccad::Point{.x = ccad::millimeters(11), .y = ccad::millimeters(11)},
      .end = ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(10)},
      .width = ccad::millimeters(0.25)});
  require(hasCode(ccad::runDrc(long_arc), "TRACK_SEGMENT_LENGTH"),
          "long track arc is diagnosed");

  ccad::Project silk_pad_clearance = valid_board_graphic_text;
  silk_pad_clearance.boards[0].design_rules.silk_clearance = ccad::millimeters(0.2);
  silk_pad_clearance.boards[0].texts.at(0).position =
      silk_pad_clearance.boards[0].pads.at(0).position;
  require(hasCode(ccad::runDrc(silk_pad_clearance), "SILK_CLEARANCE"),
          "drc reports silkscreen text too close to copper pad");
  ccad::Project silk_via_clearance = valid_board_graphic_text;
  silk_via_clearance.boards[0].design_rules.silk_clearance = ccad::millimeters(0.2);
  silk_via_clearance.boards[0].texts.at(0).position = silk_via_clearance.boards[0].vias.at(0).position;
  require(hasCode(ccad::runDrc(silk_via_clearance), "SILK_CLEARANCE"),
          "drc reports silkscreen text too close to copper via");
  ccad::Project silk_track_clearance = valid_board_graphic_text;
  silk_track_clearance.boards[0].design_rules.silk_clearance = ccad::millimeters(0.2);
  silk_track_clearance.boards[0].texts.at(0).position =
      ccad::Point{.x = ccad::millimeters(6), .y = ccad::millimeters(6)};
  require(hasCode(ccad::runDrc(silk_track_clearance), "SILK_CLEARANCE"),
          "drc reports silkscreen text too close to copper track");
  ccad::Project silk_edge_clearance = valid_board_graphic_text;
  silk_edge_clearance.boards[0].design_rules.silk_clearance = ccad::millimeters(0.2);
  silk_edge_clearance.boards[0].texts.at(0).position =
      ccad::Point{.x = ccad::millimeters(0.5), .y = ccad::millimeters(0.5)};
  require(hasCode(ccad::runDrc(silk_edge_clearance), "SILK_CLEARANCE"),
          "drc reports silkscreen text too close to board edge");
  ccad::Project silk_zone_clearance = valid_board_graphic_text;
  silk_zone_clearance.boards[0].design_rules.silk_clearance = ccad::millimeters(0.2);
  silk_zone_clearance.boards[0].texts.at(0).position =
      ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(10)};
  require(hasCode(ccad::runDrc(silk_zone_clearance), "SILK_CLEARANCE"),
          "drc reports silkscreen text over copper zone");

  ccad::Project missing_footprint = validBoardProject();
  missing_footprint.schematics[0].symbols.push_back(
      ccad::SchSymbol{.id = "U2", .reference = "U2"});
  missing_footprint.boards[0].footprints.push_back(
      ccad::BoardFootprint{.reference = "R1", .footprint_name = "0603"});
  require(hasCode(ccad::runDrc(missing_footprint), "MISSING_FOOTPRINT"),
          "drc reports schematic component missing board footprint");
  ccad::Project extra_footprint = validBoardProject();
  extra_footprint.boards[0].footprints.push_back(
      ccad::BoardFootprint{.reference = "R1", .footprint_name = "0603"});
  require(hasCode(ccad::runDrc(extra_footprint), "EXTRA_FOOTPRINT"),
          "drc reports board footprint missing schematic component");
  ccad::Project bom_parity = validBoardProject();
  bom_parity.boards[0].footprints.push_back(
      ccad::BoardFootprint{.reference = "U1", .exclude_from_bom = true});
  require(hasCode(ccad::runDrc(bom_parity), "FOOTPRINT_BOM_PARITY"),
          "drc reports schematic and footprint BOM parity mismatch");
  ccad::Project missing_pad = validBoardProject();
  missing_pad.boards[0].footprints.push_back(ccad::BoardFootprint{.reference = "U1"});
  missing_pad.schematics[0].symbols.front().pins.push_back(
      ccad::SchPin{.name = "2", .number = "2"});
  require(hasCode(ccad::runDrc(missing_pad), "MISSING_PAD"),
          "drc reports schematic pin missing from board footprint");
  ccad::Project schematic_only = validBoardProject();
  schematic_only.schematics[0].symbols.front().on_board = false;
  require(!hasCode(ccad::runDrc(schematic_only), "MISSING_FOOTPRINT"),
          "drc allows schematic-only component without board footprint");
  ccad::Project mask_bridge = boardOnlyProject();
  mask_bridge.boards[0].design_rules.solder_mask_min_width = ccad::millimeters(0.1);
  mask_bridge.boards[0].design_rules.solder_mask_expansion = ccad::millimeters(0.05);
  ccad::Pad mask_pad = mask_bridge.boards[0].pads.front();
  mask_pad.id = "P2"; mask_pad.net_id = "N2"; mask_pad.position.x = ccad::millimeters(5.8);
  mask_bridge.boards[0].pads.push_back(mask_pad);
  require(hasCode(ccad::runDrc(mask_bridge), "SOLDERMASK_BRIDGE"),
          "drc reports different-net solder mask bridge");

  ccad::Project empty_graphic_id = valid_board_graphic_text;
  empty_graphic_id.boards[0].graphics.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_graphic_id), "INVALID_BOARD_GRAPHIC_ID"),
          "drc reports empty board graphic id");

  ccad::Project duplicate_graphic_id = valid_board_graphic_text;
  duplicate_graphic_id.boards[0].graphics.push_back(duplicate_graphic_id.boards[0].graphics.front());
  require(hasCode(ccad::runDrc(duplicate_graphic_id), "DUPLICATE_BOARD_GRAPHIC_ID"),
          "drc reports duplicate board graphic ids");

  ccad::Project unknown_graphic_layer = valid_board_graphic_text;
  unknown_graphic_layer.boards[0].graphics.at(0).layer_id = "Missing.User";
  require(hasCode(ccad::runDrc(unknown_graphic_layer), "UNKNOWN_BOARD_GRAPHIC_LAYER"),
          "drc reports board graphic unknown layer");

  ccad::Project invalid_graphic_width = valid_board_graphic_text;
  invalid_graphic_width.boards[0].graphics.at(0).width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_graphic_width), "INVALID_BOARD_GRAPHIC_WIDTH"),
          "drc reports non-positive board graphic width");

  ccad::Project zero_length_graphic = valid_board_graphic_text;
  zero_length_graphic.boards[0].graphics.at(0).end =
      zero_length_graphic.boards[0].graphics.at(0).start;
  require(hasCode(ccad::runDrc(zero_length_graphic), "ZERO_LENGTH_BOARD_GRAPHIC"),
          "drc reports zero length board graphic line");

  ccad::Project graphic_outside = valid_board_graphic_text;
  graphic_outside.boards[0].graphics.at(0).end =
      ccad::Point{.x = ccad::millimeters(50), .y = ccad::millimeters(4)};
  require(hasCode(ccad::runDrc(graphic_outside), "BOARD_GRAPHIC_OUTSIDE_BOARD"),
          "drc reports board graphic outside outline");

  ccad::Project unsupported_graphic_kind = valid_board_graphic_text;
  unsupported_graphic_kind.boards[0].graphics.at(0).kind = "arc";
  require(hasCode(ccad::runDrc(unsupported_graphic_kind), "UNSUPPORTED_BOARD_GRAPHIC_KIND"),
          "drc reports unsupported board graphic kind");

  ccad::Project empty_text_id = valid_board_graphic_text;
  empty_text_id.boards[0].texts.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_text_id), "INVALID_BOARD_TEXT_ID"),
          "drc reports empty board text id");

  ccad::Project duplicate_text_id = valid_board_graphic_text;
  duplicate_text_id.boards[0].texts.push_back(duplicate_text_id.boards[0].texts.front());
  require(hasCode(ccad::runDrc(duplicate_text_id), "DUPLICATE_BOARD_TEXT_ID"),
          "drc reports duplicate board text ids");

  ccad::Project unknown_text_layer = valid_board_graphic_text;
  unknown_text_layer.boards[0].texts.at(0).layer_id = "Missing.SilkS";
  require(hasCode(ccad::runDrc(unknown_text_layer), "UNKNOWN_BOARD_TEXT_LAYER"),
          "drc reports board text unknown layer");

  ccad::Project empty_text_value = valid_board_graphic_text;
  empty_text_value.boards[0].texts.at(0).text.clear();
  require(hasCode(ccad::runDrc(empty_text_value), "EMPTY_BOARD_TEXT"),
          "drc reports empty board text value");

  ccad::Project invalid_text_size = valid_board_graphic_text;
  invalid_text_size.boards[0].texts.at(0).size.width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_text_size), "INVALID_BOARD_TEXT_SIZE"),
          "drc reports non-positive board text size");

  ccad::Project text_outside = valid_board_graphic_text;
  text_outside.boards[0].texts.at(0).position =
      ccad::Point{.x = ccad::millimeters(50), .y = ccad::millimeters(20)};
  require(hasCode(ccad::runDrc(text_outside), "BOARD_TEXT_OUTSIDE_BOARD"),
          "drc reports board text outside outline");

  ccad::Project graphic_duplicate_physical_object_id = valid_board_graphic_text;
  graphic_duplicate_physical_object_id.boards[0].graphics.at(0).id = "P1";
  require(hasCode(ccad::runDrc(graphic_duplicate_physical_object_id),
                  "DUPLICATE_PHYSICAL_OBJECT_ID"),
          "drc reports board graphic id reused by another physical object");

  ccad::Project text_duplicate_physical_object_id = valid_board_graphic_text;
  text_duplicate_physical_object_id.boards[0].texts.at(0).id = "P1";
  require(hasCode(ccad::runDrc(text_duplicate_physical_object_id),
                  "DUPLICATE_PHYSICAL_OBJECT_ID"),
          "drc reports board text id reused by another physical object");

  ccad::Project valid_route_request = validBoardProject();
  valid_route_request.boards[0].route_requests.push_back(ccad::RouteRequest{
      .id = "RR1",
      .net_id = "N1",
      .from_object_id = "P1",
      .to_object_id = "V1",
      .preferred_layer_id = "F.Cu",
      .policy = "shortest_safe",
      .width = ccad::millimeters(0.25),
  });
  require(ccad::runDrc(valid_route_request).empty(), "valid route request has no drc diagnostics");

  ccad::Project duplicate_route_request = valid_route_request;
  duplicate_route_request.boards[0].route_requests.push_back(
      duplicate_route_request.boards[0].route_requests.front());
  require(hasCode(ccad::runDrc(duplicate_route_request), "DUPLICATE_ROUTE_REQUEST_ID"),
          "drc reports duplicate route request id");

  ccad::Project empty_route_request_id = valid_route_request;
  empty_route_request_id.boards[0].route_requests.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_route_request_id), "INVALID_ROUTE_REQUEST_ID"),
          "drc reports empty route request id");

  ccad::Project unknown_route_request_net = valid_route_request;
  unknown_route_request_net.boards[0].route_requests.at(0).net_id = "NO_NET";
  require(hasCode(ccad::runDrc(unknown_route_request_net), "UNKNOWN_ROUTE_REQUEST_NET"),
          "drc reports unknown route request net");

  ccad::Project unknown_route_request_layer = valid_route_request;
  unknown_route_request_layer.boards[0].route_requests.at(0).preferred_layer_id = "Inner.Cu";
  require(hasCode(ccad::runDrc(unknown_route_request_layer), "UNKNOWN_ROUTE_REQUEST_LAYER"),
          "drc reports unknown route request preferred layer");

  ccad::Project non_copper_route_request_layer = valid_route_request;
  non_copper_route_request_layer.boards[0].layers.push_back(
      ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen"});
  non_copper_route_request_layer.boards[0].route_requests.at(0).preferred_layer_id = "F.SilkS";
  require(hasCode(ccad::runDrc(non_copper_route_request_layer),
                  "ROUTE_REQUEST_NON_COPPER_LAYER"),
          "drc reports route request on non-copper preferred layer");

  ccad::Project missing_route_request_endpoint = valid_route_request;
  missing_route_request_endpoint.boards[0].route_requests.at(0).to_object_id = "NO_OBJECT";
  require(hasCode(ccad::runDrc(missing_route_request_endpoint),
                  "UNKNOWN_ROUTE_REQUEST_ENDPOINT"),
          "drc reports unknown route request endpoint object");

  ccad::Project invalid_route_request_width = valid_route_request;
  invalid_route_request_width.boards[0].route_requests.at(0).width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_route_request_width), "INVALID_ROUTE_REQUEST_WIDTH"),
          "drc reports non-positive route request width");

  ccad::Project narrow_route_request_width = valid_route_request;
  narrow_route_request_width.boards[0].route_requests.at(0).width = ccad::millimeters(0.05);
  require(hasCode(ccad::runDrc(narrow_route_request_width),
                  "ROUTE_REQUEST_WIDTH_TOO_NARROW"),
          "drc reports route request width below configured minimum");

  ccad::Project same_endpoint_route_request = valid_route_request;
  same_endpoint_route_request.boards[0].route_requests.at(0).to_object_id = "P1";
  require(hasCode(ccad::runDrc(same_endpoint_route_request), "ROUTE_REQUEST_SAME_ENDPOINT"),
          "drc reports route request with same endpoint object");

  ccad::Project endpoint_net_mismatch_route_request = valid_route_request;
  endpoint_net_mismatch_route_request.boards[0].vias.at(0).net_id = "N2";
  require(hasCode(ccad::runDrc(endpoint_net_mismatch_route_request),
                  "ROUTE_REQUEST_ENDPOINT_NET_MISMATCH"),
          "drc reports route request endpoint net mismatch");

  ccad::Project empty_policy_route_request = valid_route_request;
  empty_policy_route_request.boards[0].route_requests.at(0).policy.clear();
  require(hasCode(ccad::runDrc(empty_policy_route_request), "INVALID_ROUTE_REQUEST_POLICY"),
          "drc reports empty route request policy");

  ccad::Project empty_pad_id = validBoardProject();
  empty_pad_id.boards[0].pads.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_pad_id), "INVALID_PAD_ID"),
          "drc reports empty pad id");

  ccad::Project empty_pad_component = validBoardProject();
  empty_pad_component.boards[0].pads.at(0).component_id.clear();
  require(hasCode(ccad::runDrc(empty_pad_component), "INVALID_PAD_COMPONENT"),
          "drc reports empty pad component id");

  ccad::Project empty_pad_pin = validBoardProject();
  empty_pad_pin.boards[0].pads.at(0).pin_name.clear();
  require(hasCode(ccad::runDrc(empty_pad_pin), "INVALID_PAD_PIN"),
          "drc reports empty pad pin name");

  ccad::Project unknown_pad_component = validBoardProject();
  unknown_pad_component.boards[0].pads.at(0).component_id = "U404";
  require(hasCode(ccad::runDrc(unknown_pad_component), "UNKNOWN_PAD_COMPONENT"),
          "drc reports pad unknown component reference");

  ccad::Project unknown_pad_pin = validBoardProject();
  unknown_pad_pin.boards[0].pads.at(0).pin_name = "404";
  require(hasCode(ccad::runDrc(unknown_pad_pin), "UNKNOWN_PAD_PIN"),
          "drc reports pad unknown component pin reference");

  ccad::Project pad_geometry_outside = validBoardProject();
  pad_geometry_outside.boards[0].pads.at(0).position =
      ccad::Point{.x = ccad::millimeters(0.4), .y = ccad::millimeters(0.4)};
  require(hasCode(ccad::runDrc(pad_geometry_outside), "PAD_GEOMETRY_OUTSIDE_BOARD"),
          "drc reports pad geometry outside board");

  ccad::Project unconnected_pad = validBoardProject();
  unconnected_pad.boards[0].pads.at(0).net_id.clear();
  require(hasDiagnostic(ccad::runDrc(unconnected_pad), "UNCONNECTED_PAD", "warning"),
          "drc reports unconnected pad as warning");

  ccad::Project unconnected_via = validBoardProject();
  unconnected_via.boards[0].vias.at(0).net_id.clear();
  require(hasDiagnostic(ccad::runDrc(unconnected_via), "UNCONNECTED_VIA", "warning"),
          "drc reports unconnected via as warning");

  ccad::Project empty_via_id = validBoardProject();
  empty_via_id.boards[0].vias.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_via_id), "INVALID_VIA_ID"),
          "drc reports empty via id");

  ccad::Project unconnected_track = validBoardProject();
  unconnected_track.boards[0].tracks.at(0).net_id.clear();
  require(hasDiagnostic(ccad::runDrc(unconnected_track), "UNCONNECTED_TRACK", "warning"),
          "drc reports unconnected track as warning");

  ccad::Project empty_track_id = validBoardProject();
  empty_track_id.boards[0].tracks.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_track_id), "INVALID_TRACK_ID"),
          "drc reports empty track id");

  ccad::Project unknown_pad_net = validBoardProject();
  unknown_pad_net.boards[0].pads.at(0).net_id = "N404";
  require(hasCode(ccad::runDrc(unknown_pad_net), "UNKNOWN_PAD_NET"),
          "drc reports unknown pad net");

  ccad::Project pad_net_member_mismatch = validBoardProject();
  pad_net_member_mismatch.boards[0].pads.at(0).net_id = "N2";
  require(hasCode(ccad::runDrc(pad_net_member_mismatch), "PAD_NET_MEMBER_MISMATCH"),
          "drc reports pad net/member mismatch");

  ccad::Project unknown_via_net = validBoardProject();
  unknown_via_net.boards[0].vias.at(0).net_id = "N404";
  require(hasCode(ccad::runDrc(unknown_via_net), "UNKNOWN_VIA_NET"),
          "drc reports unknown via net");

  ccad::Project unknown_track_net = validBoardProject();
  unknown_track_net.boards[0].tracks.at(0).net_id = "N404";
  require(hasCode(ccad::runDrc(unknown_track_net), "UNKNOWN_TRACK_NET"),
          "drc reports unknown track net");

  ccad::Project dangling_track = validBoardProject();
  dangling_track.boards[0].tracks.at(0).end = ccad::Point{.x = ccad::millimeters(10),
                                                       .y = ccad::millimeters(9)};
  require(hasDiagnostic(ccad::runDrc(dangling_track), "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc reports dangling track endpoint as warning");

  ccad::Project via_contact_track = validBoardProject();
  via_contact_track.boards[0].tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(7.6), .y = ccad::millimeters(9)};
  via_contact_track.boards[0].tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)};
  require(!hasDiagnostic(ccad::runDrc(via_contact_track), "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc accepts same-net track endpoint touching via copper area");

  ccad::Project pad_contact_track = validBoardProject();
  pad_contact_track.boards[0].tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(4.3), .y = ccad::millimeters(6)};
  pad_contact_track.boards[0].tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)};
  require(!hasDiagnostic(ccad::runDrc(pad_contact_track), "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc accepts same-net track endpoint touching pad copper area");

  ccad::Project cross_layer_pad_contact_track = validBoardProject();
  cross_layer_pad_contact_track.boards[0].tracks.at(0).layer_id = "B.Cu";
  cross_layer_pad_contact_track.boards[0].tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(4.3), .y = ccad::millimeters(6)};
  cross_layer_pad_contact_track.boards[0].tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)};
  require(hasDiagnostic(ccad::runDrc(cross_layer_pad_contact_track),
                        "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc keeps track-pad connectivity layer-aware without via");

  ccad::Project segment_contact_track = validBoardProject();
  segment_contact_track.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T_STUB",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(6.5), .y = ccad::millimeters(7.5)},
      .end = ccad::Point{.x = ccad::millimeters(8.0), .y = ccad::millimeters(9.0)},
      .width = ccad::millimeters(0.25)});
  require(
      !hasDiagnostic(ccad::runDrc(segment_contact_track), "UNCONNECTED_TRACK_ENDPOINT", "warning"),
      "drc accepts same-net track endpoint touching another same-net track segment interior");

  ccad::Project cross_layer_segment_contact_track = validBoardProject();
  cross_layer_segment_contact_track.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T_STUB_B",
      .net_id = "N1",
      .layer_id = "B.Cu",
      .start = ccad::Point{.x = ccad::millimeters(6.5), .y = ccad::millimeters(7.5)},
      .end = ccad::Point{.x = ccad::millimeters(8.0), .y = ccad::millimeters(9.0)},
      .width = ccad::millimeters(0.25)});
  require(hasDiagnostic(ccad::runDrc(cross_layer_segment_contact_track),
                        "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc keeps track-segment connectivity layer-aware without via");

  ccad::Project keepout_pad = validBoardProject();
  keepout_pad.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_PAD",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(5)},
                         .size = ccad::Size{.width = ccad::millimeters(3),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(keepout_pad), "PAD_IN_KEEPOUT"),
          "drc reports pad in keepout");

  ccad::Project invalid_pad_size_in_keepout = keepout_pad;
  invalid_pad_size_in_keepout.boards[0].pads.at(0).padstack.copper_props["top"].shape.size.width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_pad_size_in_keepout), "INVALID_PAD_SIZE"),
          "drc reports invalid pad size before keepout geometry");
  require(!hasCode(ccad::runDrc(invalid_pad_size_in_keepout), "PAD_IN_KEEPOUT"),
          "drc does not report pad keepout geometry when pad size is invalid");

  ccad::Project keepout_pad_geometry = validBoardProject();
  keepout_pad_geometry.boards[0].pads.at(0).position =
      ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(5)};
  keepout_pad_geometry.boards[0].pads.at(0).padstack.copper_props["top"].shape.size =
      ccad::Size{.width = ccad::millimeters(2.0), .height = ccad::millimeters(2.0)};
  keepout_pad_geometry.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_PAD_GEOM",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(5.9),
                                               .y = ccad::millimeters(5.9)},
                         .size = ccad::Size{.width = ccad::millimeters(1.0),
                                            .height = ccad::millimeters(1.0)}}});
  require(hasCode(ccad::runDrc(keepout_pad_geometry), "PAD_IN_KEEPOUT"),
          "drc reports pad geometry intersection with keepout");

  ccad::Project keepout_via = validBoardProject();
  keepout_via.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_VIA",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(7),
                                               .y = ccad::millimeters(8)},
                         .size = ccad::Size{.width = ccad::millimeters(3),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(keepout_via), "VIA_IN_KEEPOUT"),
          "drc reports via in keepout");

  ccad::Project invalid_via_size_in_keepout = keepout_via;
  invalid_via_size_in_keepout.boards[0].vias.at(0).diameter = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_via_size_in_keepout), "INVALID_VIA_SIZE"),
          "drc reports invalid via size before keepout geometry");
  require(!hasCode(ccad::runDrc(invalid_via_size_in_keepout), "VIA_IN_KEEPOUT"),
          "drc does not report via keepout geometry when via size is invalid");

  ccad::Project keepout_via_geometry = validBoardProject();
  keepout_via_geometry.boards[0].vias.at(0).position =
      ccad::Point{.x = ccad::millimeters(10.4), .y = ccad::millimeters(9.0)};
  keepout_via_geometry.boards[0].vias.at(0).diameter = ccad::millimeters(1.0);
  keepout_via_geometry.boards[0].vias.at(0).drill = ccad::millimeters(0.4);
  keepout_via_geometry.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_VIA_GEOM",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(9.9),
                                               .y = ccad::millimeters(8.8)},
                         .size = ccad::Size{.width = ccad::millimeters(0.4),
                                            .height = ccad::millimeters(0.4)}}});
  require(hasCode(ccad::runDrc(keepout_via_geometry), "VIA_IN_KEEPOUT"),
          "drc reports via geometry intersection with keepout");

  ccad::Project keepout_track = validBoardProject();
  keepout_track.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_TRACK",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(5)},
                         .size = ccad::Size{.width = ccad::millimeters(3),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(keepout_track), "TRACK_ENDPOINT_IN_KEEPOUT"),
          "drc reports track endpoint in keepout");

  ccad::Project invalid_track_width_in_keepout = keepout_track;
  invalid_track_width_in_keepout.boards[0].tracks.at(0).width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_track_width_in_keepout), "INVALID_TRACK_WIDTH"),
          "drc reports invalid track width before keepout geometry");
  require(!hasCode(ccad::runDrc(invalid_track_width_in_keepout), "TRACK_ENDPOINT_IN_KEEPOUT"),
          "drc does not report track keepout geometry when track width is invalid");

  ccad::Project keepout_track_crossing = validBoardProject();
  keepout_track_crossing.boards[0].tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(12)};
  keepout_track_crossing.boards[0].tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(12)};
  keepout_track_crossing.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_CROSS",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(6),
                                               .y = ccad::millimeters(10)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(4)}}});
  require(hasCode(ccad::runDrc(keepout_track_crossing), "TRACK_CROSSES_KEEPOUT"),
          "drc reports track crossing keepout with endpoints outside");

  ccad::Project keepout_track_width_overlap = validBoardProject();
  keepout_track_width_overlap.boards[0].tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(11.65)};
  keepout_track_width_overlap.boards[0].tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(11.65)};
  keepout_track_width_overlap.boards[0].tracks.at(0).width = ccad::millimeters(0.6);
  keepout_track_width_overlap.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_WIDTH_OVERLAP",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(6),
                                               .y = ccad::millimeters(10)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(1.5)}}});
  require(hasCode(ccad::runDrc(keepout_track_width_overlap), "TRACK_CROSSES_KEEPOUT"),
          "drc reports keepout crossing when only track width overlaps keepout");

  ccad::Project duplicate_keepout = validBoardProject();
  duplicate_keepout.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_DUP",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(3),
                                               .y = ccad::millimeters(3)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  duplicate_keepout.boards[0].keepouts.push_back(duplicate_keepout.boards[0].keepouts.back());
  require(hasCode(ccad::runDrc(duplicate_keepout), "DUPLICATE_KEEPOUT_ID"),
          "drc reports duplicate keepout ids");

  ccad::Project invalid_keepout_size = validBoardProject();
  invalid_keepout_size.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_SIZE",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::millimeters(0),
                                            .height = ccad::millimeters(1)}}});
  require(hasCode(ccad::runDrc(invalid_keepout_size), "INVALID_KEEPOUT_SIZE"),
          "drc reports invalid keepout size");

  ccad::Project keepout_outside = validBoardProject();
  keepout_outside.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_OUT",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(41),
                                               .y = ccad::millimeters(27)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  require(hasCode(ccad::runDrc(keepout_outside), "KEEPOUT_OUTSIDE_BOARD"),
          "drc reports keepout area outside board");

  ccad::Project unknown_keepout_kind = validBoardProject();
  unknown_keepout_kind.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K_KIND",
      .kind = "thermal",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(4)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  require(hasCode(ccad::runDrc(unknown_keepout_kind), "UNKNOWN_KEEPOUT_KIND"),
          "drc reports unknown keepout kind");

  ccad::Project empty_keepout_id = validBoardProject();
  empty_keepout_id.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(4)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  require(hasCode(ccad::runDrc(empty_keepout_id), "INVALID_KEEPOUT_ID"),
          "drc reports empty keepout id");

  ccad::Project duplicate_placement_region = validBoardProject();
  duplicate_placement_region.boards[0].placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR1",
      .kind = "component",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::millimeters(5),
                                            .height = ccad::millimeters(5)}}});
  duplicate_placement_region.boards[0].placement_regions.push_back(
      duplicate_placement_region.boards[0].placement_regions.back());
  require(hasCode(ccad::runDrc(duplicate_placement_region),
                  "DUPLICATE_PLACEMENT_REGION_ID"),
          "drc reports duplicate placement region ids");

  ccad::Project invalid_placement_region_size = validBoardProject();
  invalid_placement_region_size.boards[0].placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR_BAD",
      .kind = "component",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::nanometers(0),
                                            .height = ccad::millimeters(5)}}});
  require(hasCode(ccad::runDrc(invalid_placement_region_size),
                  "INVALID_PLACEMENT_REGION_SIZE"),
          "drc reports invalid placement region size");

  ccad::Project placement_region_outside = validBoardProject();
  placement_region_outside.boards[0].placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR_OUT",
      .kind = "component",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(40),
                                               .y = ccad::millimeters(26)},
                         .size = ccad::Size{.width = ccad::millimeters(4),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(placement_region_outside), "PLACEMENT_REGION_OUTSIDE_BOARD"),
          "drc reports placement region area outside board");

  ccad::Project unknown_placement_region_kind = validBoardProject();
  unknown_placement_region_kind.boards[0].placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR_KIND",
      .kind = "mystery",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::millimeters(5),
                                            .height = ccad::millimeters(5)}}});
  require(hasCode(ccad::runDrc(unknown_placement_region_kind),
                  "UNKNOWN_PLACEMENT_REGION_KIND"),
          "drc reports unknown placement region kind");

  ccad::Project empty_placement_region_id = validBoardProject();
  empty_placement_region_id.boards[0].placement_regions.push_back(ccad::PlacementRegion{
      .id = "",
      .kind = "component",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::millimeters(5),
                                            .height = ccad::millimeters(5)}}});
  require(hasCode(ccad::runDrc(empty_placement_region_id), "INVALID_PLACEMENT_REGION_ID"),
          "drc reports empty placement region id");

  ccad::Project same_net_touching_track = validBoardProject();
  same_net_touching_track.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T_SAME",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
      .end = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(10)},
      .width = ccad::millimeters(0.25)});
  require(!hasCode(ccad::runDrc(same_net_touching_track), "COPPER_CLEARANCE"),
          "drc allows same-net copper to touch");

  ccad::Project pad_clearance = validBoardProject();
  pad_clearance.boards[0].pads.push_back(ccad::Pad{
      .id = "P2",
      .component_id = "U2",
      .pin_name = "1",
      .net_id = "N2",
      .type = "smd",
      .position = ccad::Point{.x = ccad::millimeters(6.35), .y = ccad::millimeters(6)},
      .padstack = ccad::Padstack{
         .layer_set = {"F.Cu"},
         .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
      }});
  require(hasDiagnosticForObject(ccad::runDrc(pad_clearance), "COPPER_CLEARANCE", "P2"),
          "drc reports different-net pads closer than configured clearance");
  require(hasDiagnosticMessageContaining(ccad::runDrc(pad_clearance), "COPPER_CLEARANCE",
                                         "configured clearance 200000 nm"),
          "drc clearance diagnostic includes configured clearance value");

  ccad::Project invalid_pad_size_clearance = pad_clearance;
  invalid_pad_size_clearance.boards[0].pads.back().position.x = ccad::millimeters(5.85);
  invalid_pad_size_clearance.boards[0].pads.back().padstack.copper_props["top"].shape.size.width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_pad_size_clearance), "INVALID_PAD_SIZE"),
          "drc reports invalid pad size before clearance geometry");
  require(!hasDiagnosticForObject(ccad::runDrc(invalid_pad_size_clearance), "COPPER_CLEARANCE",
                                  "P2"),
          "drc does not report copper clearance for invalid pad geometry");

  ccad::Project cross_layer_pad_clearance = pad_clearance;
  cross_layer_pad_clearance.boards[0].pads.back().padstack.layer_set = {"B.Cu"};
  require(!hasDiagnosticForObject(ccad::runDrc(cross_layer_pad_clearance), "COPPER_CLEARANCE",
                                  "P2"),
          "drc allows different-net pads to overlap on different copper layers");

  ccad::Project relaxed_clearance = pad_clearance;
  relaxed_clearance.boards[0].design_rules.copper_clearance = ccad::millimeters(0.04);
  require(!hasDiagnosticForObject(ccad::runDrc(relaxed_clearance), "COPPER_CLEARANCE", "P2"),
          "drc obeys configured copper clearance");

  ccad::Project crossing_tracks = validBoardProject();
  crossing_tracks.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T2",
      .net_id = "N2",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(4), .y = ccad::millimeters(9)},
      .end = ccad::Point{.x = ccad::millimeters(9), .y = ccad::millimeters(4)},
      .width = ccad::millimeters(0.25)});
  require(hasDiagnosticForObject(ccad::runDrc(crossing_tracks), "COPPER_CLEARANCE", "T2"),
          "drc reports crossing different-net tracks on same layer");

  ccad::Project invalid_track_width_clearance = validBoardProject();
  invalid_track_width_clearance.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "T_BAD",
      .net_id = "N2",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(5)},
      .end = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(7)},
      .width = ccad::nanometers(0)});
  require(hasCode(ccad::runDrc(invalid_track_width_clearance), "INVALID_TRACK_WIDTH"),
          "drc reports invalid track width before clearance geometry");
  require(!hasDiagnosticForObject(ccad::runDrc(invalid_track_width_clearance),
                                  "COPPER_CLEARANCE", "T_BAD"),
          "drc does not report copper clearance for invalid track geometry");

  ccad::Project cross_layer_crossing_tracks = crossing_tracks;
  cross_layer_crossing_tracks.boards[0].tracks.back().layer_id = "B.Cu";
  require(!hasDiagnosticForObject(ccad::runDrc(cross_layer_crossing_tracks), "COPPER_CLEARANCE",
                                  "T2"),
          "drc allows different-net tracks to cross on different copper layers");

  ccad::Project via_cross_layer_clearance = cross_layer_pad_clearance;
  via_cross_layer_clearance.boards[0].vias.push_back(ccad::Via{
      .id = "V2",
      .net_id = "N2",
      .position = ccad::Point{.x = ccad::millimeters(5.1), .y = ccad::millimeters(6)},
      .diameter = ccad::millimeters(0.8),
      .drill = ccad::millimeters(0.4)});
  require(hasDiagnosticForObject(ccad::runDrc(via_cross_layer_clearance), "COPPER_CLEARANCE",
                                 "V2"),
          "drc still checks via clearance against layer-bound copper");

  ccad::Project invalid_via_size_clearance = validBoardProject();
  invalid_via_size_clearance.boards[0].vias.push_back(ccad::Via{
      .id = "V_BAD",
      .net_id = "N2",
      .position = ccad::Point{.x = ccad::millimeters(5.1), .y = ccad::millimeters(6)},
      .diameter = ccad::nanometers(0),
      .drill = ccad::millimeters(0.4)});
  require(hasCode(ccad::runDrc(invalid_via_size_clearance), "INVALID_VIA_SIZE"),
          "drc reports invalid via size before clearance geometry");
  require(!hasDiagnosticForObject(ccad::runDrc(invalid_via_size_clearance),
                                  "COPPER_CLEARANCE", "V_BAD"),
          "drc does not report copper clearance for invalid via geometry");

  ccad::Project through_hole_without_drill = validBoardProject();
  through_hole_without_drill.boards[0].pads[0].type = "through_hole";
  require(hasDiagnosticForObject(ccad::runDrc(through_hole_without_drill),
                                 "PAD_THROUGH_HOLE_WITHOUT_DRILL", "P1"),
          "drc reports through-hole pad without drill geometry");

  ccad::Project pad_drill_below_minimum = validBoardProject();
  pad_drill_below_minimum.boards[0].pads[0].type = "through_hole";
  pad_drill_below_minimum.boards[0].pads[0].padstack.drill.size.width = ccad::millimeters(0.2);
  pad_drill_below_minimum.boards[0].pads[0].padstack.drill.size.height = ccad::millimeters(0.2);
  pad_drill_below_minimum.boards[0].design_rules.min_through_hole_drill = ccad::millimeters(0.3);
  require(hasDiagnosticForObject(ccad::runDrc(pad_drill_below_minimum),
                                 "PAD_DRILL_BELOW_MINIMUM", "P1"),
          "drc reports through-hole pad drill below configured minimum");

  ccad::Project pad_edge = validBoardProject();
  pad_edge.boards[0].pads[0].position.x = ccad::millimeters(1.0);
  require(hasDiagnosticForObject(ccad::runDrc(pad_edge), "PAD_EDGE_CLEARANCE", "P1"),
          "drc reports pad copper too close to board edge");

  ccad::Project close_holes = validBoardProject();
  close_holes.boards[0].vias.push_back(ccad::Via{
      .id = "V2", .net_id = "N2", .position = ccad::Point{.x = ccad::millimeters(8.45), .y = ccad::millimeters(9)},
      .diameter = ccad::millimeters(0.8), .drill = ccad::millimeters(0.4)});
  require(hasDiagnosticForObject(ccad::runDrc(close_holes), "DRILLED_HOLES_TOO_CLOSE", "V2"),
          "drc reports via holes below hole-to-hole clearance");
}
