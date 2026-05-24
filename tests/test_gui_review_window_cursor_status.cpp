#include "ccad_gui/review_window.hpp"
#include "test_support.hpp"

#include <QPointF>
#include <QString>

#include <optional>

namespace {

ccad::Board shiftedBoard() {
  return ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(3)},
          .size = ccad::Size{.width = ccad::millimeters(44), .height = ccad::millimeters(30)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true}},
      .placement_regions = {},
      .keepouts = {},
      .pads = {},
      .vias = {},
      .tracks = {},
      .route_requests = {},
  };
}

}  // namespace

int main() {
  const std::optional<ccad::Board> board = shiftedBoard();
  require(formatCursorStatus(board, QPointF(0.0, 0.0)) == "Canvas X 0.20 mm  Y 1.20 mm",
          "off-board status accounts for shifted board origin");
  require(formatCursorStatus(board, QPointF(18.0, 18.0)) == "Board X 2.00 mm  Y 3.00 mm",
          "board origin point is inside shifted board");
  require(formatCursorStatus(board, QPointF(458.0, 318.0)) == "Board X 46.00 mm  Y 33.00 mm",
          "board max point is inside shifted board");
  require(formatCursorStatus(board, QPointF(468.0, 328.0)) == "Canvas X 47.00 mm  Y 34.00 mm",
          "point beyond shifted board is canvas status");
  require(formatCursorStatus(std::nullopt, QPointF(18.0, 18.0)) == "X --  Y --",
          "missing board keeps empty coordinate status");
}
