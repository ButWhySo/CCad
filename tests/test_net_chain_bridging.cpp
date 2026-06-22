#include "ccad_core/net_chain_bridging.hpp"
#include "ccad_core/model.hpp"

#include <iostream>
#include <string>

using namespace ccad;

int main() {
  Project proj;
  Board board;
  Pad p1;
  p1.id = "R1.1";
  p1.component_id = "R1";
  p1.net_id = "NET_A";
  p1.position.x.nanometers = 0;
  p1.position.y.nanometers = 0;
  
  Pad p2;
  p2.id = "R1.2";
  p2.component_id = "R1";
  p2.net_id = "NET_B";
  p2.position.x.nanometers = 1000000; // 1mm
  p2.position.y.nanometers = 0;
  
  board.pads.push_back(p1);
  board.pads.push_back(p2);
  
  proj.boards.push_back(board);

  NetChainBridgingReport report = calculateNetChainBridges(proj, "NET_A");
  
  if (report.bridges.size() != 1) {
    std::cerr << "Expected 1 bridge, got " << report.bridges.size() << std::endl;
    return 1;
  }
  
  if (report.bridges[0].bridge_length_nm != 1000000) {
    std::cerr << "Expected bridge length 1000000, got " << report.bridges[0].bridge_length_nm << std::endl;
    return 1;
  }

  std::cout << "test_net_chain_bridging passed" << std::endl;
  return 0;
}
