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
                         .layer_id = "F.Cu",
                         .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                         .rotation_degrees = 90.0,
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
  require(scene.keepouts.size() == 1, "canvas has keepout");
  require(scene.keepouts.at(0).id == "K1", "canvas keepout id");
  require(scene.keepouts.at(0).x_units == 20.0, "canvas keepout x is mm");
  require(scene.keepouts.at(0).width_units == 4.0, "canvas keepout width is mm");
  require(scene.pads.size() == 1, "canvas has pad");
  require(scene.pads.at(0).x_units == 5.0, "canvas pad x is mm");
  require(scene.pads.at(0).rotation_degrees == 90.0, "canvas pad rotation is degrees");
  require(scene.vias.size() == 1, "canvas has via");
  require(scene.vias.at(0).diameter_units == 0.8, "canvas via diameter is mm");
  require(scene.tracks.size() == 1, "canvas has track");
  require(scene.tracks.at(0).width_units == 0.25, "canvas track width is mm");
}

