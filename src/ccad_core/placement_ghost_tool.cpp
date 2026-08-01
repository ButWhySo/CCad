#include "placement_ghost_tool.hpp"
#include "footprint.hpp"
#include "model.hpp"

namespace ccad {

void PlacementGhostTool::setBoard(Board* board) {
    board_ = board;
}

void PlacementGhostTool::startPlacement(std::unique_ptr<Footprint> footprint) {
    activeGhost_ = std::move(footprint);
}

void PlacementGhostTool::updatePosition(double x, double y) {
    (void)x;
    (void)y;
    if (activeGhost_) {
        // In a real implementation, we would update the ghost's transform here
    }
}

void PlacementGhostTool::commitPlacement() {
    if (board_ && activeGhost_) {
        // Stub implementation, board->addFootprint is removed in new model.
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
