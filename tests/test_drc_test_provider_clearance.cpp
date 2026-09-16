#include "ccad_core/drc_test_provider_clearance.hpp"
#include "ccad_core/model.hpp"

#include <cassert>

int main() {
  ccad::Board board;
  board.design_rules.copper_clearance = ccad::millimeters(0.2);
  ccad::Pad a;
  a.id = "P1"; a.net_id = "N1"; a.position = {ccad::millimeters(0), ccad::millimeters(0)};
  a.padstack.copper_props["F.Cu"].shape.size = {ccad::millimeters(1), ccad::millimeters(1)};
  ccad::Pad b;
  b.id = "P2"; b.net_id = "N2"; b.position = {ccad::millimeters(1.1), ccad::millimeters(0)};
  b.padstack.copper_props["F.Cu"].shape.size = {ccad::millimeters(1), ccad::millimeters(1)};
  board.pads = {a, b};

  std::vector<ccad::DrcItem> violations;
  ccad::DrcTestProviderClearance provider;
  provider.run(board, violations);
  assert(violations.size() == 1);
  assert(violations.front().getErrorCode() == 1);

  board.pads[1].net_id = "N1";
  violations.clear();
  provider.run(board, violations);
  assert(violations.empty());
}
