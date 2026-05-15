#include "ccad_gui/board_canvas_renderer.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QColor>
#include <QGraphicsScene>

namespace {

ccad::CanvasScene selectionScene() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.view_width_units = 20.0;
  scene.view_height_units = 20.0;
  scene.tracks.push_back(ccad::CanvasTrack{
      .id = "T1",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .start_x_units = 2.0,
      .start_y_units = 2.0,
      .end_x_units = 16.0,
      .end_y_units = 14.0,
      .width_units = 0.25,
  });
  scene.pads.push_back(ccad::CanvasPad{
      .id = "P1",
      .net_id = "N1",
      .layer_id = "F.Cu",
      .x_units = 5.0,
      .y_units = 6.0,
      .width_units = 1.5,
      .height_units = 1.0,
  });
  return scene;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QGraphicsScene scene;
  renderBoardCanvas(scene, selectionScene());

  bool saw_track = false;
  bool saw_pad = false;
  for (QGraphicsItem* item : scene.items()) {
    const QString type = canvasObjectType(*item);
    if (type == "track") {
      saw_track = true;
      require(canvasUsesShapeSelectionHighlight(*item), "track uses shape selection highlight");
      require(canvasSelectionHighlightColor(*item) == QColor("#ef4444").lighter(160),
              "track highlight derives from track color");
    }
    if (type == "pad") {
      saw_pad = true;
      require(canvasUsesShapeSelectionHighlight(*item), "pad uses shape selection highlight");
      require(canvasSelectionHighlightColor(*item) == QColor("#be185d").lighter(160),
              "pad highlight derives from pad color");
    }
  }

  require(saw_track, "test scene has selectable track");
  require(saw_pad, "test scene has selectable pad");
}
