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
    if (!via.net_id.empty() && !hasNet(project, via.net_id)) {
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
    if (!track.net_id.empty() && !hasNet(project, track.net_id)) {
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
