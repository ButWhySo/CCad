#include "ccad_core/net_info.hpp"
#include <iostream>
#include <cassert>

using namespace ccad;

void testNetInfo() {
  Project project;
  Board board;

  // Add pad
  Pad p1;
  p1.id = "pad1";
  p1.net_id = "VCC";
  p1.position = {{1000}, {1000}};
  board.pads.push_back(p1);

  // Add via
  Via v1;
  v1.id = "via1";
  v1.net_id = "VCC";
  v1.position = {{2000}, {1000}};
  board.vias.push_back(v1);

  // Add track
  TrackSegment track1;
  track1.id = "track1";
  track1.net_id = "VCC";
  track1.start = {{1000}, {1000}};
  track1.end = {{2000}, {1000}};
  board.tracks.push_back(track1);

  project.boards.push_back(board);

  NetInfoReport report = getNetInfo(project, "VCC");
  
  assert(report.net_id == "VCC");
  assert(report.pad_count == 1);
  assert(report.via_count == 1);
  assert(report.track_length_nm == 1000); // 2000 - 1000
  assert(report.has_bounding_box == true);
  assert(report.bbox_min_x_nm == 1000);
  assert(report.bbox_max_x_nm == 2000);
  assert(report.bbox_min_y_nm == 1000);
  assert(report.bbox_max_y_nm == 1000);

  std::cout << "testNetInfo passed.\n";
}

int main() {
  testNetInfo();
  return 0;
}
