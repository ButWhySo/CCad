#ifndef CCAD_CORE_NEAREST_NEIGHBOR_CONNECTIVITY_HPP
#define CCAD_CORE_NEAREST_NEIGHBOR_CONNECTIVITY_HPP

#include "dynamic_ratnest_graph.hpp"
#include <string>
#include <vector>

namespace ccad {

// Computes the optimal (shortest) flying leads between unconnected net nodes
class NearestNeighborConnectivity {
public:
    NearestNeighborConnectivity() = default;
    ~NearestNeighborConnectivity() = default;

    // Takes a set of unconnected nodes on a specific net and calculates a Minimum Spanning Tree (MST)
    // Returns the calculated RatnestLines representing the optimal shortest-path flying leads
    std::vector<RatnestLine> computeOptimalRatnests(const std::vector<RatnestNode>& nodes) const;
};

} // namespace ccad

#endif // CCAD_CORE_NEAREST_NEIGHBOR_CONNECTIVITY_HPP
