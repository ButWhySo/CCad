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
      .source_route_request_id = {},
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
  const QRectF scene_rect = scene.sceneRect();
  require(scene_rect.width() > (20.0 * 10.0), "scene rect is wider than board width for pan room");
  require(scene_rect.height() > (20.0 * 10.0), "scene rect is taller than board height for pan room");

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

  CanvasRenderTheme theme;
  theme.track_color = QColor("#22c55e");
  theme.pad_fill_color = QColor("#0ea5e9");

  QGraphicsScene themed_scene;
  renderBoardCanvas(themed_scene, selectionScene(), theme);

  bool saw_themed_track = false;
  bool saw_themed_pad = false;
  for (QGraphicsItem* item : themed_scene.items()) {
    const QString type = canvasObjectType(*item);
    if (type == "track") {
      saw_themed_track = true;
      require(canvasSelectionHighlightColor(*item) == theme.track_color.lighter(160),
              "themed track highlight derives from theme track color");
    }
    if (type == "pad") {
      saw_themed_pad = true;
      require(canvasSelectionHighlightColor(*item) == theme.pad_fill_color.lighter(160),
              "themed pad highlight derives from theme pad color");
    }
  }

  require(saw_themed_track, "themed scene has selectable track");
  require(saw_themed_pad, "themed scene has selectable pad");
}
