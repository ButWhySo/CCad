#include "ccad_core/model.hpp"
#include "ccad_core/track_length_tuning.hpp"

#include <cmath>
#include <iostream>

int main() {
  ccad::Board board;
  board.tracks.push_back(ccad::TrackSegment{.id = "T1", .net_id = "N1",
                                             .start = {.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
                                             .end = {.x = ccad::millimeters(3), .y = ccad::millimeters(4)}});
  board.tracks.push_back(ccad::TrackSegment{.id = "T2", .net_id = "N1",
                                             .start = {.x = ccad::millimeters(3), .y = ccad::millimeters(4)},
                                             .end = {.x = ccad::millimeters(6), .y = ccad::millimeters(4)}});
  board.tracks.push_back(ccad::TrackSegment{.id = "T3", .net_id = "N2",
                                             .start = {.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
                                             .end = {.x = ccad::millimeters(1), .y = ccad::millimeters(0)}});
  board.track_arcs.push_back(ccad::TrackArc{.id = "A1", .net_id = "N1",
                                            .start = {.x = ccad::millimeters(6), .y = ccad::millimeters(4)},
                                            .mid = {.x = ccad::millimeters(7), .y = ccad::millimeters(5)},
                                            .end = {.x = ccad::millimeters(8), .y = ccad::millimeters(4)}});
  board.design_rules.board_thickness = ccad::millimeters(1.6);
  board.vias.push_back(ccad::Via{.id = "V1", .net_id = "N1"});
  ccad::TrackLengthTuning tuning(&board);
  if (std::abs(tuning.calculateCurrentLength("N1") - (8.0 + 3.141592653589793 + 1.6)) > 1e-9 ||
      tuning.calculateCurrentLength("N2") != 1.0 ||
      tuning.calculateCurrentLength("missing") != 0.0) {
    std::cerr << "track length calculation failed\n";
    return 1;
  }
  board.design_rules.use_height_for_length_calcs = false;
  if (std::abs(tuning.calculateCurrentLength("N1") - (8.0 + 3.141592653589793)) > 1e-9) {
    std::cerr << "via height toggle failed\n";
    return 1;
  }
  tuning.setSettings({.targetLength = 10.0, .minAmplitude = 0.2,
                      .maxAmplitude = 0.8, .minSpacing = 0.2});
  if (tuning.applyTuning("T1") || tuning.applyTuning("missing")) {
    std::cerr << "unsupported tuning mutation reported success\n";
    return 1;
  }
  std::cout << "track length tuning tests passed\n";
  return 0;
}
