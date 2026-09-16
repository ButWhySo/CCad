#include "ccad_core/net_tie_drc.hpp"
#include "ccad_core/model.hpp"

#include <cassert>

int main() {
  ccad::Board board;
  ccad::Pad a; a.component_id = "J1"; a.net_id = "N1"; a.position = {ccad::millimeters(5), ccad::millimeters(5)};
  ccad::Pad b; b.component_id = "J1"; b.net_id = "N2"; b.position = {ccad::millimeters(7), ccad::millimeters(5)};
  board.pads = {a, b};
  ccad::NetTieDrc tie(&board);
  tie.registerNetTie("J1", "N1", "N2");
  assert(tie.isIntersectionPermitted("N1", "N2", 6.0, 5.0));
  assert(!tie.isIntersectionPermitted("N1", "N2", 8.0, 5.0));
  assert(tie.validateNetTies().empty());
  board.pads.pop_back();
  assert(tie.validateNetTies().size() == 1);
}
