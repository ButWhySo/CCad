#ifndef CCAD_CORE_PLACEMENT_GHOST_TOOL_HPP
#define CCAD_CORE_PLACEMENT_GHOST_TOOL_HPP

#include <memory>

namespace ccad {

class Board;
class Footprint;

// Handles the interactive placement of new footprints on the canvas
class PlacementGhostTool {
public:
    PlacementGhostTool() = default;
    ~PlacementGhostTool() = default;

    void setBoard(Board* board);

    // Starts a new placement operation with a cloned footprint instance
    void startPlacement(std::unique_ptr<Footprint> footprint);

    // Updates the ghost position based on cursor movement
    void updatePosition(double x, double y);

    // Commits the current ghost footprint to the board
    void commitPlacement();

    // Cancels the placement operation and discards the ghost
    void cancelPlacement();

    bool isPlacing() const;

private:
    Board* board_ = nullptr;
    std::unique_ptr<Footprint> activeGhost_;
};

} // namespace ccad

#endif // CCAD_CORE_PLACEMENT_GHOST_TOOL_HPP
