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

void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene);
void addDiagnosticMarkers(QGraphicsScene& canvas_scene,
                          const std::vector<ccad::Diagnostic>& diagnostics);
QString canvasObjectId(const QGraphicsItem& item);
QString canvasObjectType(const QGraphicsItem& item);
bool canvasUsesShapeSelectionHighlight(const QGraphicsItem& item);
QColor canvasSelectionHighlightColor(const QGraphicsItem& item);
bool selectCanvasObjectById(QGraphicsScene& canvas_scene, const QString& id);
QString canvasDiagnosticMarkerObjectId(const QGraphicsItem& item);
QString canvasDiagnosticMarkerSeverity(const QGraphicsItem& item);
