#include "ccad_core/drc_test_provider_edge_clearance.hpp"
#include "ccad_core/model.hpp"

#include <cassert>

int main() {
  ccad::Board board;
  board.outline = {{ccad::millimeters(0), ccad::millimeters(0)},
                   {ccad::millimeters(10), ccad::millimeters(10)}};
  board.design_rules.copper_edge_clearance = ccad::millimeters(0.5);
  ccad::Via near;
  near.id = "V1"; near.position = {ccad::millimeters(0.8), ccad::millimeters(5)};
  near.diameter = ccad::millimeters(0.8);
  board.vias.push_back(near);
  std::vector<ccad::DrcItem> violations;
  ccad::DrcTestProviderEdgeClearance provider;
  provider.run(board, violations);
  assert(violations.size() == 1);
  board.vias[0].position.x = ccad::millimeters(5);
  violations.clear(); provider.run(board, violations);
  assert(violations.empty());
}
