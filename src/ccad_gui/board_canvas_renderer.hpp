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

struct CanvasRenderTheme {
  QColor background_color = QColor("#07111f");
  QColor empty_text_color = QColor("#94a3b8");
  QColor grid_color = QColor("#17243a");
  QColor board_outline_color = QColor("#38bdf8");
  QColor board_fill_color = QColor("#0f1b2d");
  QColor placement_region_color = QColor("#22c55e");
  QColor keepout_color = QColor("#f97316");
  QColor track_color = QColor("#ef4444");
  QColor pad_outline_color = QColor("#f472b6");
  QColor pad_fill_color = QColor("#be185d");
  QColor via_outline_color = QColor("#fde68a");
  QColor via_fill_color = QColor("#f59e0b");
  QColor board_label_color = QColor("#cbd5e1");
  QColor error_marker_color = QColor("#ef4444");
  QColor warning_marker_color = QColor("#f59e0b");
};

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
bool canvasUsesShapeSelectionHighlight(const QGraphicsItem& item);
QColor canvasSelectionHighlightColor(const QGraphicsItem& item);
bool selectCanvasObjectById(QGraphicsScene& canvas_scene, const QString& id);
int selectCanvasObjectsByNetId(QGraphicsScene& canvas_scene, const QString& net_id);
QString canvasDiagnosticMarkerObjectId(const QGraphicsItem& item);
QString canvasDiagnosticMarkerSeverity(const QGraphicsItem& item);
