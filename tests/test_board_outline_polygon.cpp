#include "ccad_core/board_outline_polygon.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

ccad::Layer layer(const std::string& id, const std::string& kind) {
  return ccad::Layer{.id = id, .name = id, .kind = kind, .visible = true};
}

ccad::BoardGraphic edgeLine(const std::string& id,
                            double x1,
                            double y1,
                            double x2,
                            double y2) {
  return ccad::BoardGraphic{.id = id,
                            .kind = "line",
                            .layer_id = "Edge.Cuts",
                            .start = {.x = ccad::millimeters(x1),
                                      .y = ccad::millimeters(y1)},
                            .end = {.x = ccad::millimeters(x2), .y = ccad::millimeters(y2)},
                            .width = ccad::millimeters(0.1)};
}

ccad::Board baseBoard() {
  ccad::Board board;
  board.outline = ccad::Rect{
      .origin = {.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
      .size = {.width = ccad::millimeters(50), .height = ccad::millimeters(30)}};
  board.layers = {layer("F.Cu", "copper"), layer("B.Cu", "copper"),
                  layer("Edge.Cuts", "board_edge"), layer("F.SilkS", "silkscreen")};
  return board;
}

void test_edge_cuts_rectangle_builds_closed_outline_report() {
  ccad::Board board = baseBoard();
  board.graphics.push_back(edgeLine("E1", 5, 5, 25, 5));
  board.graphics.push_back(edgeLine("E2", 25, 5, 25, 20));
  board.graphics.push_back(edgeLine("E3", 25, 20, 5, 20));
  board.graphics.push_back(edgeLine("E4", 5, 20, 5, 5));
  board.graphics.push_back(ccad::BoardGraphic{
      .id = "S1",
      .kind = "line",
      .layer_id = "F.SilkS",
      .start = {.x = ccad::millimeters(1), .y = ccad::millimeters(1)},
      .end = {.x = ccad::millimeters(2), .y = ccad::millimeters(2)},
      .width = ccad::millimeters(0.1)});

  const ccad::BoardOutlinePolygonReport report = ccad::buildBoardOutlinePolygonReport(board);

  require(report.kicad_source == "convert_shape_list_to_polygon",
          "report records KiCad converter source");
  require(report.kicad_function == "ConvertOutlineToPolygon",
          "report records KiCad function name");
  require(report.parity_scope == "edge_cuts_segment_chain_first_slice",
          "report records first-slice parity scope");
  require(report.edge_cut_segment_count == 4, "only Edge.Cuts line graphics are collected");
  require(report.outline_count == 1, "closed rectangle produces one outline");
  require(report.hole_count == 0, "first slice reports no holes");
  require(report.closed, "closed rectangle reports closed outline");
  require(report.valid, "closed rectangle reports valid outline");
  require(report.used_inferred_outline == false, "closed rectangle does not infer outline");
  require(report.bounding_box.origin.x.nanometers == 5000000, "bounding box min x");
  require(report.bounding_box.origin.y.nanometers == 5000000, "bounding box min y");
  require(report.bounding_box.size.width.nanometers == 20000000, "bounding box width");
  require(report.bounding_box.size.height.nanometers == 15000000, "bounding box height");
  require(report.points.size() == 4, "rectangle report keeps four polygon points");
  require(!report.pending_kicad_features.empty(), "report documents pending KiCad features");
}

void test_open_edge_cuts_can_infer_board_outline() {
  ccad::Board board = baseBoard();
  board.graphics.push_back(edgeLine("E1", 5, 5, 25, 5));
  board.graphics.push_back(edgeLine("E2", 25, 5, 25, 20));

  ccad::BoardOutlinePolygonOptions options;
  options.infer_outline_if_necessary = true;

  const ccad::BoardOutlinePolygonReport report = ccad::buildBoardOutlinePolygonReport(board, options);

  require(!report.closed, "open chain remains reported as open");
  require(report.valid, "inferred fallback is usable");
  require(report.used_inferred_outline, "open chain uses inferred board outline");
  require(report.outline_count == 1, "inferred board rectangle is one outline");
  require(report.bounding_box.size.width.nanometers == board.outline.size.width.nanometers,
          "inferred outline uses board width");
  require(report.bounding_box.size.height.nanometers == board.outline.size.height.nanometers,
          "inferred outline uses board height");
}

}  // namespace

int main() {
  try {
    test_edge_cuts_rectangle_builds_closed_outline_report();
    test_open_edge_cuts_can_infer_board_outline();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
