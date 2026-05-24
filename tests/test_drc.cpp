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
  project.components = {ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "1", .kind = "passive"}},
  }};
  project.nets = {ccad::Net{.id = "N1",
                            .members = {ccad::NetMember{.component_id = "U1", .pin_name = "1"}}},
                  ccad::Net{.id = "N2",
                            .members = {ccad::NetMember{.component_id = "U2", .pin_name = "1"}}}};
  project.board = ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                 ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true}},
      .placement_regions = {},
      .keepouts = {},
      .pads = {ccad::Pad{.id = "P1",
                         .component_id = "U1",
                         .pin_name = "1",
                         .net_id = "N1",
                         .layer_id = "F.Cu",
                         .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                         .size = ccad::Size{.width = ccad::millimeters(1.5),
                                            .height = ccad::millimeters(1.0)}}},
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
  };
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

  ccad::Project invalid_board_outline = validBoardProject();
  invalid_board_outline.board->outline.size.width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_board_outline), "INVALID_BOARD_OUTLINE"),
          "drc reports invalid board outline size");

  ccad::Project invalid_copper_clearance = validBoardProject();
  invalid_copper_clearance.board->design_rules.copper_clearance = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_copper_clearance), "INVALID_COPPER_CLEARANCE"),
          "drc reports non-positive copper clearance rule");

  ccad::Project invalid_min_track_width = validBoardProject();
  invalid_min_track_width.board->design_rules.min_track_width = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_min_track_width), "INVALID_MIN_TRACK_WIDTH"),
          "drc reports non-positive minimum track width rule");

  ccad::Project invalid_min_via_annular_ring = validBoardProject();
  invalid_min_via_annular_ring.board->design_rules.min_via_annular_ring = ccad::nanometers(0);
  require(hasCode(ccad::runDrc(invalid_min_via_annular_ring), "INVALID_MIN_VIA_ANNULAR_RING"),
          "drc reports non-positive minimum via annular ring rule");

  ccad::Project pad_outside = validBoardProject();
  pad_outside.board->pads.at(0).position.x = ccad::millimeters(99);
  require(hasCode(ccad::runDrc(pad_outside), "PAD_OUTSIDE_BOARD"),
          "drc reports pad outside board");

  ccad::Project unknown_track_layer = validBoardProject();
  unknown_track_layer.board->tracks.at(0).layer_id = "Inner.Cu";
  require(hasCode(ccad::runDrc(unknown_track_layer), "UNKNOWN_TRACK_LAYER"),
          "drc reports unknown track layer");

  ccad::Project non_copper_pad_layer = validBoardProject();
  non_copper_pad_layer.board->layers.push_back(
      ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen"});
  non_copper_pad_layer.board->pads.at(0).layer_id = "F.SilkS";
  require(hasCode(ccad::runDrc(non_copper_pad_layer), "PAD_NON_COPPER_LAYER"),
          "drc reports pad on non-copper layer");

  ccad::Project non_copper_track_layer = validBoardProject();
  non_copper_track_layer.board->layers.push_back(
      ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen"});
  non_copper_track_layer.board->tracks.at(0).layer_id = "F.SilkS";
  require(hasCode(ccad::runDrc(non_copper_track_layer), "TRACK_NON_COPPER_LAYER"),
          "drc reports track on non-copper layer");

  ccad::Project duplicate_layer = validBoardProject();
  duplicate_layer.board->layers.push_back(duplicate_layer.board->layers.front());
  require(hasCode(ccad::runDrc(duplicate_layer), "DUPLICATE_LAYER_ID"),
          "drc reports duplicate layer id");

  ccad::Project empty_layer_id = validBoardProject();
  empty_layer_id.board->layers.push_back(
      ccad::Layer{.id = "", .name = "Invalid", .kind = "copper", .visible = true});
  require(hasCode(ccad::runDrc(empty_layer_id), "INVALID_LAYER_ID"),
          "drc reports empty layer id");

  ccad::Project empty_layer_name = validBoardProject();
  empty_layer_name.board->layers.at(0).name.clear();
  require(hasCode(ccad::runDrc(empty_layer_name), "INVALID_LAYER_NAME"),
          "drc reports empty layer name");

  ccad::Project empty_layer_kind = validBoardProject();
  empty_layer_kind.board->layers.at(0).kind.clear();
  require(hasCode(ccad::runDrc(empty_layer_kind), "INVALID_LAYER_KIND"),
          "drc reports empty layer kind");

  ccad::Project duplicate_net = validBoardProject();
  duplicate_net.nets.push_back(duplicate_net.nets.front());
  require(hasCode(ccad::runDrc(duplicate_net), "DUPLICATE_NET_ID"),
          "drc reports duplicate net id");

  ccad::Project empty_net_id = validBoardProject();
  empty_net_id.nets.push_back(
      ccad::Net{.id = "", .members = {ccad::NetMember{.component_id = "U3", .pin_name = "1"}}});
  require(hasCode(ccad::runDrc(empty_net_id), "INVALID_NET_ID"),
          "drc reports empty net id");

  ccad::Project invalid_net_member = validBoardProject();
  invalid_net_member.nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "", .pin_name = "2"});
  require(hasCode(ccad::runDrc(invalid_net_member), "INVALID_NET_MEMBER"),
          "drc reports invalid net member fields");

  ccad::Project duplicate_net_member = validBoardProject();
  duplicate_net_member.nets.at(0).members.push_back(
      duplicate_net_member.nets.at(0).members.front());
  require(hasCode(ccad::runDrc(duplicate_net_member), "DUPLICATE_NET_MEMBER"),
          "drc reports duplicate net member mapping");

  ccad::Project via_drill_too_large = validBoardProject();
  via_drill_too_large.board->vias.at(0).drill = ccad::millimeters(1.0);
  require(hasCode(ccad::runDrc(via_drill_too_large), "VIA_DRILL_TOO_LARGE"),
          "drc reports via drill too large");

  ccad::Project via_geometry_outside = validBoardProject();
  via_geometry_outside.board->vias.at(0).position =
      ccad::Point{.x = ccad::millimeters(0.1), .y = ccad::millimeters(0.1)};
  require(hasCode(ccad::runDrc(via_geometry_outside), "VIA_GEOMETRY_OUTSIDE_BOARD"),
          "drc reports via copper geometry outside board");

  ccad::Project via_small_ring = validBoardProject();
  via_small_ring.board->vias.at(0).diameter = ccad::millimeters(0.45);
  via_small_ring.board->vias.at(0).drill = ccad::millimeters(0.4);
  require(hasCode(ccad::runDrc(via_small_ring), "VIA_ANNULAR_RING_TOO_SMALL"),
          "drc reports via annular ring below configured minimum");
  require(hasDiagnosticMessageContaining(ccad::runDrc(via_small_ring),
                                         "VIA_ANNULAR_RING_TOO_SMALL",
                                         "configured minimum 100000 nm"),
          "drc via annular ring diagnostic includes configured minimum value");

  ccad::Project relaxed_via_ring = via_small_ring;
  relaxed_via_ring.board->design_rules.min_via_annular_ring = ccad::millimeters(0.02);
  require(!hasCode(ccad::runDrc(relaxed_via_ring), "VIA_ANNULAR_RING_TOO_SMALL"),
          "drc obeys configured minimum via annular ring");

  ccad::Project zero_length_track = validBoardProject();
  zero_length_track.board->tracks.at(0).end = zero_length_track.board->tracks.at(0).start;
  require(hasCode(ccad::runDrc(zero_length_track), "ZERO_LENGTH_TRACK"),
          "drc reports zero length track");

  ccad::Project narrow_track = validBoardProject();
  narrow_track.board->tracks.at(0).width = ccad::millimeters(0.10);
  require(hasCode(ccad::runDrc(narrow_track), "TRACK_TOO_NARROW"),
          "drc reports track width below configured minimum");
  require(hasDiagnosticMessageContaining(ccad::runDrc(narrow_track), "TRACK_TOO_NARROW",
                                         "configured minimum 150000 nm"),
          "drc track width diagnostic includes configured minimum value");

  ccad::Project relaxed_track_width = narrow_track;
  relaxed_track_width.board->design_rules.min_track_width = ccad::millimeters(0.08);
  require(!hasCode(ccad::runDrc(relaxed_track_width), "TRACK_TOO_NARROW"),
          "drc obeys configured minimum track width");

  ccad::Project track_geometry_outside = validBoardProject();
  track_geometry_outside.board->tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(0.05), .y = ccad::millimeters(6)};
  track_geometry_outside.board->tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)};
  require(hasCode(ccad::runDrc(track_geometry_outside), "TRACK_GEOMETRY_OUTSIDE_BOARD"),
          "drc reports track copper geometry outside board");

  ccad::Project duplicate_pad = validBoardProject();
  duplicate_pad.board->pads.push_back(duplicate_pad.board->pads.at(0));
  require(hasCode(ccad::runDrc(duplicate_pad), "DUPLICATE_PAD_ID"),
          "drc reports duplicate pad id");

  ccad::Project duplicate_physical_object_id = validBoardProject();
  duplicate_physical_object_id.board->vias.at(0).id = "P1";
  require(hasCode(ccad::runDrc(duplicate_physical_object_id), "DUPLICATE_PHYSICAL_OBJECT_ID"),
          "drc reports physical object id reused across types");

  ccad::Project empty_pad_id = validBoardProject();
  empty_pad_id.board->pads.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_pad_id), "INVALID_PAD_ID"),
          "drc reports empty pad id");

  ccad::Project empty_pad_component = validBoardProject();
  empty_pad_component.board->pads.at(0).component_id.clear();
  require(hasCode(ccad::runDrc(empty_pad_component), "INVALID_PAD_COMPONENT"),
          "drc reports empty pad component id");

  ccad::Project empty_pad_pin = validBoardProject();
  empty_pad_pin.board->pads.at(0).pin_name.clear();
  require(hasCode(ccad::runDrc(empty_pad_pin), "INVALID_PAD_PIN"),
          "drc reports empty pad pin name");

  ccad::Project unknown_pad_component = validBoardProject();
  unknown_pad_component.board->pads.at(0).component_id = "U404";
  require(hasCode(ccad::runDrc(unknown_pad_component), "UNKNOWN_PAD_COMPONENT"),
          "drc reports pad unknown component reference");

  ccad::Project unknown_pad_pin = validBoardProject();
  unknown_pad_pin.board->pads.at(0).pin_name = "404";
  require(hasCode(ccad::runDrc(unknown_pad_pin), "UNKNOWN_PAD_PIN"),
          "drc reports pad unknown component pin reference");

  ccad::Project pad_geometry_outside = validBoardProject();
  pad_geometry_outside.board->pads.at(0).position =
      ccad::Point{.x = ccad::millimeters(0.4), .y = ccad::millimeters(0.4)};
  require(hasCode(ccad::runDrc(pad_geometry_outside), "PAD_GEOMETRY_OUTSIDE_BOARD"),
          "drc reports pad geometry outside board");

  ccad::Project unconnected_pad = validBoardProject();
  unconnected_pad.board->pads.at(0).net_id.clear();
  require(hasDiagnostic(ccad::runDrc(unconnected_pad), "UNCONNECTED_PAD", "warning"),
          "drc reports unconnected pad as warning");

  ccad::Project unconnected_via = validBoardProject();
  unconnected_via.board->vias.at(0).net_id.clear();
  require(hasDiagnostic(ccad::runDrc(unconnected_via), "UNCONNECTED_VIA", "warning"),
          "drc reports unconnected via as warning");

  ccad::Project empty_via_id = validBoardProject();
  empty_via_id.board->vias.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_via_id), "INVALID_VIA_ID"),
          "drc reports empty via id");

  ccad::Project unconnected_track = validBoardProject();
  unconnected_track.board->tracks.at(0).net_id.clear();
  require(hasDiagnostic(ccad::runDrc(unconnected_track), "UNCONNECTED_TRACK", "warning"),
          "drc reports unconnected track as warning");

  ccad::Project empty_track_id = validBoardProject();
  empty_track_id.board->tracks.at(0).id.clear();
  require(hasCode(ccad::runDrc(empty_track_id), "INVALID_TRACK_ID"),
          "drc reports empty track id");

  ccad::Project unknown_pad_net = validBoardProject();
  unknown_pad_net.board->pads.at(0).net_id = "N404";
  require(hasCode(ccad::runDrc(unknown_pad_net), "UNKNOWN_PAD_NET"),
          "drc reports unknown pad net");

  ccad::Project pad_net_member_mismatch = validBoardProject();
  pad_net_member_mismatch.board->pads.at(0).net_id = "N2";
  require(hasCode(ccad::runDrc(pad_net_member_mismatch), "PAD_NET_MEMBER_MISMATCH"),
          "drc reports pad net/member mismatch");

  ccad::Project unknown_via_net = validBoardProject();
  unknown_via_net.board->vias.at(0).net_id = "N404";
  require(hasCode(ccad::runDrc(unknown_via_net), "UNKNOWN_VIA_NET"),
          "drc reports unknown via net");

  ccad::Project unknown_track_net = validBoardProject();
  unknown_track_net.board->tracks.at(0).net_id = "N404";
  require(hasCode(ccad::runDrc(unknown_track_net), "UNKNOWN_TRACK_NET"),
          "drc reports unknown track net");

  ccad::Project dangling_track = validBoardProject();
  dangling_track.board->tracks.at(0).end = ccad::Point{.x = ccad::millimeters(10),
                                                       .y = ccad::millimeters(9)};
  require(hasDiagnostic(ccad::runDrc(dangling_track), "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc reports dangling track endpoint as warning");

  ccad::Project via_contact_track = validBoardProject();
  via_contact_track.board->tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(7.6), .y = ccad::millimeters(9)};
  via_contact_track.board->tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)};
  require(!hasDiagnostic(ccad::runDrc(via_contact_track), "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc accepts same-net track endpoint touching via copper area");

  ccad::Project pad_contact_track = validBoardProject();
  pad_contact_track.board->tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(4.3), .y = ccad::millimeters(6)};
  pad_contact_track.board->tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)};
  require(!hasDiagnostic(ccad::runDrc(pad_contact_track), "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc accepts same-net track endpoint touching pad copper area");

  ccad::Project cross_layer_pad_contact_track = validBoardProject();
  cross_layer_pad_contact_track.board->tracks.at(0).layer_id = "B.Cu";
  cross_layer_pad_contact_track.board->tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(4.3), .y = ccad::millimeters(6)};
  cross_layer_pad_contact_track.board->tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)};
  require(hasDiagnostic(ccad::runDrc(cross_layer_pad_contact_track),
                        "UNCONNECTED_TRACK_ENDPOINT", "warning"),
          "drc keeps track-pad connectivity layer-aware without via");

  ccad::Project segment_contact_track = validBoardProject();
  segment_contact_track.board->tracks.push_back(ccad::TrackSegment{
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
  cross_layer_segment_contact_track.board->tracks.push_back(ccad::TrackSegment{
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
  keepout_pad.board->keepouts.push_back(ccad::Keepout{
      .id = "K_PAD",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(5)},
                         .size = ccad::Size{.width = ccad::millimeters(3),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(keepout_pad), "PAD_IN_KEEPOUT"),
          "drc reports pad in keepout");

  ccad::Project keepout_pad_geometry = validBoardProject();
  keepout_pad_geometry.board->pads.at(0).position =
      ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(5)};
  keepout_pad_geometry.board->pads.at(0).size =
      ccad::Size{.width = ccad::millimeters(2.0), .height = ccad::millimeters(2.0)};
  keepout_pad_geometry.board->keepouts.push_back(ccad::Keepout{
      .id = "K_PAD_GEOM",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(5.9),
                                               .y = ccad::millimeters(5.9)},
                         .size = ccad::Size{.width = ccad::millimeters(1.0),
                                            .height = ccad::millimeters(1.0)}}});
  require(hasCode(ccad::runDrc(keepout_pad_geometry), "PAD_IN_KEEPOUT"),
          "drc reports pad geometry intersection with keepout");

  ccad::Project keepout_via = validBoardProject();
  keepout_via.board->keepouts.push_back(ccad::Keepout{
      .id = "K_VIA",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(7),
                                               .y = ccad::millimeters(8)},
                         .size = ccad::Size{.width = ccad::millimeters(3),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(keepout_via), "VIA_IN_KEEPOUT"),
          "drc reports via in keepout");

  ccad::Project keepout_via_geometry = validBoardProject();
  keepout_via_geometry.board->vias.at(0).position =
      ccad::Point{.x = ccad::millimeters(10.4), .y = ccad::millimeters(9.0)};
  keepout_via_geometry.board->vias.at(0).diameter = ccad::millimeters(1.0);
  keepout_via_geometry.board->vias.at(0).drill = ccad::millimeters(0.4);
  keepout_via_geometry.board->keepouts.push_back(ccad::Keepout{
      .id = "K_VIA_GEOM",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(9.9),
                                               .y = ccad::millimeters(8.8)},
                         .size = ccad::Size{.width = ccad::millimeters(0.4),
                                            .height = ccad::millimeters(0.4)}}});
  require(hasCode(ccad::runDrc(keepout_via_geometry), "VIA_IN_KEEPOUT"),
          "drc reports via geometry intersection with keepout");

  ccad::Project keepout_track = validBoardProject();
  keepout_track.board->keepouts.push_back(ccad::Keepout{
      .id = "K_TRACK",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(5)},
                         .size = ccad::Size{.width = ccad::millimeters(3),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(keepout_track), "TRACK_ENDPOINT_IN_KEEPOUT"),
          "drc reports track endpoint in keepout");

  ccad::Project keepout_track_crossing = validBoardProject();
  keepout_track_crossing.board->tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(12)};
  keepout_track_crossing.board->tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(12)};
  keepout_track_crossing.board->keepouts.push_back(ccad::Keepout{
      .id = "K_CROSS",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(6),
                                               .y = ccad::millimeters(10)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(4)}}});
  require(hasCode(ccad::runDrc(keepout_track_crossing), "TRACK_CROSSES_KEEPOUT"),
          "drc reports track crossing keepout with endpoints outside");

  ccad::Project keepout_track_width_overlap = validBoardProject();
  keepout_track_width_overlap.board->tracks.at(0).start =
      ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(11.65)};
  keepout_track_width_overlap.board->tracks.at(0).end =
      ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(11.65)};
  keepout_track_width_overlap.board->tracks.at(0).width = ccad::millimeters(0.6);
  keepout_track_width_overlap.board->keepouts.push_back(ccad::Keepout{
      .id = "K_WIDTH_OVERLAP",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(6),
                                               .y = ccad::millimeters(10)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(1.5)}}});
  require(hasCode(ccad::runDrc(keepout_track_width_overlap), "TRACK_CROSSES_KEEPOUT"),
          "drc reports keepout crossing when only track width overlaps keepout");

  ccad::Project duplicate_keepout = validBoardProject();
  duplicate_keepout.board->keepouts.push_back(ccad::Keepout{
      .id = "K_DUP",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(3),
                                               .y = ccad::millimeters(3)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  duplicate_keepout.board->keepouts.push_back(duplicate_keepout.board->keepouts.back());
  require(hasCode(ccad::runDrc(duplicate_keepout), "DUPLICATE_KEEPOUT_ID"),
          "drc reports duplicate keepout ids");

  ccad::Project invalid_keepout_size = validBoardProject();
  invalid_keepout_size.board->keepouts.push_back(ccad::Keepout{
      .id = "K_SIZE",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::millimeters(0),
                                            .height = ccad::millimeters(1)}}});
  require(hasCode(ccad::runDrc(invalid_keepout_size), "INVALID_KEEPOUT_SIZE"),
          "drc reports invalid keepout size");

  ccad::Project keepout_outside = validBoardProject();
  keepout_outside.board->keepouts.push_back(ccad::Keepout{
      .id = "K_OUT",
      .kind = "routing",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(41),
                                               .y = ccad::millimeters(27)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  require(hasCode(ccad::runDrc(keepout_outside), "KEEPOUT_OUTSIDE_BOARD"),
          "drc reports keepout area outside board");

  ccad::Project unknown_keepout_kind = validBoardProject();
  unknown_keepout_kind.board->keepouts.push_back(ccad::Keepout{
      .id = "K_KIND",
      .kind = "thermal",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(4)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  require(hasCode(ccad::runDrc(unknown_keepout_kind), "UNKNOWN_KEEPOUT_KIND"),
          "drc reports unknown keepout kind");

  ccad::Project empty_keepout_id = validBoardProject();
  empty_keepout_id.board->keepouts.push_back(ccad::Keepout{
      .id = "",
      .kind = "placement",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(4),
                                               .y = ccad::millimeters(4)},
                         .size = ccad::Size{.width = ccad::millimeters(2),
                                            .height = ccad::millimeters(2)}}});
  require(hasCode(ccad::runDrc(empty_keepout_id), "INVALID_KEEPOUT_ID"),
          "drc reports empty keepout id");

  ccad::Project duplicate_placement_region = validBoardProject();
  duplicate_placement_region.board->placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR1",
      .kind = "component",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::millimeters(5),
                                            .height = ccad::millimeters(5)}}});
  duplicate_placement_region.board->placement_regions.push_back(
      duplicate_placement_region.board->placement_regions.back());
  require(hasCode(ccad::runDrc(duplicate_placement_region),
                  "DUPLICATE_PLACEMENT_REGION_ID"),
          "drc reports duplicate placement region ids");

  ccad::Project invalid_placement_region_size = validBoardProject();
  invalid_placement_region_size.board->placement_regions.push_back(ccad::PlacementRegion{
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
  placement_region_outside.board->placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR_OUT",
      .kind = "component",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(40),
                                               .y = ccad::millimeters(26)},
                         .size = ccad::Size{.width = ccad::millimeters(4),
                                            .height = ccad::millimeters(3)}}});
  require(hasCode(ccad::runDrc(placement_region_outside), "PLACEMENT_REGION_OUTSIDE_BOARD"),
          "drc reports placement region area outside board");

  ccad::Project unknown_placement_region_kind = validBoardProject();
  unknown_placement_region_kind.board->placement_regions.push_back(ccad::PlacementRegion{
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
  empty_placement_region_id.board->placement_regions.push_back(ccad::PlacementRegion{
      .id = "",
      .kind = "component",
      .area = ccad::Rect{.origin = ccad::Point{.x = ccad::millimeters(2),
                                               .y = ccad::millimeters(2)},
                         .size = ccad::Size{.width = ccad::millimeters(5),
                                            .height = ccad::millimeters(5)}}});
  require(hasCode(ccad::runDrc(empty_placement_region_id), "INVALID_PLACEMENT_REGION_ID"),
          "drc reports empty placement region id");

  ccad::Project same_net_touching_track = validBoardProject();
  same_net_touching_track.board->tracks.push_back(ccad::TrackSegment{
      .id = "T_SAME",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
      .end = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(10)},
      .width = ccad::millimeters(0.25)});
  require(!hasCode(ccad::runDrc(same_net_touching_track), "COPPER_CLEARANCE"),
          "drc allows same-net copper to touch");

  ccad::Project pad_clearance = validBoardProject();
  pad_clearance.board->pads.push_back(ccad::Pad{
      .id = "P2",
      .component_id = "U2",
      .pin_name = "1",
      .net_id = "N2",
      .layer_id = "F.Cu",
      .position = ccad::Point{.x = ccad::millimeters(6.35), .y = ccad::millimeters(6)},
      .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}});
  require(hasDiagnosticForObject(ccad::runDrc(pad_clearance), "COPPER_CLEARANCE", "P2"),
          "drc reports different-net pads closer than configured clearance");
  require(hasDiagnosticMessageContaining(ccad::runDrc(pad_clearance), "COPPER_CLEARANCE",
                                         "configured clearance 200000 nm"),
          "drc clearance diagnostic includes configured clearance value");

  ccad::Project cross_layer_pad_clearance = pad_clearance;
  cross_layer_pad_clearance.board->pads.back().layer_id = "B.Cu";
  require(!hasDiagnosticForObject(ccad::runDrc(cross_layer_pad_clearance), "COPPER_CLEARANCE",
                                  "P2"),
          "drc allows different-net pads to overlap on different copper layers");

  ccad::Project relaxed_clearance = pad_clearance;
  relaxed_clearance.board->design_rules.copper_clearance = ccad::millimeters(0.04);
  require(!hasDiagnosticForObject(ccad::runDrc(relaxed_clearance), "COPPER_CLEARANCE", "P2"),
          "drc obeys configured copper clearance");

  ccad::Project crossing_tracks = validBoardProject();
  crossing_tracks.board->tracks.push_back(ccad::TrackSegment{
      .id = "T2",
      .net_id = "N2",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(4), .y = ccad::millimeters(9)},
      .end = ccad::Point{.x = ccad::millimeters(9), .y = ccad::millimeters(4)},
      .width = ccad::millimeters(0.25)});
  require(hasDiagnosticForObject(ccad::runDrc(crossing_tracks), "COPPER_CLEARANCE", "T2"),
          "drc reports crossing different-net tracks on same layer");

  ccad::Project cross_layer_crossing_tracks = crossing_tracks;
  cross_layer_crossing_tracks.board->tracks.back().layer_id = "B.Cu";
  require(!hasDiagnosticForObject(ccad::runDrc(cross_layer_crossing_tracks), "COPPER_CLEARANCE",
                                  "T2"),
          "drc allows different-net tracks to cross on different copper layers");

  ccad::Project via_cross_layer_clearance = cross_layer_pad_clearance;
  via_cross_layer_clearance.board->vias.push_back(ccad::Via{
      .id = "V2",
      .net_id = "N2",
      .position = ccad::Point{.x = ccad::millimeters(5.1), .y = ccad::millimeters(6)},
      .diameter = ccad::millimeters(0.8),
      .drill = ccad::millimeters(0.4)});
  require(hasDiagnosticForObject(ccad::runDrc(via_cross_layer_clearance), "COPPER_CLEARANCE",
                                 "V2"),
          "drc still checks via clearance against layer-bound copper");
}
