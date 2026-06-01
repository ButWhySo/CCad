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
  scene.layers.push_back(
      ccad::CanvasLayer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true});
  scene.pads.push_back(ccad::CanvasPad{
      .id = "P1",
      .net_id = "N1",
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "rect",
      .x_units = 5.0,
      .y_units = 5.0,
      .width_units = 1.0,
      .height_units = 1.0,
  });
  scene.pads.push_back(ccad::CanvasPad{
      .id = "P2",
      .net_id = "N2",
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "rect",
      .x_units = 14.0,
      .y_units = 14.0,
      .width_units = 1.0,
      .height_units = 1.0,
  });
  scene.tracks.push_back(ccad::CanvasTrack{
      .id = "T1",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .source_route_request_id = "RR1",
      .start_x_units = 5.0,
      .start_y_units = 5.0,
      .end_x_units = 10.0,
      .end_y_units = 5.0,
      .width_units = 0.25,
  });
  return scene;
}

ccad::CanvasScene hiddenLayerScene() {
  ccad::CanvasScene scene = sceneWithPadAndTrack();
  scene.layers.at(0).visible = false;
  scene.vias.push_back(ccad::CanvasVia{
      .id = "V1",
      .net_id = "N1",
      .x_units = 8.0,
      .y_units = 8.0,
      .diameter_units = 1.0,
      .drill_units = 0.5,
  });
  return scene;
}

ccad::CanvasScene originShiftedScene() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.board_origin_x_units = 10.0;
  scene.board_origin_y_units = 20.0;
  scene.view_width_units = 20.0;
  scene.view_height_units = 20.0;
  scene.layers.push_back(
      ccad::CanvasLayer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true});
  scene.pads.push_back(ccad::CanvasPad{
      .id = "P_ORIGIN",
      .net_id = "N1",
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "rect",
      .x_units = 12.0,
      .y_units = 23.0,
      .width_units = 1.0,
      .height_units = 1.0,
  });
  return scene;
}

void requireNear(const double actual, const double expected, const char* message) {
  require(actual > expected - 0.01 && actual < expected + 0.01, message);
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
  require(selectCanvasObjectById(scene, "T1"), "route-provenanced track can be selected");
  require(canvasObjectRouteRequestId(*scene.selectedItems().first()) == "RR1",
          "track exposes route request metadata");
  require(selectCanvasObjectsByRouteRequestId(scene, "RR1") == 1,
          "selects routed tracks by route request id");
  require(canvasObjectId(*scene.selectedItems().first()) == "T1",
          "route request selection selects track");
  require(selectCanvasObjectsByRouteRequestId(scene, "NOPE") == 0,
          "missing route request reports zero");
  require(scene.selectedItems().isEmpty(), "missing route request clears selection");

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

  QGraphicsScene hidden_scene;
  renderBoardCanvas(hidden_scene, hiddenLayerScene());
  require(!selectCanvasObjectById(hidden_scene, "P1"), "hidden layer suppresses pad selection");
  require(!selectCanvasObjectById(hidden_scene, "T1"), "hidden layer suppresses track selection");
  require(selectCanvasObjectById(hidden_scene, "V1"), "vias stay visible when copper layer is hidden");
  require(selectCanvasObjectsByNetId(hidden_scene, "N1") == 1,
          "hidden layer net selection excludes hidden copper");

  QGraphicsScene origin_scene;
  renderBoardCanvas(origin_scene, originShiftedScene());
  require(selectCanvasObjectById(origin_scene, "P_ORIGIN"), "selects origin-shifted pad");
  const QRectF bounds = origin_scene.selectedItems().first()->sceneBoundingRect();
  requireNear(bounds.center().x(), 38.0, "origin-shifted pad x is board-relative");
  requireNear(bounds.center().y(), 48.0, "origin-shifted pad y is board-relative");
}
