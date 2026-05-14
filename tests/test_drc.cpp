#include "ccad_core/drc.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

#include <string>
#include <vector>

namespace {

ccad::Project validBoardProject() {
  ccad::Project project;
  project.id = "proj-drc";
  project.name = "drc";
  project.board = ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                 ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true}},
      .pads = {ccad::Pad{.id = "P1",
                         .component_id = "U1",
                         .pin_name = "1",
                         .net_id = "N1",
                         .layer_id = "F.Cu",
                         .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                         .size = ccad::Size{.width = ccad::millimeters(1.5),
                                            .height = ccad::millimeters(1.0)}}},
      .vias = {ccad::Via{.id = "V1",
                         .net_id = "N1",
                         .position = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
                         .diameter = ccad::millimeters(0.8),
                         .drill = ccad::millimeters(0.4)}},
      .tracks = {ccad::TrackSegment{.id = "T1",
                                    .net_id = "N1",
                                    .layer_id = "F.Cu",
                                    .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                                    .end = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
                                    .width = ccad::millimeters(0.25)}},
  };
  return project;
}

bool hasCode(const std::vector<ccad::Diagnostic>& diagnostics, const std::string& code) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.code == code) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  require(ccad::runDrc(validBoardProject()).empty(), "valid board has no drc diagnostics");

  ccad::Project pad_outside = validBoardProject();
  pad_outside.board->pads.at(0).position.x = ccad::millimeters(99);
  require(hasCode(ccad::runDrc(pad_outside), "PAD_OUTSIDE_BOARD"),
          "drc reports pad outside board");

  ccad::Project unknown_track_layer = validBoardProject();
  unknown_track_layer.board->tracks.at(0).layer_id = "Inner.Cu";
  require(hasCode(ccad::runDrc(unknown_track_layer), "UNKNOWN_TRACK_LAYER"),
          "drc reports unknown track layer");

  ccad::Project via_drill_too_large = validBoardProject();
  via_drill_too_large.board->vias.at(0).drill = ccad::millimeters(1.0);
  require(hasCode(ccad::runDrc(via_drill_too_large), "VIA_DRILL_TOO_LARGE"),
          "drc reports via drill too large");

  ccad::Project zero_length_track = validBoardProject();
  zero_length_track.board->tracks.at(0).end = zero_length_track.board->tracks.at(0).start;
  require(hasCode(ccad::runDrc(zero_length_track), "ZERO_LENGTH_TRACK"),
          "drc reports zero length track");

  ccad::Project duplicate_pad = validBoardProject();
  duplicate_pad.board->pads.push_back(duplicate_pad.board->pads.at(0));
  require(hasCode(ccad::runDrc(duplicate_pad), "DUPLICATE_PAD_ID"),
          "drc reports duplicate pad id");
}
