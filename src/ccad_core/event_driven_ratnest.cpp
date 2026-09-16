#include "event_driven_ratnest.hpp"
#include "model.hpp"
#include <algorithm>
#include <set>

namespace ccad {

EventDrivenRatnest::EventDrivenRatnest(Board* board, DynamicRatnestGraph* graph, NearestNeighborConnectivity* algorithm)
    : board_(board), graph_(graph), algorithm_(algorithm) {
}

void EventDrivenRatnest::onBoardModified() {
    if (!board_ || !graph_ || !algorithm_) return;

    graph_->buildGraph();
    std::vector<RatnestLine> updatedRatnests;
    std::set<std::string> processedNets;
    for (const Pad& pad : board_->pads) {
        if (!pad.net_id.empty()) processedNets.insert(pad.net_id);
    }
    for (const Via& via : board_->vias) {
        if (!via.net_id.empty()) processedNets.insert(via.net_id);
    }
    for (const TrackSegment& track : board_->tracks) {
        if (!track.net_id.empty()) processedNets.insert(track.net_id);
    }
    for (const std::string& net : processedNets) {
        const auto nodes = graph_->getNodesForNet(net);
        const auto lines = algorithm_->computeOptimalRatnests(nodes);
        updatedRatnests.insert(updatedRatnests.end(), lines.begin(), lines.end());
    }
    graph_->setActiveRatnests(updatedRatnests);
}

} // namespace ccad
