#include "nearest_neighbor_connectivity.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ccad {

std::vector<RatnestLine> NearestNeighborConnectivity::computeOptimalRatnests(const std::vector<RatnestNode>& nodes) const {
    std::vector<RatnestLine> ratnests;
    if (nodes.size() < 2) return ratnests;

    std::vector<bool> used(nodes.size(), false);
    std::vector<double> best(nodes.size(), std::numeric_limits<double>::infinity());
    std::vector<std::size_t> parent(nodes.size(), nodes.size());
    best[0] = 0.0;
    for (std::size_t step = 0; step < nodes.size(); ++step) {
        std::size_t current = nodes.size();
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (used[i]) continue;
            if (current == nodes.size() || best[i] < best[current] ||
                (best[i] == best[current] && nodes[i].nodeId < nodes[current].nodeId)) {
                current = i;
            }
        }
        if (current == nodes.size()) break;
        used[current] = true;
        if (parent[current] != nodes.size()) {
            const auto& a = nodes[parent[current]];
            const auto& b = nodes[current];
            if (a.nodeId < b.nodeId) ratnests.push_back({a.nodeId, b.nodeId, b.netCode});
            else ratnests.push_back({b.nodeId, a.nodeId, b.netCode});
        }
        for (std::size_t i = 0; i < nodes.size(); ++i) {
            if (used[i]) continue;
            const double dx = nodes[current].x - nodes[i].x;
            const double dy = nodes[current].y - nodes[i].y;
            const double distance = std::hypot(dx, dy);
            if (distance < best[i] || (distance == best[i] &&
                                       nodes[current].nodeId < nodes[parent[i]].nodeId)) {
                best[i] = distance;
                parent[i] = current;
            }
        }
    }
    return ratnests;
}

} // namespace ccad
