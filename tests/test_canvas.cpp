#include "ccad_core/canvas.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

namespace {

ccad::Project boardProject() {
  ccad::Project project;
  project.id = "proj-canvas";
  project.name = "canvas";
  project.board = ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .layers = {},
  };
  return project;
}

}  // namespace

int main() {
  ccad::Project empty;
  empty.id = "empty";
  const ccad::CanvasScene empty_scene = ccad::buildCanvasScene(empty);
  require(!empty_scene.has_board, "empty scene reports no board");
  require(empty_scene.board_width_nm == 0, "empty scene width zero");
  require(empty_scene.board_height_nm == 0, "empty scene height zero");

  const ccad::CanvasScene scene = ccad::buildCanvasScene(boardProject());
  require(scene.has_board, "board scene reports board");
  require(scene.board_width_nm == 42000000, "board scene width set");
  require(scene.board_height_nm == 28000000, "board scene height set");
  require(scene.view_width_units == 42.0, "view width is mm");
  require(scene.view_height_units == 28.0, "view height is mm");
}

