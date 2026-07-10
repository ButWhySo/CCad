#include "router_tool.hpp"
#include "board.hpp"

namespace ccad {

void RouterTool::setBoard(Board* board) {
    board_ = board;
}

void RouterTool::routeTrack(double x1, double y1, double x2, double y2) {
    if (!board_) return;
    
    // Stub for interactive routing bridge delegating to PnsRouter
    // Currently this just stubs out the entry point KiCad uses for manual routing tools.
}

void RouterTool::startRouting(double x, double y, int layer) {
    if (!board_) return;
}

void RouterTool::updateRouting(double x, double y) {
    if (!board_) return;
}

void RouterTool::commitRouting() {
    if (!board_) return;
}

void RouterTool::cancelRouting() {
    if (!board_) return;
}

} // namespace ccad
