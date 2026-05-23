#include "ccad_gui/board_canvas_renderer.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QGraphicsScene>

namespace {

ccad::CanvasScene sceneWithPadAndTrack() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.view_width_units = 20.0;
  scene.view_height_units = 20.0;
  scene.pads.push_back(ccad::CanvasPad{
      .id = "P1",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .x_units = 5.0,
      .y_units = 5.0,
      .width_units = 1.0,
      .height_units = 1.0,
  });
  scene.pads.push_back(ccad::CanvasPad{
      .id = "P2",
      .net_id = "N2",
      .layer_id = "F.Cu",
      .x_units = 14.0,
      .y_units = 14.0,
      .width_units = 1.0,
      .height_units = 1.0,
  });
  scene.tracks.push_back(ccad::CanvasTrack{
      .id = "T1",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .start_x_units = 5.0,
      .start_y_units = 5.0,
      .end_x_units = 10.0,
      .end_y_units = 5.0,
      .width_units = 0.25,
  });
  return scene;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QGraphicsScene scene;
  renderBoardCanvas(scene, sceneWithPadAndTrack());

  require(selectCanvasObjectById(scene, "P1"), "selects existing pad by object id");
  require(!scene.selectedItems().isEmpty(), "selection list is populated");
  require(canvasObjectId(*scene.selectedItems().first()) == "P1", "selected item has requested id");

  require(selectCanvasObjectById(scene, "T1"), "selects existing track by object id");
  require(scene.selectedItems().size() == 1, "select by id clears previous selection");
  require(canvasObjectId(*scene.selectedItems().first()) == "T1", "new selected item has requested id");

  require(!selectCanvasObjectById(scene, "NOPE"), "missing object id reports false");
  require(scene.selectedItems().isEmpty(), "missing object clears selection");

  int n1_tagged_items = 0;
  for (QGraphicsItem* item : scene.items()) {
    if (canvasObjectNetId(*item) == "N1") {
      ++n1_tagged_items;
    }
  }
  require(n1_tagged_items == 2, "pad and track expose matching net metadata");

  require(selectCanvasObjectsByNetId(scene, "N1") == 2, "selects all canvas objects on net");
  require(scene.selectedItems().size() == 2, "net selection selects both matching objects");
  for (QGraphicsItem* item : scene.selectedItems()) {
    require(canvasObjectNetId(*item) == "N1", "selected item belongs to requested net");
  }

  require(selectCanvasObjectsByNetId(scene, "N2") == 1, "selects one-object net");
  require(scene.selectedItems().size() == 1, "net selection clears previous selection");
  require(canvasObjectId(*scene.selectedItems().first()) == "P2", "single selected item is N2 pad");

  require(selectCanvasObjectsByNetId(scene, "NOPE") == 0, "missing net reports zero");
  require(scene.selectedItems().isEmpty(), "missing net clears selection");
}
