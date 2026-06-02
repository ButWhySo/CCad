#pragma once

#include "ccad_core/canvas.hpp"
#include "ccad_core/erc.hpp"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QColor>
#include <QString>

constexpr int kCanvasObjectIdRole = 0;
constexpr int kCanvasObjectTypeRole = 1;
constexpr int kCanvasShapeSelectionHighlightRole = 2;
constexpr int kCanvasDiagnosticMarkerObjectIdRole = 3;
constexpr int kCanvasDiagnosticMarkerSeverityRole = 4;
constexpr int kCanvasSelectionHighlightColorRole = 5;
constexpr int kCanvasObjectNetIdRole = 6;
constexpr int kCanvasObjectLayerIdRole = 7;
constexpr int kCanvasObjectRouteRequestIdRole = 8;
constexpr int kCanvasSelectionHighlightWidthRole = 9;

struct CanvasRenderTheme {
  QColor background_color = QColor("#07111f");
  QColor empty_text_color = QColor("#94a3b8");
  QColor grid_color = QColor("#17243a");
  QColor board_outline_color = QColor("#38bdf8");
  QColor board_fill_color = QColor("#0f1b2d");
  QColor placement_region_color = QColor("#22c55e");
  QColor keepout_color = QColor("#f97316");
  QColor track_color = QColor("#ef4444");
  QColor front_copper_color = QColor("#c83434");
  QColor back_copper_color = QColor("#237a45");
  QColor inner_copper_color = QColor("#c08b2c");
  QColor front_paste_color = QColor("#d8d2d8");
  QColor back_paste_color = QColor("#9ca3af");
  QColor front_mask_color = QColor("#7c3aed");
  QColor back_mask_color = QColor("#2563eb");
  QColor front_silkscreen_color = QColor("#d8d2b8");
  QColor back_silkscreen_color = QColor("#8fb6ff");
  QColor front_fab_color = QColor("#c9cfd8");
  QColor back_fab_color = QColor("#7f8ea3");
  QColor front_courtyard_color = QColor("#b47cff");
  QColor back_courtyard_color = QColor("#7f5fd8");
  QColor edge_cuts_color = QColor("#d6e4ff");
  QColor drawing_color = QColor("#6b7280");
  QColor pad_outline_color = QColor("#f472b6");
  QColor pad_fill_color = QColor("#be185d");
  QColor via_outline_color = QColor("#fde68a");
  QColor via_fill_color = QColor("#f59e0b");
  QColor board_label_color = QColor("#cbd5e1");
  QColor error_marker_color = QColor("#ef4444");
  QColor warning_marker_color = QColor("#f59e0b");
};

inline QColor colorForKiCadLayer(const CanvasRenderTheme& theme, const std::string& layer_id) {
  if (layer_id == "F.Cu") {
    return theme.front_copper_color;
  }
  if (layer_id == "B.Cu") {
    return theme.back_copper_color;
  }
  if (layer_id.starts_with("In") && layer_id.ends_with(".Cu")) {
    return theme.inner_copper_color;
  }
  if (layer_id == "F.Paste") {
    return theme.front_paste_color;
  }
  if (layer_id == "B.Paste") {
    return theme.back_paste_color;
  }
  if (layer_id == "F.Mask") {
    return theme.front_mask_color;
  }
  if (layer_id == "B.Mask") {
    return theme.back_mask_color;
  }
  if (layer_id == "F.SilkS") {
    return theme.front_silkscreen_color;
  }
  if (layer_id == "B.SilkS") {
    return theme.back_silkscreen_color;
  }
  if (layer_id == "F.Fab") {
    return theme.front_fab_color;
  }
  if (layer_id == "B.Fab") {
    return theme.back_fab_color;
  }
  if (layer_id == "F.CrtYd") {
    return theme.front_courtyard_color;
  }
  if (layer_id == "B.CrtYd") {
    return theme.back_courtyard_color;
  }
  if (layer_id == "Edge.Cuts" || layer_id == "Margin") {
    return theme.edge_cuts_color;
  }
  if (layer_id == "Dwgs.User" || layer_id == "Cmts.User" || layer_id.starts_with("Eco") ||
      layer_id.starts_with("User.")) {
    return theme.drawing_color;
  }
  return theme.track_color;
}

void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene);
void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene,
                       const CanvasRenderTheme& theme);
void addDiagnosticMarkers(QGraphicsScene& canvas_scene,
                          const std::vector<ccad::Diagnostic>& diagnostics);
void addDiagnosticMarkers(QGraphicsScene& canvas_scene,
                          const std::vector<ccad::Diagnostic>& diagnostics,
                          const CanvasRenderTheme& theme);
QString canvasObjectId(const QGraphicsItem& item);
QString canvasObjectType(const QGraphicsItem& item);
QString canvasObjectNetId(const QGraphicsItem& item);
QString canvasObjectLayerId(const QGraphicsItem& item);
QString canvasObjectRouteRequestId(const QGraphicsItem& item);
bool canvasUsesShapeSelectionHighlight(const QGraphicsItem& item);
QColor canvasSelectionHighlightColor(const QGraphicsItem& item);
double canvasSelectionHighlightWidth(const QGraphicsItem& item);
bool selectCanvasObjectById(QGraphicsScene& canvas_scene, const QString& id);
int selectCanvasObjectsByNetId(QGraphicsScene& canvas_scene, const QString& net_id);
int selectCanvasObjectsByRouteRequestId(QGraphicsScene& canvas_scene,
                                        const QString& route_request_id);
QString canvasDiagnosticMarkerObjectId(const QGraphicsItem& item);
QString canvasDiagnosticMarkerSeverity(const QGraphicsItem& item);
