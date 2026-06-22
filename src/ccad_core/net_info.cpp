#include "ccad_core/net_info.hpp"

#include <algorithm>
#include <cmath>

namespace ccad {

NetInfoReport getNetInfo(const Project& project, const std::string& net_id) {
  NetInfoReport report;
  report.net_id = net_id;
  report.pending_kicad_features = {
      "pad-to-die internal IC length delays",
      "netclass assignments and clearances",
      "cross-board net connectivity",
      "differential pair net coupling"
  };

  if (project.boards.empty()) {
    report.diagnostics.push_back("project has no board");
    return report;
  }

  const Board& board = project.boards.front();

  int64_t min_x = 0;
  int64_t min_y = 0;
  int64_t max_x = 0;
  int64_t max_y = 0;
  bool first_point = true;

  auto update_bbox = [&](int64_t x, int64_t y) {
    if (first_point) {
      min_x = max_x = x;
      min_y = max_y = y;
      first_point = false;
    } else {
      min_x = std::min(min_x, x);
      max_x = std::max(max_x, x);
      min_y = std::min(min_y, y);
      max_y = std::max(max_y, y);
    }
  };

  // Check pads
  for (const Pad& pad : board.pads) {
    if (pad.net_id == net_id) {
      report.pad_count++;
      update_bbox(pad.position.x.nanometers, pad.position.y.nanometers);
    }
  }

  // Check vias
  for (const Via& via : board.vias) {
    if (via.net_id == net_id) {
      report.via_count++;
      update_bbox(via.position.x.nanometers, via.position.y.nanometers);
    }
  }

  // Check tracks
  for (const TrackSegment& track : board.tracks) {
    if (track.net_id == net_id) {
      // Calculate length
      double dx = static_cast<double>(track.start.x.nanometers - track.end.x.nanometers);
      double dy = static_cast<double>(track.start.y.nanometers - track.end.y.nanometers);
      report.track_length_nm += static_cast<int64_t>(std::sqrt(dx * dx + dy * dy));

      update_bbox(track.start.x.nanometers, track.start.y.nanometers);
      update_bbox(track.end.x.nanometers, track.end.y.nanometers);
    }
  }

  if (!first_point) {
    report.has_bounding_box = true;
    report.bbox_min_x_nm = min_x;
    report.bbox_min_y_nm = min_y;
    report.bbox_max_x_nm = max_x;
    report.bbox_max_y_nm = max_y;
  }

  return report;
}

}  // namespace ccad
