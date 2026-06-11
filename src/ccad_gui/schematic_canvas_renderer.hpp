#pragma once

#include "ccad_core/canvas.hpp"
#include "ccad_gui/board_canvas_renderer.hpp" // for CanvasRenderTheme

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QColor>
#include <QString>

void renderSchematicCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene);
void renderSchematicCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene,
                           const CanvasRenderTheme& theme);
