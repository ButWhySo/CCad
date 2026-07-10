#include "drawing_tool.hpp"
#include "board.hpp"

namespace ccad {

void DrawingTool::setBoard(Board* board) {
    board_ = board;
}

void DrawingTool::drawLine(double x1, double y1, double x2, double y2, int layer) {
    if (!board_) return;
    
    // Stub for interactive line drawing
}

void DrawingTool::drawCircle(double cx, double cy, double radius, int layer) {
    if (!board_) return;

    // Stub for interactive circle drawing
}

void DrawingTool::drawText(const std::string& text, double x, double y, int layer) {
    if (!board_) return;

    // Stub for interactive text placement
}

} // namespace ccad
