#ifndef CCAD_CORE_EVENT_DRIVEN_RATNEST_HPP
#define CCAD_CORE_EVENT_DRIVEN_RATNEST_HPP

#include "dynamic_ratnest_graph.hpp"
#include "nearest_neighbor_connectivity.hpp"

namespace ccad {

class Board;

// Wires the connectivity graph to the board's modification event stream for real-time recalculation
class EventDrivenRatnest {
public:
    EventDrivenRatnest(Board* board, DynamicRatnestGraph* graph, NearestNeighborConnectivity* algorithm);
    ~EventDrivenRatnest() = default;

    // Triggered when a footprint is dragged or a track is modified
    // Forces an update of the ratnest graph and recalculates the MST for affected nets
    void onBoardModified();

private:
    Board* board_ = nullptr;
    DynamicRatnestGraph* graph_ = nullptr;
    NearestNeighborConnectivity* algorithm_ = nullptr;
};

} // namespace ccad

#endif // CCAD_CORE_EVENT_DRIVEN_RATNEST_HPP
