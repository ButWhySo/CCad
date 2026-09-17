#include "ccad_core/canvas.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

namespace {

ccad::Project boardProject() {
  ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.schematics.push_back(ccad::Schematic{});
  project.id = "proj-canvas";
  project.name = "canvas";
  project.boards.push_back(ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(3)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu",
                              .name = "Front copper",
                              .kind = "signal",
                              .visible = true},
                 ccad::Layer{.id = "B.Cu",
                              .name = "Back copper",
                              .kind = "signal",
                              .visible = false},
                 ccad::Layer{.id = "F.SilkS",
                              .name = "Front silkscreen",
                              .kind = "silkscreen",
                              .visible = true},
                 ccad::Layer{.id = "Dwgs.User",
                              .name = "User drawings",
                              .kind = "user",
                              .visible = true}},
      .placement_regions = {ccad::PlacementRegion{
          .id = "PR1",
          .kind = "component",
          .area = ccad::Rect{
              .origin = ccad::Point{.x = ccad::millimeters(3), .y = ccad::millimeters(4)},
              .size = ccad::Size{.width = ccad::millimeters(9),
                                  .height = ccad::millimeters(5)}}}},
      .keepouts = {ccad::Keepout{.id = "K1",
                                 .kind = "placement",
                                 .area = ccad::Rect{
                                     .origin = ccad::Point{.x = ccad::millimeters(20),
                                                           .y = ccad::millimeters(10)},
                                     .size = ccad::Size{.width = ccad::millimeters(4),
                                                        .height = ccad::millimeters(3)}}}},
      .pads = {ccad::Pad{.id = "P1",
                         .component_id = "U1",
                         .pin_name = "1",
                         .net_id = "N1",
                         .type = "smd",
                         .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                         .rotation_degrees = 90.0,
                         .padstack = ccad::Padstack{
                             .layer_set = {"F.Cu"},
                             .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::RoundRect, .size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.0)}, .offset = ccad::Point{.x = ccad::millimeters(0), .y = ccad::millimeters(0)}, .roundrect_rratio = 0.25, .chamfer_ratio = 0.0, .chamfer_positions = 0, .trapezoid_delta_size = ccad::Size{.width = ccad::millimeters(0), .height = ccad::millimeters(0)}}}}}
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
                                    .width = ccad::millimeters(0.25),
                                    .source_route_request_id = "RR1"}},
      .graphics = {ccad::BoardGraphic{.id = "G1",
                                       .kind = "line",
                                       .layer_id = "Dwgs.User",
                                       .start = ccad::Point{.x = ccad::millimeters(4),
                                                           .y = ccad::millimeters(5)},
                                       .end = ccad::Point{.x = ccad::millimeters(16),
                                                         .y = ccad::millimeters(5)},
                                       .width = ccad::millimeters(0.15)}},
      .texts = {ccad::BoardText{.id = "BT1",
                                .layer_id = "F.SilkS",
                                .text = "RECTIFIER",
                                .position = ccad::Point{.x = ccad::millimeters(12),
                                                        .y = ccad::millimeters(20)},
                                .size = ccad::Size{.width = ccad::millimeters(1.5),
                                                   .height = ccad::millimeters(1.5)}}},
      .dimensions = {ccad::BoardDimension{
          .id = "D1",
          .layer_id = "F.Fab",
          .kind = "linear",
          .text = "10 mm",
          .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(5)},
          .end = ccad::Point{.x = ccad::millimeters(15), .y = ccad::millimeters(5)},
          .text_position = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(3)},
      }},
      .groups = {ccad::BoardGroup{
          .id = "GRP1",
          .name = "MyGroup",
          .members = {"P1", "V1"}
      }},
      .barcodes = {ccad::BoardBarcode{.id = "BC1",
                                      .layer_id = "F.SilkS",
                                      .text = "Hello World",
                                      .kind = ccad::BarcodeType::QRCode,
                                      .position = {ccad::millimeters(10), ccad::millimeters(20)},
                                      .rotation_degrees = 90.0,
                                      .size = {ccad::millimeters(5), ccad::millimeters(5)}}},
      .targets = {ccad::BoardTarget{.id = "T1",
                                    .layer_id = "F.Cu",
                                    .shape = ccad::TargetShape::X,
                                    .position_x = ccad::millimeters(15),
                                    .position_y = ccad::millimeters(25),
                                    .size = ccad::millimeters(3),
                                    .line_width = ccad::millimeters(0.5)}},
      .zones = {ccad::BoardZone{
          .id = "Z1",
          .name = "GND",
          .net_id = "GND",
          .layer_ids = {"F.Cu"},
          .outline = {ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(2)},
                      ccad::Point{.x = ccad::millimeters(18), .y = ccad::millimeters(2)},
                      ccad::Point{.x = ccad::millimeters(18), .y = ccad::millimeters(12)},
                      ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(12)}},
          .filled_thermal_spokes = {{ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(5)},
                                     ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(5)},
                                     ccad::millimeters(0.3)}},
          .clearance = ccad::millimeters(0.25),
          .min_thickness = ccad::millimeters(0.1),
          .fill_enabled = true,
          .pad_connection = "solid",
      }},
      .route_requests = {ccad::RouteRequest{.id = "RR1",
                                            .net_id = "N1",
                                            .from_object_id = "P1",
                                            .to_object_id = "V1",
                                            .preferred_layer_id = "F.Cu",
                                            .policy = "shortest_safe",
                                            .width = ccad::millimeters(0.25)}},
  });
  return project;
}

}  // namespace

int main() {
  ccad::Project empty;
  empty.schematics.push_back(ccad::Schematic{});
  empty.schematics.push_back(ccad::Schematic{});
  empty.id = "empty";
  const ccad::CanvasScene empty_scene = ccad::buildCanvasScene(ccad::Board{});

  require(empty_scene.board_width_nm == 0, "empty scene width zero");
  require(empty_scene.board_height_nm == 0, "empty scene height zero");

  const ccad::Project bp = boardProject();
  const ccad::CanvasScene scene = ccad::buildCanvasScene(bp.boards[0]);
  require(scene.has_board, "board scene reports board");
  require(scene.board_width_nm == 42000000, "board scene width set");
  require(scene.board_height_nm == 28000000, "board scene height set");
  require(scene.board_origin_x_units == 2.0, "board origin x is mm");
  require(scene.board_origin_y_units == 3.0, "board origin y is mm");
  require(scene.view_width_units == 42.0, "view width is mm");
  require(scene.view_height_units == 28.0, "view height is mm");
  require(scene.board_bounding_boxes.size() == 1,
          "canvas exposes one KiCad board bounding-box view item");
  require(scene.board_bounding_boxes.at(0).id == "board.bounding_box",
          "canvas board bounding-box id");
  require(scene.board_bounding_boxes.at(0).class_name == "BOARD_BOUNDING_BOX",
          "canvas board bounding-box class");
  require(scene.board_bounding_boxes.at(0).layer_id == "LAYER_BOARD_BOUNDING_BOX",
          "canvas board bounding-box view layer");
  require(scene.board_bounding_boxes.at(0).skip_struct,
          "canvas board bounding-box matches KiCad skip struct behavior");
  require(scene.board_bounding_boxes.at(0).x_units == 2.0,
          "canvas board bounding-box x is mm");
  require(scene.board_bounding_boxes.at(0).y_units == 3.0,
          "canvas board bounding-box y is mm");
  require(scene.board_bounding_boxes.at(0).width_units == 42.0,
          "canvas board bounding-box width is mm");
  require(scene.board_bounding_boxes.at(0).height_units == 28.0,
          "canvas board bounding-box height is mm");
  require(scene.layers.size() == 4, "canvas has board layers");
  require(scene.layers.at(0).id == "F.Cu", "canvas layer id");
  require(scene.layers.at(0).name == "Front copper", "canvas layer name");
  require(scene.layers.at(0).kind == "signal", "canvas layer kind");
  require(scene.layers.at(0).visible, "canvas front layer visibility");
  require(!scene.layers.at(1).visible, "canvas back layer visibility");
  require(scene.placement_regions.size() == 1, "canvas has placement region");
  require(scene.placement_regions.at(0).id == "PR1", "canvas placement region id");
  require(scene.placement_regions.at(0).x_units == 3.0, "canvas placement region x is mm");
  require(scene.placement_regions.at(0).width_units == 9.0,
          "canvas placement region width is mm");
  require(scene.keepouts.size() == 1, "canvas has keepout");
  require(scene.keepouts.at(0).id == "K1", "canvas keepout id");
  require(scene.keepouts.at(0).x_units == 20.0, "canvas keepout x is mm");
  require(scene.keepouts.at(0).width_units == 4.0, "canvas keepout width is mm");
  require(scene.pads.size() == 1, "canvas has pad");
  require(scene.pads.at(0).net_id == "N1", "canvas pad net id");
  require(scene.pads.at(0).layers.empty() == false && scene.pads.at(0).layers.front() == "F.Cu", "canvas pad layer id");
  require(scene.pads.at(0).shape == "roundrect", "canvas pad shape");
  require(scene.pads.at(0).roundrect_rratio.has_value(), "canvas pad roundrect ratio");
  require(scene.pads.at(0).x_units == 5.0, "canvas pad x is mm");
  require(scene.pads.at(0).rotation_degrees == 90.0, "canvas pad rotation is degrees");
  require(scene.vias.size() == 1, "canvas has via");
  require(scene.vias.at(0).net_id == "N1", "canvas via net id");
  require(scene.vias.at(0).diameter_units == 0.8, "canvas via diameter is mm");
  require(scene.tracks.size() == 1, "canvas has track");
  require(scene.tracks.at(0).net_id == "N1", "canvas track net id");
  require(scene.tracks.at(0).layer_id == "F.Cu", "canvas track layer id");
  require(scene.tracks.at(0).source_route_request_id == "RR1",
          "canvas track exposes route provenance");
  require(scene.tracks.at(0).width_units == 0.25, "canvas track width is mm");
  require(scene.lines.size() == 1, "canvas has board graphic line");
  require(scene.lines.at(0).id == "G1", "canvas board graphic id");
  require(scene.lines.at(0).layer_id == "Dwgs.User", "canvas board graphic layer id");
  require(scene.lines.at(0).start_x_units == 4.0, "canvas board graphic start x is mm");
  require(scene.lines.at(0).end_x_units == 16.0, "canvas board graphic end x is mm");
  require(scene.lines.at(0).width_units == 0.15, "canvas board graphic width is mm");
  require(scene.texts.size() == 1, "canvas has board text");
  require(scene.texts.at(0).id == "BT1", "canvas board text id");
  require(scene.texts.at(0).layer_id == "F.SilkS", "canvas board text layer id");
  require(scene.texts.at(0).text == "RECTIFIER", "canvas board text value");
  require(scene.texts.at(0).x_units == 12.0, "canvas board text x is mm");
  require(scene.texts.at(0).size_x_units == 1.5, "canvas board text width is mm");
  require(scene.dimensions.size() == 1, "canvas has dimension");
  require(scene.dimensions.at(0).id == "D1", "canvas dimension id");
  require(scene.dimensions.at(0).text == "10 mm", "canvas dimension text");
  require(scene.dimensions.at(0).lines.size() == 3, "canvas dimension has measurement and extension lines");
  require(scene.dimensions.at(0).lines.at(0).sx == 5.0, "canvas dimension start x");
  require(scene.dimensions.at(0).lines.at(0).sy == 3.0, "canvas dimension text-line y");
  require(scene.dimensions.at(0).lines.at(1).sx == 5.0, "canvas dimension first extension starts at board point");
  require(scene.dimensions.at(0).lines.at(1).sy == 5.0, "canvas dimension first extension y");
  require(scene.zones.size() == 1, "canvas has board zone");
  require(scene.zones.at(0).id == "Z1", "canvas board zone id");
  require(scene.zones.at(0).layer_ids.size() == 1, "canvas board zone layer set");
  require(scene.zones.at(0).pts_x_units.size() == 4, "canvas board zone outline points");
  require(scene.zones.at(0).fill_enabled, "canvas board zone fill state");
  require(scene.zones.at(0).thermal_spokes.size() == 1,
          "canvas exposes persisted thermal spoke");
  require(scene.zones.at(0).thermal_spokes.at(0).start_x_units == 5.0 &&
              scene.zones.at(0).thermal_spokes.at(0).width_units == 0.3,
          "canvas converts thermal spoke geometry to mm");
  require(scene.route_requests.size() == 1, "canvas has route requests");
  require(scene.route_requests.at(0).id == "RR1", "canvas route request id");
  require(scene.route_requests.at(0).from_object_id == "P1", "canvas route from object id");
  require(scene.route_requests.at(0).to_object_id == "V1", "canvas route to object id");
  require(scene.route_requests.at(0).preferred_layer_id == "F.Cu",
          "canvas route preferred layer id");
  require(scene.route_requests.at(0).policy == "shortest_safe", "canvas route policy");
  require(scene.route_requests.at(0).width_nm == 250000, "canvas route width is nm");
  require(scene.route_requests.at(0).routed_segment_count == 1,
          "canvas route counts partial routed segments");

  require(scene.barcodes.size() == 1, "canvas contains barcodes");
  require(scene.barcodes.at(0).id == "BC1", "canvas barcode id");
  require(scene.barcodes.at(0).layer_id == "F.SilkS", "canvas barcode layer_id");
  require(scene.barcodes.at(0).text == "Hello World", "canvas barcode text");
  require(scene.barcodes.at(0).kind == "QRCode", "canvas barcode kind");
  require(std::abs(scene.barcodes.at(0).x_units - 10.0) < 1e-6, "canvas barcode x");
  require(std::abs(scene.barcodes.at(0).y_units - 20.0) < 1e-6, "canvas barcode y");
  require(std::abs(scene.barcodes.at(0).width_units - 5.0) < 1e-6, "canvas barcode width");
  require(std::abs(scene.barcodes.at(0).height_units - 5.0) < 1e-6, "canvas barcode height");
  require(scene.barcodes.at(0).rotation_degrees == 90.0, "canvas barcode rotation");

  require(scene.targets.size() == 1, "canvas contains targets");
  require(scene.targets.at(0).id == "T1", "canvas target id");
  require(scene.targets.at(0).layer_id == "F.Cu", "canvas target layer_id");
  require(scene.targets.at(0).shape == "X", "canvas target shape");
  require(std::abs(scene.targets.at(0).x_units - 15.0) < 1e-6, "canvas target x");
  require(std::abs(scene.targets.at(0).y_units - 25.0) < 1e-6, "canvas target y");
  require(std::abs(scene.targets.at(0).size_units - 3.0) < 1e-6, "canvas target size");
  require(std::abs(scene.targets.at(0).line_width_units - 0.5) < 1e-6, "canvas target line width");

  ccad::Schematic zero_position_schematic;
  zero_position_schematic.symbols = {
      ccad::SchSymbol{.id = "U1", .lib_id = "Device:R"},
      ccad::SchSymbol{.id = "U2", .lib_id = "Device:C"},
  };
  const ccad::CanvasScene laid_out_scene =
      ccad::buildSchematicScene(zero_position_schematic);
  require(laid_out_scene.symbols.size() == 2,
          "schematic scene retains zero-position symbols");
  require(laid_out_scene.symbols.at(0).x_units != laid_out_scene.symbols.at(1).x_units ||
              laid_out_scene.symbols.at(0).y_units != laid_out_scene.symbols.at(1).y_units,
          "schematic scene separates unplaced symbols for display");

  ccad::Schematic positioned_schematic;
  positioned_schematic.symbols = {
      ccad::SchSymbol{.id = "U3", .lib_id = "Device:R", .position = ccad::Point{
          ccad::millimeters(42.0), ccad::millimeters(17.0)}}};
  const ccad::CanvasScene positioned_scene =
      ccad::buildSchematicScene(positioned_schematic);
  require(positioned_scene.symbols.at(0).x_units == 42.0 &&
              positioned_scene.symbols.at(0).y_units == 17.0,
          "schematic scene preserves explicit symbol position");
}

