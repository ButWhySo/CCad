#include "ccad_core/model.hpp"
#include "ccad_core/router_tool.hpp"
#include <cassert>

int main() {
  ccad::Board board;
  board.tracks.push_back(ccad::TrackSegment{.id = "wide", .net_id = "N2", .layer_id = "F.Cu",
      .start = {ccad::millimeters(5), ccad::millimeters(1)},
      .end = {ccad::millimeters(5), ccad::millimeters(4)}, .width = ccad::millimeters(4.0)});
  ccad::RouterTool router;
  router.setBoard(&board);
  router.setActiveNet("N1");
  router.routeTrack(3.2, 1, 3.2, 4);
  assert(board.tracks.size() == 1);
  assert(router.routeBlocked());
  return 0;
}
