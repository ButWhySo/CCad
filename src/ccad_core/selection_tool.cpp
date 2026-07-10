#include "selection_tool.hpp"
#include "board.hpp"
#include "board_item.hpp"

namespace ccad {

void SelectionTool::setBoard(Board* board) {
    board_ = board;
}

void SelectionTool::selectPoint(double x, double y) {
    if (!board_) return;
    
    // Stub for hit testing and selection logic
    // KiCad uses complex item collectors here
}

void SelectionTool::selectArea(double x1, double y1, double x2, double y2) {
    if (!board_) return;

    // Stub for bounding box area selection logic
}

void SelectionTool::clearSelection() {
    selection_.clear();
}

const std::vector<BoardItem*>& SelectionTool::getSelection() const {
    return selection_;
}

} // namespace ccad
