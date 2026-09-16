#include "ccad_core/nearest_neighbor_connectivity.hpp"

#include <cstdlib>
#include <iostream>
#include <set>

int main() {
  const std::vector<ccad::RatnestNode> nodes = {
      {"A", "N1", 0.0, 0.0}, {"B", "N1", 1.0, 0.0},
      {"C", "N1", 1.0, 1.0}, {"D", "N1", 0.0, 1.0}};
  const auto lines = ccad::NearestNeighborConnectivity{}.computeOptimalRatnests(nodes);
  if (lines.size() != 3) {
    std::cerr << "MST must contain node_count - 1 lines\n";
    return EXIT_FAILURE;
  }
  std::set<std::string> nodes_seen;
  for (const auto& line : lines) {
    nodes_seen.insert(line.nodeA);
    nodes_seen.insert(line.nodeB);
    if (line.netCode != "N1" || line.nodeA >= line.nodeB) return EXIT_FAILURE;
  }
  if (nodes_seen.size() != nodes.size()) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
