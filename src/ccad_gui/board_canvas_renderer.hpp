#pragma once

#include "ccad_core/canvas.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>

void renderBoardCanvas(QGraphicsScene& canvas_scene, QGraphicsView& canvas_view,
                       const ccad::CanvasScene& scene);
