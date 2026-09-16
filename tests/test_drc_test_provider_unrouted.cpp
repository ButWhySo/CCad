#include "ccad_core/drc_test_provider_unrouted.hpp"
#include "ccad_core/model.hpp"

#include <cassert>

int main() {
  ccad::Board board;
  ccad::Pad a; a.id = "P1"; a.net_id = "N1"; a.position = {ccad::millimeters(1), ccad::millimeters(1)};
  ccad::Pad b; b.id = "P2"; b.net_id = "N1"; b.position = {ccad::millimeters(5), ccad::millimeters(1)};
  board.pads = {a, b};
  std::vector<ccad::DrcItem> violations;
  ccad::DrcTestProviderUnrouted provider;
  provider.run(board, violations);
  assert(violations.size() == 1);
  ccad::TrackSegment track; track.id = "T1"; track.net_id = "N1"; track.start = a.position; track.end = b.position;
  board.tracks.push_back(track); violations.clear(); provider.run(board, violations);
  assert(violations.empty());
}
