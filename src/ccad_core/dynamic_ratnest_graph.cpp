#include "dynamic_ratnest_graph.hpp"
#include "model.hpp"

#include <algorithm>

namespace ccad {

DynamicRatnestGraph::DynamicRatnestGraph(Board* board)
    : board_(board) {
}

void DynamicRatnestGraph::buildGraph() {
    netNodes_.clear();
    if (!board_) return;
    
    auto addNode = [this](const std::string& net, const std::string& id, double x, double y) {
        if (!net.empty() && !id.empty()) netNodes_[net].push_back({id, net, x, y});
    };
    for (const Pad& pad : board_->pads) {
        addNode(pad.net_id, pad.id, pad.position.x.nanometers / 1e6,
                pad.position.y.nanometers / 1e6);
    }
    for (const Via& via : board_->vias) {
        addNode(via.net_id, via.id, via.position.x.nanometers / 1e6,
                via.position.y.nanometers / 1e6);
    }
    for (const TrackSegment& track : board_->tracks) {
        addNode(track.net_id, track.id + ":start", track.start.x.nanometers / 1e6,
                track.start.y.nanometers / 1e6);
        addNode(track.net_id, track.id + ":end", track.end.x.nanometers / 1e6,
                track.end.y.nanometers / 1e6);
    }
    for (auto& [net, nodes] : netNodes_) {
        std::sort(nodes.begin(), nodes.end(), [](const RatnestNode& a, const RatnestNode& b) {
            return a.nodeId < b.nodeId;
        });
    }
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
