#include "ccad_gui/board_canvas_renderer.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QColor>
#include <QGraphicsPathItem>
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
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "rect",
      .x_units = 5.0,
      .y_units = 6.0,
      .width_units = 1.5,
      .height_units = 1.0,
  });
  return scene;
}

ccad::CanvasScene complexPadShapeScene() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.view_width_units = 24.0;
  scene.view_height_units = 20.0;
  scene.pads.push_back(ccad::CanvasPad{
      .id = "TRAP",
      .net_id = "N1",
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "trapezoid",
      .x_units = 6.0,
      .y_units = 8.0,
      .width_units = 3.0,
      .height_units = 1.6,
  });
  scene.pads.push_back(ccad::CanvasPad{
      .id = "CHAMFER",
      .net_id = "N1",
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "chamfered_rect",
      .x_units = 14.0,
      .y_units = 8.0,
      .width_units = 3.0,
      .height_units = 1.6,
      .chamfer_ratio = 0.25,
  });
  return scene;
}

ccad::CanvasScene throughHoleScene() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.view_width_units = 20.0;
  scene.view_height_units = 20.0;
  scene.pads.push_back(ccad::CanvasPad{
      .id = "J1.1",
      .net_id = "AC1",
      .layers = {"*.Cu", "*.Mask"},
      .type = "thru_hole",
      .shape = "circle",
      .x_units = 10.0,
      .y_units = 10.0,
      .width_units = 2.0,
      .height_units = 2.0,
      .drill_units = 1.0,
  });
  return scene;
}

ccad::CanvasScene maskPasteScene() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.view_width_units = 20.0;
  scene.view_height_units = 20.0;
  scene.layers.push_back(ccad::CanvasLayer{
      .id = "F.Cu",
      .name = "Front copper",
      .kind = "copper",
      .visible = false,
  });
  scene.layers.push_back(ccad::CanvasLayer{
      .id = "F.Mask",
      .name = "Front solder mask",
      .kind = "mask",
      .visible = true,
  });
  scene.layers.push_back(ccad::CanvasLayer{
      .id = "F.Paste",
      .name = "Front solder paste",
      .kind = "paste",
      .visible = true,
  });
  scene.pads.push_back(ccad::CanvasPad{
      .id = "U1.1",
      .net_id = "N1",
      .layers = {"F.Cu", "F.Mask", "F.Paste"},
      .type = "smd",
      .shape = "roundrect",
      .x_units = 10.0,
      .y_units = 10.0,
      .width_units = 2.0,
      .height_units = 1.2,
      .roundrect_rratio = 0.25,
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
      require(canvasSelectionHighlightColor(*item) == QColor("#c83434").lighter(160),
              "track highlight derives from track color");
      require(canvasSelectionHighlightWidth(*item) >= 3.5,
              "track selection highlight is wider than a centerline");
    }
    if (type == "pad") {
      saw_pad = true;
      require(canvasUsesShapeSelectionHighlight(*item), "pad uses shape selection highlight");
      require(canvasSelectionHighlightColor(*item) == QColor("#c83434").lighter(160),
              "pad highlight derives from pad color");
    }
  }

  require(saw_track, "test scene has selectable track");
  require(saw_pad, "test scene has selectable pad");

  CanvasRenderTheme theme;
  theme.track_color = QColor("#22c55e");
  theme.front_copper_color = QColor("#22c55e");
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
      require(canvasSelectionHighlightColor(*item) == theme.front_copper_color.lighter(160),
              "themed pad highlight derives from theme pad color");
    }
  }

  require(saw_themed_track, "themed scene has selectable track");
  require(saw_themed_pad, "themed scene has selectable pad");

  ccad::CanvasScene copper_scene = selectionScene();
  copper_scene.tracks.push_back(ccad::CanvasTrack{
      .id = "B1",
      .net_id = "N2",
      .layer_id = "B.Cu",
      .source_route_request_id = {},
      .start_x_units = 2.0,
      .start_y_units = 16.0,
      .end_x_units = 16.0,
      .end_y_units = 16.0,
      .width_units = 0.35,
  });
  QGraphicsScene copper_rendered;
  renderBoardCanvas(copper_rendered, copper_scene);
  QColor front_highlight;
  QColor back_highlight;
  for (QGraphicsItem* item : copper_rendered.items()) {
    if (canvasObjectId(*item) == "T1") {
      front_highlight = canvasSelectionHighlightColor(*item);
    }
    if (canvasObjectId(*item) == "B1") {
      back_highlight = canvasSelectionHighlightColor(*item);
    }
  }
  require(front_highlight.isValid(), "front copper track is rendered");
  require(back_highlight.isValid(), "back copper track is rendered");
  require(front_highlight != back_highlight, "front and back copper layers render with different colors");

  QGraphicsScene through_hole_scene;
  renderBoardCanvas(through_hole_scene, throughHoleScene());
  bool saw_round_pad_path = false;
  bool saw_drill_hole = false;
  for (QGraphicsItem* item : through_hole_scene.items()) {
    if (canvasObjectType(*item) == "pad") {
      auto* path_item = dynamic_cast<QGraphicsPathItem*>(item);
      require(path_item != nullptr, "through-hole pad is rendered as a path item");
      saw_round_pad_path = path_item->path().elementCount() > 5;
    }
    if (canvasObjectType(*item) == "pad-drill") {
      saw_drill_hole = true;
    }
  }
  require(saw_round_pad_path, "through-hole circular pad does not render as a rectangle");
  require(saw_drill_hole, "through-hole circular pad renders a drill opening");

  QGraphicsScene complex_scene;
  renderBoardCanvas(complex_scene, complexPadShapeScene());
  bool saw_trapezoid = false;
  bool saw_chamfered_rect = false;
  for (QGraphicsItem* item : complex_scene.items()) {
    auto* path_item = dynamic_cast<QGraphicsPathItem*>(item);
    if (path_item == nullptr) {
      continue;
    }
    if (canvasObjectId(*item) == "TRAP") {
      saw_trapezoid = path_item->path().elementCount() >= 5;
    }
    if (canvasObjectId(*item) == "CHAMFER") {
      saw_chamfered_rect = path_item->path().elementCount() >= 6;
    }
  }
  require(saw_trapezoid, "trapezoid pad renders as a polygon path");
  require(saw_chamfered_rect, "chamfered pad renders as a chamfered polygon path");

  const CanvasRenderTheme default_theme;
  require(colorForKiCadLayer(default_theme, "F.Mask") != colorForKiCadLayer(default_theme, "F.Cu"),
          "front mask has a distinct layer color from front copper");
  require(colorForKiCadLayer(default_theme, "F.Paste") != colorForKiCadLayer(default_theme, "F.Cu"),
          "front paste has a distinct layer color from front copper");
  require(colorForKiCadLayer(default_theme, "B.Mask") != colorForKiCadLayer(default_theme, "B.Cu"),
          "back mask has a distinct layer color from back copper");
  require(colorForKiCadLayer(default_theme, "B.Paste") != colorForKiCadLayer(default_theme, "B.Cu"),
          "back paste has a distinct layer color from back copper");

  QGraphicsScene mask_paste_scene;
  renderBoardCanvas(mask_paste_scene, maskPasteScene());
  bool saw_hidden_copper_pad = false;
  bool saw_mask_aperture = false;
  bool saw_paste_aperture = false;
  for (QGraphicsItem* item : mask_paste_scene.items()) {
    if (canvasObjectType(*item) == "pad") {
      saw_hidden_copper_pad = true;
    }
    if (canvasObjectType(*item) == "pad-mask") {
      saw_mask_aperture = true;
      require(canvasObjectLayerId(*item) == "F.Mask", "mask aperture records mask layer");
    }
    if (canvasObjectType(*item) == "pad-paste") {
      saw_paste_aperture = true;
      require(canvasObjectLayerId(*item) == "F.Paste", "paste aperture records paste layer");
    }
  }
  require(!saw_hidden_copper_pad, "hidden copper layer does not render as copper because mask is visible");
  require(saw_mask_aperture, "visible solder mask aperture renders as a separate layer object");
  require(saw_paste_aperture, "visible solder paste aperture renders as a separate layer object");
}
