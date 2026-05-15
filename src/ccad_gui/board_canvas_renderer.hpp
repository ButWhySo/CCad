#pragma once

#include "ccad_core/canvas.hpp"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QString>

constexpr int kCanvasObjectIdRole = 0;
constexpr int kCanvasObjectTypeRole = 1;

void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene);
QString canvasObjectId(const QGraphicsItem& item);
QString canvasObjectType(const QGraphicsItem& item);
