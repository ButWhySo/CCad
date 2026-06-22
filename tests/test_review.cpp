#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "test_support.hpp"

#include <string>

namespace {

ccad::Project validProject() {
  ccad::Project project;
  project.id = "proj-review";
  project.name = "review";
  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].components.push_back(ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "VDD", .kind = "power"}},
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
          .origin = ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(3)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .design_rules = ccad::DesignRules{
          .copper_clearance = ccad::millimeters(0.25),
          .min_track_width = ccad::millimeters(0.18),
          .min_via_annular_ring = ccad::millimeters(0.11),
      },
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                 ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = false},
                 ccad::Layer{.id = "F.Mask", .name = "Front mask", .kind = "mask", .visible = true},
                 ccad::Layer{.id = "Edge.Cuts", .name = "Board outline", .kind = "board_edge", .visible = true},
                 ccad::Layer{.id = "User.9", .name = "User 9", .kind = "user", .visible = false}},
      .placement_regions = {},
      .keepouts = {},
      .pads = {},
      .vias = {},
      .tracks = {},
      .graphics = {},
      .texts = {},
      .zones = {},
      .route_requests = {},
  });
  return project;
}

ccad::Project boardOnlyProject() {
  ccad::Project project = validProject();
  project.schematics.clear();
  project.id = "proj-board-only";
  project.name = "board only";
  project.boards[0].pads.push_back(ccad::Pad{
      .id = "P_BOARD",
      .component_id = "J1",
      .pin_name = "1",
      .net_id = "N_BOARD",
      .position = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(10)},
      .padstack = ccad::Padstack{
         .layer_set = {"F.Cu"},
         .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.0)}}}}}
      }});
  return project;
}

}  // namespace

int main() {
  const ccad::ProjectReview clean = ccad::buildReview(validProject());
  require(clean.project_id == "proj-review", "review keeps project id");
  require(clean.project_name == "review", "review keeps project name");
  require(clean.component_count == 1, "component count set");
  require(clean.net_count == 1, "net count set");
  require(clean.constraint_count == 1, "constraint count set");
  require(clean.has_board, "review reports board present");
  require(clean.board_origin_x_nm == 2000000, "review reports board origin x");
  require(clean.board_origin_y_nm == 3000000, "review reports board origin y");
  require(clean.board_width_nm == 42000000, "review reports board width");
  require(clean.board_height_nm == 28000000, "review reports board height");
  require(clean.copper_clearance_nm == 250000, "review reports copper clearance rule");
  require(clean.min_track_width_nm == 180000, "review reports minimum track width rule");
  require(clean.min_via_annular_ring_nm == 110000,
          "review reports minimum via annular ring rule");
  require(clean.layer_count == 5, "review reports layer count");
  require(clean.copper_layer_count == 2, "review reports copper layer count");
  require(clean.non_copper_layer_count == 3, "review reports non-copper layer count");
  require(clean.visible_layer_count == 3, "review reports visible layer count");
  require(clean.hidden_layer_count == 2, "review reports hidden layer count");
  require(clean.pad_count == 0, "review reports pad count");
  require(clean.via_count == 0, "review reports via count");
  require(clean.track_count == 0, "review reports track count");
  require(clean.placement_region_count == 0, "review reports placement region count");
  require(clean.keepout_count == 0, "review reports keepout count");
  require(clean.route_request_count == 0, "review reports route request count");
  require(clean.routed_segment_count == 0, "review reports routed segment count");
  require(clean.open_route_count == 0, "review reports open route count");
  require(clean.partial_route_count == 0, "review reports partial route count");
  require(clean.completed_route_count == 0, "review reports completed route count");
  require(clean.error_count == 0, "clean review has no errors");
  require(clean.warning_count == 0, "clean review has no warnings");
  require(clean.status == "Clean: 1 component, 1 net, 1 constraint", "clean status text");

  const ccad::ProjectReview board_only = ccad::buildReview(boardOnlyProject());
  require(board_only.project_id == "proj-board-only", "board-only review keeps project id");
  require(board_only.has_board, "board-only review reports board present");
  require(board_only.component_count == 0, "board-only review has no schematic components");
  require(board_only.net_count == 0, "board-only review has no schematic nets");
  require(board_only.pad_count == 1, "board-only review still counts pads");
  require(board_only.error_count == 0, "board-only review has no errors");
  require(board_only.warning_count == 0, "board-only review has no schematic-empty warning");
  require(board_only.status == "Clean: 0 components, 0 nets, 0 constraints",
          "board-only review status is clean");

  ccad::Project drc_invalid = validProject();
  drc_invalid.boards[0].keepouts.push_back(ccad::Keepout{
      .id = "K1",
      .kind = "placement",
      .area = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(4), .y = ccad::millimeters(5)},
          .size = ccad::Size{.width = ccad::millimeters(3), .height = ccad::millimeters(3)}}});
  drc_invalid.boards[0].pads.push_back(ccad::Pad{
      .id = "P1",
      .component_id = "U1",
      .pin_name = "VDD",
      .net_id = "N_3V3",
      .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
      .padstack = ccad::Padstack{
         .layer_set = {"F.Cu"},
         .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.0)}}}}}
      }});
  const ccad::ProjectReview drc_review = ccad::buildReview(drc_invalid);
  require(drc_review.pad_count == 1, "review counts pads");
  require(drc_review.keepout_count == 1, "review counts keepouts");
  require(drc_review.error_count == 1, "review includes drc error");
  require(drc_review.diagnostics.size() == 1, "review carries drc diagnostic");
  require(drc_review.diagnostics.at(0).code == "PAD_IN_KEEPOUT", "review includes drc code");
  require(drc_review.diagnostics.at(0).object_id == "P1", "review includes drc object id");

  drc_invalid.boards[0].placement_regions.push_back(ccad::PlacementRegion{
      .id = "PR1",
      .kind = "component",
      .area = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(2)},
          .size = ccad::Size{.width = ccad::millimeters(5), .height = ccad::millimeters(5)}}});
  const ccad::ProjectReview placement_review = ccad::buildReview(drc_invalid);
  require(placement_review.placement_region_count == 1, "review counts placement regions");

  ccad::Project invalid = validProject();
  invalid.schematics[0].nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U404", .pin_name = "VDD"});
  const ccad::ProjectReview invalid_review = ccad::buildReview(invalid);
  require(invalid_review.error_count == 1, "invalid review has one error");
  require(invalid_review.warning_count == 0, "invalid review has no warnings");
  require(invalid_review.diagnostics.size() == 1, "invalid review carries diagnostic");
  require(invalid_review.status == "Errors: 1, warnings: 0", "invalid status text");

  ccad::Project empty;
  empty.schematics.push_back(ccad::Schematic{});
  empty.schematics.push_back(ccad::Schematic{});
  empty.id = "proj-empty";
  empty.name = "empty";
  const ccad::ProjectReview warning_review = ccad::buildReview(empty);
  require(warning_review.error_count == 0, "warning review has no errors");
  require(warning_review.warning_count == 1, "warning review has one warning");
  require(warning_review.status == "Warnings: 1", "warning status text");

  ccad::Project route_review_project = validProject();
  route_review_project.boards[0].pads.push_back(ccad::Pad{
      .id = "P1",
      .component_id = "U1",
      .pin_name = "VDD",
      .net_id = "N_3V3",
      .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
      .padstack = ccad::Padstack{
         .layer_set = {"F.Cu"},
         .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.0)}}}}}
      }});
  route_review_project.boards[0].vias.push_back(ccad::Via{
      .id = "V1",
      .net_id = "N_3V3",
      .position = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
      .diameter = ccad::millimeters(0.8),
      .drill = ccad::millimeters(0.4)});
  route_review_project.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "RT1",
      .net_id = "N_3V3",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
      .end = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
      .width = ccad::millimeters(0.25),
      .source_route_request_id = "RR1"});
  route_review_project.boards[0].tracks.push_back(ccad::TrackSegment{
      .id = "RT2",
      .net_id = "N_3V3",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
      .end = ccad::Point{.x = ccad::millimeters(9), .y = ccad::millimeters(10)},
      .width = ccad::millimeters(0.25),
      .source_route_request_id = "DONE1"});
  route_review_project.boards[0].route_requests.push_back(ccad::RouteRequest{
      .id = "RR1",
      .net_id = "N_3V3",
      .from_object_id = "P1",
      .to_object_id = "V1",
      .preferred_layer_id = "F.Cu",
      .policy = "shortest_safe",
      .width = ccad::millimeters(0.25)});
  route_review_project.boards[0].route_requests.push_back(ccad::RouteRequest{
      .id = "RR2",
      .net_id = "N_3V3",
      .from_object_id = "P1",
      .to_object_id = "V1",
      .preferred_layer_id = "F.Cu",
      .policy = "shortest_safe",
      .width = ccad::millimeters(0.25)});
  const ccad::ProjectReview route_review = ccad::buildReview(route_review_project);
  require(route_review.route_request_count == 2, "review counts open route requests");
  require(route_review.routed_segment_count == 2, "review counts provenanced routed segments");
  require(route_review.open_route_count == 1, "review counts open routes");
  require(route_review.partial_route_count == 1, "review counts partial routes");
  require(route_review.completed_route_count == 1, "review counts completed routes");
}

