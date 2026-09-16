#include "ccad_core/dynamic_ratnest_graph.hpp"
#include "ccad_core/event_driven_ratnest.hpp"
#include "ccad_core/model.hpp"

#include <cstdlib>
#include <iostream>

int main() {
  ccad::Board board;
  board.pads.push_back({.id = "P1", .net_id = "N1", .position = {{1000000}, {2000000}}});
  board.vias.push_back({.id = "V1", .net_id = "N1", .position = {{3000000}, {2000000}}});
  board.tracks.push_back({.id = "T1", .net_id = "N1", .start = {{1000000}, {2000000}},
                          .end = {{3000000}, {2000000}}});
  board.pads.push_back({.id = "P2", .net_id = "N2", .position = {{9000000}, {9000000}}});
  ccad::DynamicRatnestGraph graph(&board);
  graph.buildGraph();
  const auto n1 = graph.getNodesForNet("N1");
  if (n1.size() != 4 || n1[0].nodeId != "P1" || n1[1].nodeId != "T1:end" ||
      n1[2].nodeId != "T1:start" || n1[3].nodeId != "V1") {
    std::cerr << "ratnest graph did not collect or sort N1 nodes\n";
    return EXIT_FAILURE;
  }
  if (graph.getNodesForNet("N2").size() != 1) return EXIT_FAILURE;
  ccad::NearestNeighborConnectivity algorithm;
  ccad::EventDrivenRatnest updater(&board, &graph, &algorithm);
  updater.onBoardModified();
  if (graph.getActiveRatnests().size() != 3) {
    std::cerr << "event-driven ratnest update did not publish MST edges\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
