#include "ccad_core/drc_test_provider_courtyard.hpp"
#include "ccad_core/model.hpp"
#include <cassert>

int main() {
  ccad::Board board;
  board.footprints = {
      ccad::BoardFootprint{.reference = "U1", .front_courtyard = {{{ccad::millimeters(1), ccad::millimeters(1)},
                                                                     {ccad::millimeters(3), ccad::millimeters(1)},
                                                                     {ccad::millimeters(3), ccad::millimeters(3)},
                                                                     {ccad::millimeters(1), ccad::millimeters(3)}}}},
      ccad::BoardFootprint{.reference = "U2", .front_courtyard = {{{ccad::millimeters(2), ccad::millimeters(2)},
                                                                     {ccad::millimeters(4), ccad::millimeters(2)},
                                                                     {ccad::millimeters(4), ccad::millimeters(4)},
                                                                     {ccad::millimeters(2), ccad::millimeters(4)}}}},
  };
  std::vector<ccad::DrcItem> violations;
  ccad::DrcTestProviderCourtyard{}.run(board, violations);
  assert(violations.size() == 1);
  assert(violations.front().getErrorText() == "Courtyard overlap: U1 / U2");
  board.footprints[1].front_courtyard = {{{ccad::millimeters(5), ccad::millimeters(5)},
                                          {ccad::millimeters(6), ccad::millimeters(5)},
                                          {ccad::millimeters(6), ccad::millimeters(6)},
                                          {ccad::millimeters(5), ccad::millimeters(6)}}};
  violations.clear();
  ccad::DrcTestProviderCourtyard{}.run(board, violations);
  assert(violations.empty());
}
