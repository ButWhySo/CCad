#include "ccad_core/diff_pair_tuning.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

#include <cmath>

int main() {
  ccad::Board board;
  board.tracks.push_back(ccad::TrackSegment{.id = "P1", .net_id = "D+",
                                             .start = {ccad::millimeters(0), ccad::millimeters(0)},
                                             .end = {ccad::millimeters(3), ccad::millimeters(0)}});
  board.tracks.push_back(ccad::TrackSegment{.id = "N1", .net_id = "D-",
                                             .start = {ccad::millimeters(0), ccad::millimeters(1)},
                                             .end = {ccad::millimeters(4), ccad::millimeters(1)}});
  ccad::DiffPairTuning tuning(&board);
  require(std::abs(tuning.calculateCurrentSkew("D+", "D-") - 1.0) < 1e-9,
          "diff pair skew equals absolute routed length difference in millimetres");
  require(tuning.calculateCurrentSkew("missing", "D-") == 0.0,
          "missing diff pair net has zero skew");
  return 0;
}
