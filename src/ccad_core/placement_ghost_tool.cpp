#include "placement_ghost_tool.hpp"
#include "footprint.hpp"
#include "board.hpp"

namespace ccad {

void PlacementGhostTool::setBoard(Board* board) {
    board_ = board;
}

void PlacementGhostTool::startPlacement(std::unique_ptr<Footprint> footprint) {
    activeGhost_ = std::move(footprint);
}

void PlacementGhostTool::updatePosition(double x, double y) {
    if (activeGhost_) {
        // In a real implementation, we would update the ghost's transform here
    }
}

void PlacementGhostTool::commitPlacement() {
    if (board_ && activeGhost_) {
        board_->addFootprint(std::move(activeGhost_));
    }
    activeGhost_.reset();
}

void PlacementGhostTool::cancelPlacement() {
    activeGhost_.reset();
}

bool PlacementGhostTool::isPlacing() const {
    return activeGhost_ != nullptr;
}

} // namespace ccad
