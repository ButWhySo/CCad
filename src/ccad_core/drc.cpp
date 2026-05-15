#include "ccad_core/drc.hpp"

#include <set>
#include <string>

namespace ccad {
namespace {

Diagnostic makeDiagnostic(const std::string& code, const std::string& message,
                          const std::string& object_id) {
  return Diagnostic{
      .severity = "error",
      .code = code,
      .message = message,
      .object_id = object_id,
  };
}

Diagnostic makeWarning(const std::string& code, const std::string& message,
                       const std::string& object_id) {
  return Diagnostic{
      .severity = "warning",
      .code = code,
      .message = message,
      .object_id = object_id,
  };
}

bool hasLayer(const Board& board, const std::string& layer_id) {
  for (const Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return true;
    }
  }
  return false;
}

bool hasNet(const Project& project, const std::string& net_id) {
  for (const Net& net : project.nets) {
    if (net.id == net_id) {
      return true;
    }
  }
  return false;
}

bool containsPoint(const Board& board, const Point& point) {
  const Point min = board.outline.origin;
  const Point max = maxPoint(board.outline);
  return point.x.nanometers >= min.x.nanometers && point.x.nanometers <= max.x.nanometers &&
         point.y.nanometers >= min.y.nanometers && point.y.nanometers <= max.y.nanometers;
}

bool rectContainsPoint(const Rect& rect, const Point& point) {
  const Point max = maxPoint(rect);
  return point.x.nanometers >= rect.origin.x.nanometers && point.x.nanometers <= max.x.nanometers &&
         point.y.nanometers >= rect.origin.y.nanometers && point.y.nanometers <= max.y.nanometers;
}

int orientation(const Point& a, const Point& b, const Point& c) {
  const long double ab_x = static_cast<long double>(b.x.nanometers - a.x.nanometers);
  const long double ab_y = static_cast<long double>(b.y.nanometers - a.y.nanometers);
  const long double ac_x = static_cast<long double>(c.x.nanometers - a.x.nanometers);
  const long double ac_y = static_cast<long double>(c.y.nanometers - a.y.nanometers);
  const long double cross = (ab_x * ac_y) - (ab_y * ac_x);
  if (cross > 0) {
    return 1;
  }
  if (cross < 0) {
    return -1;
  }
  return 0;
}

bool pointOnSegment(const Point& a, const Point& b, const Point& point) {
  if (orientation(a, b, point) != 0) {
    return false;
  }
  const std::int64_t min_x = std::min(a.x.nanometers, b.x.nanometers);
  const std::int64_t max_x = std::max(a.x.nanometers, b.x.nanometers);
  const std::int64_t min_y = std::min(a.y.nanometers, b.y.nanometers);
  const std::int64_t max_y = std::max(a.y.nanometers, b.y.nanometers);
  return point.x.nanometers >= min_x && point.x.nanometers <= max_x &&
         point.y.nanometers >= min_y && point.y.nanometers <= max_y;
}

bool segmentsIntersect(const Point& a, const Point& b, const Point& c, const Point& d) {
  const int o1 = orientation(a, b, c);
  const int o2 = orientation(a, b, d);
  const int o3 = orientation(c, d, a);
  const int o4 = orientation(c, d, b);
  if (o1 != o2 && o3 != o4) {
    return true;
  }
  return (o1 == 0 && pointOnSegment(a, b, c)) || (o2 == 0 && pointOnSegment(a, b, d)) ||
         (o3 == 0 && pointOnSegment(c, d, a)) || (o4 == 0 && pointOnSegment(c, d, b));
}

bool segmentIntersectsRect(const Point& start, const Point& end, const Rect& rect) {
  if (rectContainsPoint(rect, start) || rectContainsPoint(rect, end)) {
    return true;
  }

  const Point top_left = rect.origin;
  const Point bottom_right = maxPoint(rect);
  const Point top_right{.x = bottom_right.x, .y = top_left.y};
  const Point bottom_left{.x = top_left.x, .y = bottom_right.y};
  return segmentsIntersect(start, end, top_left, top_right) ||
         segmentsIntersect(start, end, top_right, bottom_right) ||
         segmentsIntersect(start, end, bottom_right, bottom_left) ||
         segmentsIntersect(start, end, bottom_left, top_left);
}

bool isPositive(const Length& length) {
  return length.nanometers > 0;
}

bool samePoint(const Point& left, const Point& right) {
  return left.x.nanometers == right.x.nanometers && left.y.nanometers == right.y.nanometers;
}

bool endpointTouchesSameNetPrimitive(const Board& board, const TrackSegment& source_track,
                                     const Point& endpoint) {
  if (source_track.net_id.empty()) {
    return true;
  }

  for (const Pad& pad : board.pads) {
    if (pad.net_id == source_track.net_id && samePoint(pad.position, endpoint)) {
      return true;
    }
  }
  for (const Via& via : board.vias) {
    if (via.net_id == source_track.net_id && samePoint(via.position, endpoint)) {
      return true;
    }
  }
  for (const TrackSegment& track : board.tracks) {
    if (track.id == source_track.id || track.net_id != source_track.net_id) {
      continue;
    }
    if (samePoint(track.start, endpoint) || samePoint(track.end, endpoint)) {
      return true;
    }
  }
  return false;
}

void checkPads(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Pad& pad : board.pads) {
    if (!ids.insert(pad.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_PAD_ID", "Pad ID appears more than once", pad.id));
    }
    if (!hasLayer(board, pad.layer_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_PAD_LAYER", "Pad references an unknown layer", pad.id));
    }
    if (!containsPoint(board, pad.position)) {
      diagnostics.push_back(
          makeDiagnostic("PAD_OUTSIDE_BOARD", "Pad position is outside board outline", pad.id));
    }
    if (!isPositive(pad.size.width) || !isPositive(pad.size.height)) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_PAD_SIZE", "Pad width and height must be positive", pad.id));
    }
    if (pad.net_id.empty()) {
      diagnostics.push_back(makeWarning("UNCONNECTED_PAD", "Pad has no assigned net", pad.id));
    } else if (!hasNet(project, pad.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_PAD_NET", "Pad references an unknown net", pad.id));
    }
    for (const Keepout& keepout : board.keepouts) {
      if (rectContainsPoint(keepout.area, pad.position)) {
        diagnostics.push_back(
            makeDiagnostic("PAD_IN_KEEPOUT", "Pad position is inside keepout " + keepout.id,
                           pad.id));
      }
    }
  }
}

void checkVias(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Via& via : board.vias) {
    if (!ids.insert(via.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_VIA_ID", "Via ID appears more than once", via.id));
    }
    if (!containsPoint(board, via.position)) {
      diagnostics.push_back(
          makeDiagnostic("VIA_OUTSIDE_BOARD", "Via position is outside board outline", via.id));
    }
    if (via.net_id.empty()) {
      diagnostics.push_back(makeWarning("UNCONNECTED_VIA", "Via has no assigned net", via.id));
    } else if (!hasNet(project, via.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_VIA_NET", "Via references an unknown net", via.id));
    }
    if (!isPositive(via.diameter) || !isPositive(via.drill)) {
      diagnostics.push_back(makeDiagnostic("INVALID_VIA_SIZE",
                                           "Via diameter and drill must be positive", via.id));
    }
    if (via.drill.nanometers > via.diameter.nanometers) {
      diagnostics.push_back(makeDiagnostic("VIA_DRILL_TOO_LARGE",
                                           "Via drill must be less than or equal to diameter",
                                           via.id));
    }
    for (const Keepout& keepout : board.keepouts) {
      if (rectContainsPoint(keepout.area, via.position)) {
        diagnostics.push_back(
            makeDiagnostic("VIA_IN_KEEPOUT", "Via position is inside keepout " + keepout.id,
                           via.id));
      }
    }
  }
}

void checkTracks(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const TrackSegment& track : board.tracks) {
    if (!ids.insert(track.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_TRACK_ID", "Track ID appears more than once", track.id));
    }
    if (!hasLayer(board, track.layer_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_TRACK_LAYER", "Track references an unknown layer", track.id));
    }
    if (track.net_id.empty()) {
      diagnostics.push_back(
          makeWarning("UNCONNECTED_TRACK", "Track has no assigned net", track.id));
    } else if (!hasNet(project, track.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_TRACK_NET", "Track references an unknown net", track.id));
    }
    if (!containsPoint(board, track.start)) {
      diagnostics.push_back(
          makeDiagnostic("TRACK_START_OUTSIDE_BOARD", "Track start is outside board outline",
                         track.id));
    }
    if (!containsPoint(board, track.end)) {
      diagnostics.push_back(makeDiagnostic("TRACK_END_OUTSIDE_BOARD",
                                           "Track end is outside board outline", track.id));
    }
    if (!isPositive(track.width)) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_TRACK_WIDTH", "Track width must be positive", track.id));
    }
    if (samePoint(track.start, track.end)) {
      diagnostics.push_back(
          makeDiagnostic("ZERO_LENGTH_TRACK", "Track start and end must be different", track.id));
    }
    if (!endpointTouchesSameNetPrimitive(board, track, track.start) ||
        !endpointTouchesSameNetPrimitive(board, track, track.end)) {
      diagnostics.push_back(makeWarning("UNCONNECTED_TRACK_ENDPOINT",
                                        "Track endpoint does not touch a same-net pad, via, or track",
                                        track.id));
    }
    for (const Keepout& keepout : board.keepouts) {
      if (rectContainsPoint(keepout.area, track.start) ||
          rectContainsPoint(keepout.area, track.end)) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_ENDPOINT_IN_KEEPOUT", "Track endpoint is inside keepout " + keepout.id,
            track.id));
      } else if (segmentIntersectsRect(track.start, track.end, keepout.area)) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_CROSSES_KEEPOUT", "Track segment crosses keepout " + keepout.id, track.id));
      }
    }
  }
}

}  // namespace

std::vector<Diagnostic> runDrc(const Project& project) {
  std::vector<Diagnostic> diagnostics;
  if (!project.board.has_value()) {
    return diagnostics;
  }

  const Board& board = *project.board;
  checkPads(project, board, diagnostics);
  checkVias(project, board, diagnostics);
  checkTracks(project, board, diagnostics);
  return diagnostics;
}

}  // namespace ccad
