#include "drawing_tool.hpp"
#include "model.hpp"

namespace ccad {

void DrawingTool::setBoard(Board* board) {
    board_ = board;
}

void DrawingTool::drawLine(double x1, double y1, double x2, double y2, int layer) {
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
    (void)layer;
    if (!board_) return;
    
    // Stub for interactive line drawing
}

void DrawingTool::drawCircle(double cx, double cy, double radius, int layer) {
    (void)cx;
    (void)cy;
    (void)radius;
    (void)layer;
    if (!board_) return;

    // Stub for interactive circle drawing
}

void DrawingTool::drawText(const std::string& text, double x, double y, int layer) {
    (void)text;
    (void)x;
    (void)y;
    (void)layer;
    if (!board_) return;

    // Stub for interactive text placement
}

} // namespace ccad
