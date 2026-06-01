#include "ccad_gui/board_canvas_renderer.hpp"
#include "ccad_core/erc.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QGraphicsScene>

namespace {

ccad::CanvasScene sceneWithPad() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.view_width_units = 20.0;
  scene.view_height_units = 20.0;
  scene.pads.push_back(ccad::CanvasPad{
      .id = "P1",
      .net_id = "N1",
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "rect",
      .x_units = 10.0,
      .y_units = 5.0,
      .width_units = 1.0,
      .height_units = 1.0,
  });
  return scene;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QGraphicsScene scene;
  renderBoardCanvas(scene, sceneWithPad());
  addDiagnosticMarkers(scene, {ccad::Diagnostic{.severity = "error",
                                                .code = "PAD_IN_KEEPOUT",
                                                .message = "Pad in keepout",
                                                .object_id = "P1"},
                               ccad::Diagnostic{.severity = "warning",
                                                .code = "UNKNOWN",
                                                .message = "No canvas object",
                                                .object_id = "NOPE"}});

  int marker_count = 0;
  for (QGraphicsItem* item : scene.items()) {
    if (canvasDiagnosticMarkerObjectId(*item) == "P1") {
      ++marker_count;
      require(canvasDiagnosticMarkerSeverity(*item) == "error", "marker severity stored");
    }
  }
  require(marker_count == 1, "one marker created for matching object id");
}
