#include "dynamic_ratnest_graph.hpp"
#include "model.hpp"

namespace ccad {

DynamicRatnestGraph::DynamicRatnestGraph(Board* board)
    : board_(board) {
}

void DynamicRatnestGraph::buildGraph() {
    netNodes_.clear();
    if (!board_) return;
    
    // Stub:
    // 1. Scan board for all pads and track endpoints
    // 2. Identify nodes that are not fully connected by physical copper tracks
    // 3. Populate netNodes_ grouped by their netCode
}

std::vector<RatnestNode> DynamicRatnestGraph::getNodesForNet(const std::string& netCode) const {
    auto it = netNodes_.find(netCode);
    if (it != netNodes_.end()) {
        return it->second;
    }
    return {};
}

void DynamicRatnestGraph::setActiveRatnests(const std::vector<RatnestLine>& ratnests) {
    activeRatnests_ = ratnests;
}

std::vector<RatnestLine> DynamicRatnestGraph::getActiveRatnests() const {
    return activeRatnests_;
}

} // namespace ccad
