#include "ccad_core/drc.hpp"

#include "ccad_core/board_design_settings.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

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

bool hasBoardObject(const Board& board, const std::string& object_id) {
  for (const Pad& pad : board.pads) {
    if (pad.id == object_id) {
      return true;
    }
  }
  for (const Via& via : board.vias) {
    if (via.id == object_id) {
      return true;
    }
  }
  for (const TrackSegment& track : board.tracks) {
    if (track.id == object_id) {
      return true;
    }
  }
  return false;
}

std::string boardObjectNetId(const Board& board, const std::string& object_id) {
  for (const Pad& pad : board.pads) {
    if (pad.id == object_id) {
      return pad.net_id;
    }
  }
  for (const Via& via : board.vias) {
    if (via.id == object_id) {
      return via.net_id;
    }
  }
  for (const TrackSegment& track : board.tracks) {
    if (track.id == object_id) {
      return track.net_id;
    }
  }
  return "";
}

bool hasNet(const Project& project, const std::string& net_id) {
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return false;
  }
  for (const Net& net : schematic->nets) {
    if (net.id == net_id) {
      return true;
    }
  }
  return false;
}

const Net* findNet(const Project& project, const std::string& net_id) {
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return nullptr;
  }
  for (const Net& net : schematic->nets) {
    if (net.id == net_id) {
      return &net;
    }
  }
  return nullptr;
}

bool containsPoint(const Board& board, const Point& point) {
  const Point min = board.outline.origin;
  const Point max = maxPoint(board.outline);
  return point.x.nanometers >= min.x.nanometers && point.x.nanometers <= max.x.nanometers &&
         point.y.nanometers >= min.y.nanometers && point.y.nanometers <= max.y.nanometers;
}

const SchSymbol* findComponent(const Project& project, const std::string& component_id) {
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return nullptr;
  }
  for (const SchSymbol& component : schematic->symbols) {
    if (component.id == component_id) {
      return &component;
    }
  }
  return nullptr;
}

bool componentHasPin(const SchSymbol& component, const std::string& pin_name) {
  for (const SchPin& pin : component.pins) {
    if (pin.name == pin_name) {
      return true;
    }
  }
  return false;
}

bool netContainsMember(const Net& net, const std::string& component_id,
                       const std::string& pin_name) {
  for (const NetMember& member : net.members) {
    if (member.component_id == component_id && member.pin_name == pin_name) {
      return true;
    }
  }
  return false;
}

long double distanceToBoardEdge(const Board& board, const Point& point) {
  const std::int64_t min_x = board.outline.origin.x.nanometers;
  const std::int64_t min_y = board.outline.origin.y.nanometers;
  const Point max = maxPoint(board.outline);
  const std::int64_t max_x = max.x.nanometers;
  const std::int64_t max_y = max.y.nanometers;
  const std::int64_t left = point.x.nanometers - min_x;
  const std::int64_t right = max_x - point.x.nanometers;
  const std::int64_t bottom = point.y.nanometers - min_y;
  const std::int64_t top = max_y - point.y.nanometers;
  const std::int64_t nearest = std::min(std::min(left, right), std::min(bottom, top));
  return static_cast<long double>(nearest);
}

bool rectContainsPoint(const Rect& rect, const Point& point) {
  const Point max = maxPoint(rect);
  return point.x.nanometers >= rect.origin.x.nanometers && point.x.nanometers <= max.x.nanometers &&
         point.y.nanometers >= rect.origin.y.nanometers && point.y.nanometers <= max.y.nanometers;
}

std::vector<Point> rectCorners(const Rect& rect) {
  const Point top_left = rect.origin;
  const Point bottom_right = maxPoint(rect);
  const Point top_right{.x = bottom_right.x, .y = top_left.y};
  const Point bottom_left{.x = top_left.x, .y = bottom_right.y};
  return {top_left, top_right, bottom_right, bottom_left};
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

long double distanceBetweenPoints(const Point& left, const Point& right) {
  const long double dx = static_cast<long double>(left.x.nanometers - right.x.nanometers);
  const long double dy = static_cast<long double>(left.y.nanometers - right.y.nanometers);
  return std::hypotl(dx, dy);
}

long double drcDistancePointToSegment(const Point& point, const Point& start, const Point& end) {
  return static_cast<long double>(ccad::distancePointToSegment(point, start, end));
}

long double distanceBetweenSegments(const Point& first_start, const Point& first_end,
                                    const Point& second_start, const Point& second_end) {
  if (segmentsIntersect(first_start, first_end, second_start, second_end)) {
    return 0.0L;
  }
  return std::min({drcDistancePointToSegment(first_start, second_start, second_end),
                   drcDistancePointToSegment(first_end, second_start, second_end),
                   drcDistancePointToSegment(second_start, first_start, first_end),
                   drcDistancePointToSegment(second_end, first_start, first_end)});
}

std::vector<Point> padCorners(const Pad& pad) {
  constexpr long double pi = 3.141592653589793238462643383279502884L;
  const long double radians = (static_cast<long double>(pad.rotation_degrees) * pi) / 180.0L;
  const long double cosine = std::cos(radians);
  const long double sine = std::sin(radians);
  const long double center_x = static_cast<long double>(pad.position.x.nanometers);
  const long double center_y = static_cast<long double>(pad.position.y.nanometers);
  const Size psize = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
  const long double half_width = static_cast<long double>(psize.width.nanometers) / 2.0L;
  const long double half_height = static_cast<long double>(psize.height.nanometers) / 2.0L;

  std::vector<Point> corners;
  corners.reserve(4);
  for (const auto& local : {std::pair<long double, long double>{-half_width, -half_height},
                            std::pair<long double, long double>{half_width, -half_height},
                            std::pair<long double, long double>{half_width, half_height},
                            std::pair<long double, long double>{-half_width, half_height}}) {
    const long double rotated_x = center_x + (local.first * cosine) - (local.second * sine);
    const long double rotated_y = center_y + (local.first * sine) + (local.second * cosine);
    corners.push_back(Point{.x = nanometers(static_cast<std::int64_t>(std::llround(rotated_x))),
                            .y = nanometers(static_cast<std::int64_t>(std::llround(rotated_y)))});
  }
  return corners;
}

bool pointInPolygon(const Point& point, const std::vector<Point>& polygon) {
  bool inside = false;
  for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const long double ix = static_cast<long double>(polygon.at(i).x.nanometers);
    const long double iy = static_cast<long double>(polygon.at(i).y.nanometers);
    const long double jx = static_cast<long double>(polygon.at(j).x.nanometers);
    const long double jy = static_cast<long double>(polygon.at(j).y.nanometers);
    const long double px = static_cast<long double>(point.x.nanometers);
    const long double py = static_cast<long double>(point.y.nanometers);
    if (pointOnSegment(polygon.at(j), polygon.at(i), point)) {
      return true;
    }
    const bool crosses_y = (iy > py) != (jy > py);
    if (crosses_y && px < (((jx - ix) * (py - iy)) / (jy - iy)) + ix) {
      inside = !inside;
    }
  }
  return inside;
}

bool segmentIntersectsPolygon(const Point& start, const Point& end,
                              const std::vector<Point>& polygon) {
  if (pointInPolygon(start, polygon) || pointInPolygon(end, polygon)) {
    return true;
  }
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const Point& edge_start = polygon.at(i);
    const Point& edge_end = polygon.at((i + 1) % polygon.size());
    if (segmentsIntersect(start, end, edge_start, edge_end)) {
      return true;
    }
  }
  return false;
}

Rect inflateRect(const Rect& rect, std::int64_t margin_nm) {
  const std::int64_t min_x = rect.origin.x.nanometers - margin_nm;
  const std::int64_t min_y = rect.origin.y.nanometers - margin_nm;
  const std::int64_t width_nm = rect.size.width.nanometers + (2 * margin_nm);
  const std::int64_t height_nm = rect.size.height.nanometers + (2 * margin_nm);
  return Rect{.origin = Point{.x = nanometers(min_x), .y = nanometers(min_y)},
              .size = Size{.width = nanometers(width_nm), .height = nanometers(height_nm)}};
}

bool polygonIntersectsRect(const std::vector<Point>& polygon, const Rect& rect) {
  const std::vector<Point> rect_polygon = rectCorners(rect);
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const Point& edge_start = polygon.at(i);
    const Point& edge_end = polygon.at((i + 1) % polygon.size());
    if (segmentIntersectsRect(edge_start, edge_end, rect)) {
      return true;
    }
  }
  for (const Point& point : polygon) {
    if (rectContainsPoint(rect, point)) {
      return true;
    }
  }
  for (const Point& rect_point : rect_polygon) {
    if (pointInPolygon(rect_point, polygon)) {
      return true;
    }
  }
  return false;
}

long double distancePointToRect(const Point& point, const Rect& rect) {
  const Point rect_max = maxPoint(rect);
  const long double px = static_cast<long double>(point.x.nanometers);
  const long double py = static_cast<long double>(point.y.nanometers);
  const long double min_x = static_cast<long double>(rect.origin.x.nanometers);
  const long double min_y = static_cast<long double>(rect.origin.y.nanometers);
  const long double max_x = static_cast<long double>(rect_max.x.nanometers);
  const long double max_y = static_cast<long double>(rect_max.y.nanometers);
  const long double clamped_x = std::clamp(px, min_x, max_x);
  const long double clamped_y = std::clamp(py, min_y, max_y);
  return std::hypotl(px - clamped_x, py - clamped_y);
}

long double distancePointToPolygon(const Point& point, const std::vector<Point>& polygon) {
  if (pointInPolygon(point, polygon)) {
    return 0.0L;
  }
  long double distance = drcDistancePointToSegment(point, polygon.at(0), polygon.at(1));
  for (std::size_t i = 1; i < polygon.size(); ++i) {
    distance = std::min(distance, drcDistancePointToSegment(point, polygon.at(i),
                                                           polygon.at((i + 1) % polygon.size())));
  }
  return distance;
}

long double distanceSegmentToPolygon(const Point& start, const Point& end,
                                     const std::vector<Point>& polygon) {
  if (segmentIntersectsPolygon(start, end, polygon)) {
    return 0.0L;
  }
  long double distance = distanceBetweenSegments(start, end, polygon.at(0), polygon.at(1));
  for (std::size_t i = 1; i < polygon.size(); ++i) {
    distance =
        std::min(distance, distanceBetweenSegments(start, end, polygon.at(i),
                                                  polygon.at((i + 1) % polygon.size())));
  }
  return distance;
}

long double distanceBetweenPolygons(const std::vector<Point>& first,
                                    const std::vector<Point>& second) {
  for (std::size_t i = 0; i < first.size(); ++i) {
    if (pointInPolygon(first.at(i), second)) {
      return 0.0L;
    }
  }
  for (std::size_t i = 0; i < second.size(); ++i) {
    if (pointInPolygon(second.at(i), first)) {
      return 0.0L;
    }
  }

  long double distance = distanceBetweenSegments(first.at(0), first.at(1), second.at(0),
                                                 second.at(1));
  for (std::size_t first_index = 0; first_index < first.size(); ++first_index) {
    for (std::size_t second_index = 0; second_index < second.size(); ++second_index) {
      distance = std::min(
          distance,
          distanceBetweenSegments(first.at(first_index),
                                  first.at((first_index + 1) % first.size()),
                                  second.at(second_index),
                                  second.at((second_index + 1) % second.size())));
    }
  }
  return distance;
}

bool sameNonEmptyNet(const std::string& left, const std::string& right) {
  return !left.empty() && left == right;
}

bool differentNonEmptyNets(const std::string& left, const std::string& right) {
  return !left.empty() && !right.empty() && left != right;
}

bool isCopperLayer(const Board& board, const std::string& layer_id) {
  for (const Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return layer.kind == "copper";
    }
  }
  return false;
}

bool shareCopperLayer(const Board& board, const std::string& left_layer,
                      const std::string& right_layer) {
  if (left_layer == right_layer && isCopperLayer(board, left_layer)) return true;
  if (left_layer.starts_with("*.") && right_layer.starts_with("*.")) return true; // simplified
  if (left_layer.starts_with("*.") && isCopperLayer(board, right_layer)) return true;
  if (right_layer.starts_with("*.") && isCopperLayer(board, left_layer)) return true;
  return false;
}

bool padSharesCopperLayer(const Board& board, const Pad& pad, const std::string& layer) {
  for (const std::string& l : pad.padstack.layer_set) {
    if (shareCopperLayer(board, l, layer)) return true;
  }
  return false;
}

bool padsShareCopperLayer(const Board& board, const Pad& p1, const Pad& p2) {
  for (const std::string& l1 : p1.padstack.layer_set) {
    for (const std::string& l2 : p2.padstack.layer_set) {
      if (shareCopperLayer(board, l1, l2)) return true;
    }
  }
  return false;
}

bool padOnCopperLayer(const Board& board, const Pad& pad) {
  for (const std::string& l : pad.padstack.layer_set) {
    if (l.starts_with("*.") || isCopperLayer(board, l)) return true;
  }
  return false;
}


bool hasValidPadGeometry(const Pad& pad) {
  const Size psize = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
  return isPositive(psize.width) && isPositive(psize.height);
}

bool hasValidTrackGeometry(const TrackSegment& track) { return isPositive(track.width); }

bool hasValidViaGeometry(const Via& via) {
  return isPositive(via.diameter) && isPositive(via.drill) &&
         via.drill.nanometers <= via.diameter.nanometers;
}

void addClearanceDiagnostic(std::vector<Diagnostic>& diagnostics, const std::string& checked_id,
                            const std::string& other_id, const Length& clearance) {
  diagnostics.push_back(makeDiagnostic("COPPER_CLEARANCE",
                                       "Different-net copper is closer than configured clearance " +
                                           std::to_string(clearance.nanometers) + " nm to " +
                                           other_id,
                                       checked_id));
}

bool endpointTouchesSameNetPrimitive(const Board& board, const TrackSegment& source_track,
                                     const Point& endpoint) {
  if (source_track.net_id.empty()) {
    return true;
  }

  for (const Pad& pad : board.pads) {
    if (pad.net_id == source_track.net_id &&
        padSharesCopperLayer(board, pad, source_track.layer_id) &&
        pointInPolygon(endpoint, padCorners(pad))) {
      return true;
    }
  }
  for (const Via& via : board.vias) {
    if (via.net_id == source_track.net_id &&
        distanceBetweenPoints(via.position, endpoint) <=
            (static_cast<long double>(via.diameter.nanometers) / 2.0L)) {
      return true;
    }
  }
  for (const TrackSegment& track : board.tracks) {
    if (track.id == source_track.id || track.net_id != source_track.net_id ||
        track.layer_id != source_track.layer_id) {
      continue;
    }
    if (samePoint(track.start, endpoint) || samePoint(track.end, endpoint) ||
        pointOnSegment(track.start, track.end, endpoint)) {
      return true;
    }
  }
  return false;
}

void checkPads(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  const bool has_schematic = primarySchematic(project) != nullptr;
  std::set<std::string> ids;
  for (const Pad& pad : board.pads) {
    if (pad.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_PAD_ID", "Pad ID must not be empty", pad.id));
    }
    if (!ids.insert(pad.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_PAD_ID", "Pad ID appears more than once", pad.id));
    }
    if (pad.component_id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_PAD_COMPONENT",
                                           "Pad component_id must not be empty", pad.id));
    } else if (has_schematic && findComponent(project, pad.component_id) == nullptr) {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_PAD_COMPONENT",
                                           "Pad references an unknown component", pad.id));
    }
    if (pad.pin_name.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_PAD_PIN", "Pad pin_name must not be empty", pad.id));
    } else if (has_schematic) {
      const SchSymbol* component = findComponent(project, pad.component_id);
      if (component != nullptr && !componentHasPin(*component, pad.pin_name)) {
        diagnostics.push_back(
            makeDiagnostic("UNKNOWN_PAD_PIN", "Pad references an unknown component pin", pad.id));
      }
    }
    bool has_unknown = false;
    for (const std::string& l : pad.padstack.layer_set) {
      if (!l.starts_with("*.") && !hasLayer(board, l)) has_unknown = true;
    }
    if (has_unknown) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_PAD_LAYER", "Pad references an unknown layer", pad.id));
    } else if (!padOnCopperLayer(board, pad)) {
      diagnostics.push_back(
          makeDiagnostic("PAD_NON_COPPER_LAYER", "Pad must be on a copper layer", pad.id));
    }
    if (!containsPoint(board, pad.position)) {
      diagnostics.push_back(
          makeDiagnostic("PAD_OUTSIDE_BOARD", "Pad position is outside board outline", pad.id));
    }
    const Size psize = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
    const bool pad_size_positive = isPositive(psize.width) && isPositive(psize.height);
    if (!pad_size_positive) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_PAD_SIZE", "Pad width and height must be positive", pad.id));
    } else {
      const std::vector<Point> corners = padCorners(pad);
      bool all_corners_inside = true;
      for (const Point& corner : corners) {
        if (!containsPoint(board, corner)) {
          all_corners_inside = false;
          break;
        }
      }
      if (!all_corners_inside) {
        diagnostics.push_back(makeDiagnostic(
            "PAD_GEOMETRY_OUTSIDE_BOARD", "Pad geometry extends outside board outline", pad.id));
      }
    }
    if ((pad.type == "through_hole" || pad.type == "thru_hole") &&
        (!isPositive(pad.padstack.drill.size.width) ||
         !isPositive(pad.padstack.drill.size.height))) {
      diagnostics.push_back(makeDiagnostic(
          "PAD_THROUGH_HOLE_WITHOUT_DRILL",
          "Through-hole pad must define a positive drill geometry", pad.id));
    } else if ((pad.type == "through_hole" || pad.type == "thru_hole") &&
               board.design_rules.min_through_hole_drill.nanometers > 0 &&
               std::min(pad.padstack.drill.size.width.nanometers,
                        pad.padstack.drill.size.height.nanometers) <
                   board.design_rules.min_through_hole_drill.nanometers) {
      diagnostics.push_back(makeDiagnostic(
          "PAD_DRILL_BELOW_MINIMUM",
          "Through-hole pad drill is below configured minimum " +
              std::to_string(board.design_rules.min_through_hole_drill.nanometers) + " nm",
          pad.id));
    }
    if (pad.net_id.empty()) {
      diagnostics.push_back(makeWarning("UNCONNECTED_PAD", "Pad has no assigned net", pad.id));
    } else if (has_schematic && !hasNet(project, pad.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_PAD_NET", "Pad references an unknown net", pad.id));
    } else if (has_schematic && !pad.component_id.empty() && !pad.pin_name.empty()) {
      const Net* net = findNet(project, pad.net_id);
      if (net != nullptr && !netContainsMember(*net, pad.component_id, pad.pin_name)) {
        diagnostics.push_back(makeDiagnostic(
            "PAD_NET_MEMBER_MISMATCH",
            "Pad net does not contain the pad component/pin as a logical member", pad.id));
      }
    }
    if (pad_size_positive) {
      const long double edge_clearance = static_cast<long double>(
          board.design_rules.copper_edge_clearance.nanometers);
      if (edge_clearance > 0.0L) {
        for (const Point& corner : padCorners(pad)) {
          if (distanceToBoardEdge(board, corner) < edge_clearance) {
            diagnostics.push_back(makeDiagnostic(
                "PAD_EDGE_CLEARANCE", "Pad copper is closer to board edge than configured clearance",
                pad.id));
            break;
          }
        }
      }
      for (const Keepout& keepout : board.keepouts) {
        if (polygonIntersectsRect(padCorners(pad), keepout.area)) {
          diagnostics.push_back(
              makeDiagnostic("PAD_IN_KEEPOUT", "Pad geometry intersects keepout " + keepout.id,
                             pad.id));
        }
      }
    }
  }
}

void checkVias(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  const bool has_schematic = primarySchematic(project) != nullptr;
  std::set<std::string> ids;
  for (const Via& via : board.vias) {
    if (via.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_VIA_ID", "Via ID must not be empty", via.id));
    }
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
    } else if (has_schematic && !hasNet(project, via.net_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_VIA_NET", "Via references an unknown net", via.id));
    }
    const bool via_size_positive = isPositive(via.diameter) && isPositive(via.drill);
    const bool via_drill_fits = via.drill.nanometers <= via.diameter.nanometers;
    if (!via_size_positive) {
      diagnostics.push_back(makeDiagnostic("INVALID_VIA_SIZE",
                                           "Via diameter and drill must be positive", via.id));
    } else {
      const std::int64_t radius_nm = via.diameter.nanometers / 2;
      const Point min{.x = nanometers(via.position.x.nanometers - radius_nm),
                      .y = nanometers(via.position.y.nanometers - radius_nm)};
      const Point max{.x = nanometers(via.position.x.nanometers + radius_nm),
                      .y = nanometers(via.position.y.nanometers + radius_nm)};
      if (!containsPoint(board, min) || !containsPoint(board, max)) {
        diagnostics.push_back(makeDiagnostic(
            "VIA_GEOMETRY_OUTSIDE_BOARD", "Via copper geometry extends outside board outline",
            via.id));
      }
    }
    if (!via_drill_fits) {
      diagnostics.push_back(makeDiagnostic("VIA_DRILL_TOO_LARGE",
                                           "Via drill must be less than or equal to diameter",
                                           via.id));
    }
    if (via_size_positive && board.design_rules.min_via_diameter.nanometers > 0 &&
        via.diameter.nanometers < board.design_rules.min_via_diameter.nanometers) {
      diagnostics.push_back(makeDiagnostic(
          "VIA_DIAMETER_BELOW_MINIMUM",
          "Via diameter is below configured minimum " +
              std::to_string(board.design_rules.min_via_diameter.nanometers) + " nm",
          via.id));
    }
    if (via_size_positive && board.design_rules.max_via_diameter.nanometers > 0 &&
        via.diameter.nanometers > board.design_rules.max_via_diameter.nanometers) {
      diagnostics.push_back(makeDiagnostic("VIA_DIAMETER_ABOVE_MAXIMUM",
          "Via diameter is above configured maximum " +
              std::to_string(board.design_rules.max_via_diameter.nanometers) + " nm", via.id));
    }
    if (via_size_positive && board.design_rules.min_through_hole_drill.nanometers > 0 &&
        via.drill.nanometers < board.design_rules.min_through_hole_drill.nanometers) {
      diagnostics.push_back(makeDiagnostic(
          "VIA_DRILL_BELOW_MINIMUM",
          "Via drill is below configured minimum " +
              std::to_string(board.design_rules.min_through_hole_drill.nanometers) + " nm",
          via.id));
    }
    const std::int64_t annular_ring = (via.diameter.nanometers - via.drill.nanometers) / 2;
    if (via_size_positive && via_drill_fits &&
        annular_ring < board.design_rules.min_via_annular_ring.nanometers) {
      diagnostics.push_back(
          makeDiagnostic("VIA_ANNULAR_RING_TOO_SMALL",
                         "Via annular ring is below configured minimum " +
                             std::to_string(board.design_rules.min_via_annular_ring.nanometers) +
                             " nm",
                         via.id));
    }
    if (via_size_positive) {
      for (const Keepout& keepout : board.keepouts) {
        const long double radius = static_cast<long double>(via.diameter.nanometers) / 2.0L;
        if (distancePointToRect(via.position, keepout.area) <= radius) {
          diagnostics.push_back(
              makeDiagnostic("VIA_IN_KEEPOUT", "Via geometry intersects keepout " + keepout.id,
                             via.id));
        }
      }
    }
  }
}

void checkHoleToHole(const Board& board, std::vector<Diagnostic>& diagnostics) {
  const auto holeRadius = [](const Length& diameter) {
    return static_cast<long double>(diameter.nanometers) / 2.0L;
  };
  for (std::size_t i = 0; i < board.vias.size(); ++i) {
    const Via& a = board.vias[i];
    if (!isPositive(a.drill) || !isPositive(a.diameter)) continue;
    for (std::size_t j = i + 1; j < board.vias.size(); ++j) {
      const Via& b = board.vias[j];
      if (!isPositive(b.drill) || !isPositive(b.diameter) || a.net_id == b.net_id) continue;
      const long double centre_distance = distanceBetweenPoints(a.position, b.position);
      const long double edge_clearance = centre_distance - holeRadius(a.drill) - holeRadius(b.drill);
      if (edge_clearance <= 0.0L) {
        diagnostics.push_back(makeDiagnostic("DRILLED_HOLES_COLOCATED",
            "Different-net drilled holes are colocated", b.id));
      } else if (edge_clearance < board.design_rules.min_hole_to_hole.nanometers) {
        diagnostics.push_back(makeDiagnostic("DRILLED_HOLES_TOO_CLOSE",
            "Different-net drilled holes are closer than configured clearance", b.id));
      }
    }
  }
}

void checkTracks(const Project& project, const Board& board, std::vector<Diagnostic>& diagnostics) {
  const bool has_schematic = primarySchematic(project) != nullptr;
  std::set<std::string> ids;
  for (const TrackSegment& track : board.tracks) {
    if (track.id.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_TRACK_ID", "Track ID must not be empty", track.id));
    }
    if (!ids.insert(track.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_TRACK_ID", "Track ID appears more than once", track.id));
    }
    if (!hasLayer(board, track.layer_id)) {
      diagnostics.push_back(
          makeDiagnostic("UNKNOWN_TRACK_LAYER", "Track references an unknown layer", track.id));
    } else if (!isCopperLayer(board, track.layer_id)) {
      diagnostics.push_back(makeDiagnostic("TRACK_NON_COPPER_LAYER",
                                           "Track must be on a copper layer", track.id));
    }
    if (track.net_id.empty()) {
      diagnostics.push_back(
          makeWarning("UNCONNECTED_TRACK", "Track has no assigned net", track.id));
    } else if (has_schematic && !hasNet(project, track.net_id)) {
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
    const bool track_width_positive = isPositive(track.width);
    if (!track_width_positive) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_TRACK_WIDTH", "Track width must be positive", track.id));
    } else {
      if (track.width.nanometers < board.design_rules.min_track_width.nanometers) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_TOO_NARROW",
            "Track width is below configured minimum " +
                std::to_string(board.design_rules.min_track_width.nanometers) + " nm",
            track.id));
      }
      if (board.design_rules.max_track_width.nanometers > 0 &&
          track.width.nanometers > board.design_rules.max_track_width.nanometers) {
        diagnostics.push_back(makeDiagnostic("TRACK_TOO_WIDE",
            "Track width is above configured maximum " +
                std::to_string(board.design_rules.max_track_width.nanometers) + " nm", track.id));
      }
      const long double half_width = static_cast<long double>(track.width.nanometers) / 2.0L;
      if (distanceToBoardEdge(board, track.start) < half_width ||
          distanceToBoardEdge(board, track.end) < half_width) {
        diagnostics.push_back(makeDiagnostic(
            "TRACK_GEOMETRY_OUTSIDE_BOARD",
            "Track copper geometry extends outside board outline", track.id));
      }
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
    if (track_width_positive) {
      for (const Keepout& keepout : board.keepouts) {
        const std::int64_t half_width_nm = track.width.nanometers / 2;
        const Rect inflated_keepout = inflateRect(keepout.area, half_width_nm);
        if (rectContainsPoint(inflated_keepout, track.start) ||
            rectContainsPoint(inflated_keepout, track.end)) {
          diagnostics.push_back(makeDiagnostic(
              "TRACK_ENDPOINT_IN_KEEPOUT", "Track endpoint is inside keepout " + keepout.id,
              track.id));
        } else if (segmentIntersectsRect(track.start, track.end, inflated_keepout)) {
          diagnostics.push_back(makeDiagnostic(
              "TRACK_CROSSES_KEEPOUT", "Track segment crosses keepout " + keepout.id, track.id));
        }
      }
    }
  }
}

void checkBoardGraphics(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const BoardGraphic& graphic : board.graphics) {
    if (graphic.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_BOARD_GRAPHIC_ID",
                                           "Board graphic ID must not be empty", graphic.id));
    }
    if (!ids.insert(graphic.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_BOARD_GRAPHIC_ID",
                                           "Board graphic ID appears more than once",
                                           graphic.id));
    }
    if (graphic.kind != "line") {
      diagnostics.push_back(makeDiagnostic("UNSUPPORTED_BOARD_GRAPHIC_KIND",
                                           "Board graphic kind is not supported yet",
                                           graphic.id));
    }
    if (!hasLayer(board, graphic.layer_id)) {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_BOARD_GRAPHIC_LAYER",
                                           "Board graphic references an unknown layer",
                                           graphic.id));
    }
    if (!isPositive(graphic.width)) {
      diagnostics.push_back(makeDiagnostic("INVALID_BOARD_GRAPHIC_WIDTH",
                                           "Board graphic width must be positive",
                                           graphic.id));
    }
    if (samePoint(graphic.start, graphic.end)) {
      diagnostics.push_back(makeDiagnostic("ZERO_LENGTH_BOARD_GRAPHIC",
                                           "Board graphic line start and end must be different",
                                           graphic.id));
    }
    if (!containsPoint(board, graphic.start) || !containsPoint(board, graphic.end)) {
      diagnostics.push_back(makeDiagnostic("BOARD_GRAPHIC_OUTSIDE_BOARD",
                                           "Board graphic endpoint is outside board outline",
                                           graphic.id));
    }
  }
}

void checkBoardTexts(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const BoardText& text : board.texts) {
    if (text.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_BOARD_TEXT_ID",
                                           "Board text ID must not be empty", text.id));
    }
    if (!ids.insert(text.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_BOARD_TEXT_ID",
                                           "Board text ID appears more than once",
                                           text.id));
    }
    if (!hasLayer(board, text.layer_id)) {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_BOARD_TEXT_LAYER",
                                           "Board text references an unknown layer", text.id));
    }
    if (text.text.empty()) {
      diagnostics.push_back(makeDiagnostic("EMPTY_BOARD_TEXT", "Board text must not be empty",
                                           text.id));
    }
    if (!isPositive(text.size.width) || !isPositive(text.size.height)) {
      diagnostics.push_back(makeDiagnostic("INVALID_BOARD_TEXT_SIZE",
                                           "Board text width and height must be positive",
                                           text.id));
    }
    if (board.design_rules.min_text_height.nanometers > 0 &&
        text.size.height.nanometers < board.design_rules.min_text_height.nanometers) {
      diagnostics.push_back(makeDiagnostic("TEXT_HEIGHT_BELOW_MINIMUM",
                                           "Board text height is below configured minimum",
                                           text.id));
    }
    if (board.design_rules.min_text_thickness.nanometers > 0 &&
        (text.stroke_width.nanometers <= 0 ||
         text.stroke_width.nanometers < board.design_rules.min_text_thickness.nanometers)) {
      diagnostics.push_back(makeDiagnostic("TEXT_THICKNESS_BELOW_MINIMUM",
                                           "Board text stroke width is below configured minimum",
                                           text.id));
    }
    if (!containsPoint(board, text.position)) {
      diagnostics.push_back(makeDiagnostic("BOARD_TEXT_OUTSIDE_BOARD",
                                           "Board text origin is outside board outline",
                                           text.id));
    }
    const bool back_layer = text.layer_id.starts_with("B.");
    if ((back_layer && !text.mirrored) || (!back_layer && text.mirrored)) {
      diagnostics.push_back(makeDiagnostic(
          back_layer ? "NONMIRRORED_TEXT_ON_BACK_LAYER" : "MIRRORED_TEXT_ON_FRONT_LAYER",
          back_layer ? "Back-layer text must be mirrored" : "Front-layer text must not be mirrored",
          text.id));
    }
  }
}

bool isSupportedZonePadConnection(const std::string& pad_connection) {
  return pad_connection == "thermal" || pad_connection == "solid" || pad_connection == "none";
}

void checkBoardZones(const Project& project, const Board& board,
                     std::vector<Diagnostic>& diagnostics) {
  const bool has_schematic = primarySchematic(project) != nullptr;
  std::set<std::string> ids;
  for (const BoardZone& zone : board.zones) {
    if (zone.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_ZONE_ID", "Zone ID must not be empty",
                                           zone.id));
    }
    if (!ids.insert(zone.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_ZONE_ID",
                                           "Zone ID appears more than once", zone.id));
    }
    if (zone.layer_ids.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_ZONE_LAYER_SET",
                                           "Zone must reference at least one copper layer",
                                           zone.id));
    }
    std::set<std::string> zone_layers;
    for (const std::string& layer_id : zone.layer_ids) {
      if (!zone_layers.insert(layer_id).second) {
        diagnostics.push_back(makeDiagnostic("DUPLICATE_ZONE_LAYER",
                                             "Zone layer appears more than once", zone.id));
      }
      if (!hasLayer(board, layer_id)) {
        diagnostics.push_back(makeDiagnostic("UNKNOWN_ZONE_LAYER",
                                             "Zone references an unknown layer", zone.id));
      } else if (!isCopperLayer(board, layer_id)) {
        diagnostics.push_back(makeDiagnostic("ZONE_NON_COPPER_LAYER",
                                             "Zone must be on copper layers", zone.id));
      }
    }
    if (has_schematic && !zone.net_id.empty() && !hasNet(project, zone.net_id)) {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_ZONE_NET",
                                           "Zone references an unknown net", zone.id));
    }
    if (zone.outline.size() < 3) {
      diagnostics.push_back(makeDiagnostic("INVALID_ZONE_OUTLINE",
                                           "Zone outline must contain at least three corners",
                                           zone.id));
    }
    for (const Point& point : zone.outline) {
      if (!containsPoint(board, point)) {
        diagnostics.push_back(makeDiagnostic("ZONE_OUTSIDE_BOARD",
                                             "Zone outline point is outside board outline",
                                             zone.id));
        break;
      }
    }
    if (!isPositive(zone.clearance)) {
      diagnostics.push_back(makeDiagnostic("INVALID_ZONE_CLEARANCE",
                                           "Zone clearance must be positive", zone.id));
    }
    if (!isPositive(zone.min_thickness)) {
      diagnostics.push_back(makeDiagnostic("INVALID_ZONE_MIN_THICKNESS",
                                           "Zone minimum thickness must be positive",
                                           zone.id));
    }
    if (!isSupportedZonePadConnection(zone.pad_connection)) {
      diagnostics.push_back(makeDiagnostic("INVALID_ZONE_PAD_CONNECTION",
                                           "Zone pad connection mode is unsupported",
                                           zone.id));
    }
  }
}

void checkRouteRequests(const Project& project, const Board& board,
                        std::vector<Diagnostic>& diagnostics) {
  const bool has_schematic = primarySchematic(project) != nullptr;
  std::set<std::string> ids;
  for (const RouteRequest& route_request : board.route_requests) {
    if (route_request.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_ROUTE_REQUEST_ID",
                                           "Route request ID must not be empty",
                                           route_request.id));
    }
    if (!ids.insert(route_request.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_ROUTE_REQUEST_ID",
                                           "Route request ID appears more than once",
                                           route_request.id));
    }
    if (route_request.net_id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_ROUTE_REQUEST_NET",
                                           "Route request net_id must not be empty",
                                           route_request.id));
    } else if (has_schematic && !hasNet(project, route_request.net_id)) {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_ROUTE_REQUEST_NET",
                                           "Route request references an unknown net",
                                           route_request.id));
    }
    if (route_request.from_object_id == route_request.to_object_id) {
      diagnostics.push_back(makeDiagnostic("ROUTE_REQUEST_SAME_ENDPOINT",
                                           "Route request endpoints must be different objects",
                                           route_request.id));
    }
    if (!hasLayer(board, route_request.preferred_layer_id)) {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_ROUTE_REQUEST_LAYER",
                                           "Route request references an unknown preferred layer",
                                           route_request.id));
    } else if (!isCopperLayer(board, route_request.preferred_layer_id)) {
      diagnostics.push_back(makeDiagnostic("ROUTE_REQUEST_NON_COPPER_LAYER",
                                           "Route request preferred layer must be copper",
                                           route_request.id));
    }
    if (!hasBoardObject(board, route_request.from_object_id) ||
        !hasBoardObject(board, route_request.to_object_id)) {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_ROUTE_REQUEST_ENDPOINT",
                                           "Route request endpoint object does not exist",
                                           route_request.id));
    } else if (!route_request.net_id.empty()) {
      const std::string from_net = boardObjectNetId(board, route_request.from_object_id);
      const std::string to_net = boardObjectNetId(board, route_request.to_object_id);
      if ((!from_net.empty() && from_net != route_request.net_id) ||
          (!to_net.empty() && to_net != route_request.net_id)) {
        diagnostics.push_back(makeDiagnostic("ROUTE_REQUEST_ENDPOINT_NET_MISMATCH",
                                             "Route request endpoint net does not match request net",
                                             route_request.id));
      }
    }
    if (!isPositive(route_request.width)) {
      diagnostics.push_back(makeDiagnostic("INVALID_ROUTE_REQUEST_WIDTH",
                                           "Route request width must be positive",
                                           route_request.id));
    } else if (route_request.width.nanometers < board.design_rules.min_track_width.nanometers) {
      diagnostics.push_back(makeDiagnostic(
          "ROUTE_REQUEST_WIDTH_TOO_NARROW",
          "Route request width is below configured minimum " +
              std::to_string(board.design_rules.min_track_width.nanometers) + " nm",
          route_request.id));
    }
    if (route_request.policy.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_ROUTE_REQUEST_POLICY",
                                           "Route request policy must not be empty",
                                           route_request.id));
    }
  }
}

void checkProjectNets(const Project& project, std::vector<Diagnostic>& diagnostics) {
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return;
  }
  std::set<std::string> ids;
  for (const Net& net : schematic->nets) {
    if (net.id.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_NET_ID", "Net ID must not be empty", net.id));
      continue;
    }
    if (!ids.insert(net.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_NET_ID", "Net ID appears more than once", net.id));
    }

    std::set<std::pair<std::string, std::string>> members;
    for (const NetMember& member : net.members) {
      if (member.component_id.empty() || member.pin_name.empty()) {
        diagnostics.push_back(makeDiagnostic(
            "INVALID_NET_MEMBER", "Net member must include component_id and pin_name", net.id));
        continue;
      }
      const std::pair<std::string, std::string> member_key{member.component_id, member.pin_name};
      if (!members.insert(member_key).second) {
        diagnostics.push_back(makeDiagnostic(
            "DUPLICATE_NET_MEMBER", "Net contains duplicate member component/pin", net.id));
      }
    }
  }
}

void checkBoardOutline(const Board& board, std::vector<Diagnostic>& diagnostics) {
  if (!isPositive(board.outline.size.width) || !isPositive(board.outline.size.height)) {
    diagnostics.push_back(makeDiagnostic("INVALID_BOARD_OUTLINE",
                                         "Board outline width and height must be positive",
                                         "board"));
  }
}

void checkDesignRules(const Board& board, std::vector<Diagnostic>& diagnostics) {
  for (const auto& error : validateDesignRules(board.design_rules)) {
    diagnostics.push_back(makeDiagnostic(error.code, error.message, "board.design_rules"));
  }
}

void checkLayers(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Layer& layer : board.layers) {
    if (layer.id.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_LAYER_ID", "Layer ID must not be empty", layer.id));
    }
    if (layer.name.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_LAYER_NAME", "Layer name must not be empty", layer.id));
    }
    if (layer.kind.empty()) {
      diagnostics.push_back(
          makeDiagnostic("INVALID_LAYER_KIND", "Layer kind must not be empty", layer.id));
    }
    if (!ids.insert(layer.id).second) {
      diagnostics.push_back(
          makeDiagnostic("DUPLICATE_LAYER_ID", "Layer ID appears more than once", layer.id));
    }
  }
}

void checkKeepouts(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Keepout& keepout : board.keepouts) {
    if (keepout.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_KEEPOUT_ID",
                                           "Keepout ID must not be empty", keepout.id));
    }
    if (!ids.insert(keepout.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_KEEPOUT_ID",
                                           "Keepout ID appears more than once", keepout.id));
    }

    if (!isPositive(keepout.area.size.width) || !isPositive(keepout.area.size.height)) {
      diagnostics.push_back(makeDiagnostic("INVALID_KEEPOUT_SIZE",
                                           "Keepout width and height must be positive",
                                           keepout.id));
      continue;
    }

    if (keepout.kind != "placement" && keepout.kind != "routing") {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_KEEPOUT_KIND",
                                           "Keepout kind must be placement or routing",
                                           keepout.id));
    }

    const Point min = keepout.area.origin;
    const Point max = maxPoint(keepout.area);
    if (!containsPoint(board, min) || !containsPoint(board, max)) {
      diagnostics.push_back(makeDiagnostic("KEEPOUT_OUTSIDE_BOARD",
                                           "Keepout area is outside board outline", keepout.id));
    }
  }
}

void checkPlacementRegions(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const PlacementRegion& region : board.placement_regions) {
    if (region.id.empty()) {
      diagnostics.push_back(makeDiagnostic("INVALID_PLACEMENT_REGION_ID",
                                           "Placement region ID must not be empty", region.id));
    }
    if (!ids.insert(region.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PLACEMENT_REGION_ID",
                                           "Placement region ID appears more than once",
                                           region.id));
    }

    if (!isPositive(region.area.size.width) || !isPositive(region.area.size.height)) {
      diagnostics.push_back(makeDiagnostic("INVALID_PLACEMENT_REGION_SIZE",
                                           "Placement region width and height must be positive",
                                           region.id));
      continue;
    }

    if (region.kind != "component" && region.kind != "module") {
      diagnostics.push_back(makeDiagnostic("UNKNOWN_PLACEMENT_REGION_KIND",
                                           "Placement region kind must be component or module",
                                           region.id));
    }

    const Point min = region.area.origin;
    const Point max = maxPoint(region.area);
    if (!containsPoint(board, min) || !containsPoint(board, max)) {
      diagnostics.push_back(makeDiagnostic("PLACEMENT_REGION_OUTSIDE_BOARD",
                                           "Placement region area is outside board outline",
                                           region.id));
    }
  }
}

void checkPhysicalObjectIds(const Board& board, std::vector<Diagnostic>& diagnostics) {
  std::set<std::string> ids;
  for (const Pad& pad : board.pads) {
    if (!pad.id.empty() && !ids.insert(pad.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           pad.id));
    }
  }
  for (const Via& via : board.vias) {
    if (!via.id.empty() && !ids.insert(via.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           via.id));
    }
  }
  for (const TrackSegment& track : board.tracks) {
    if (!track.id.empty() && !ids.insert(track.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           track.id));
    }
  }
  for (const BoardGraphic& graphic : board.graphics) {
    if (!graphic.id.empty() && !ids.insert(graphic.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           graphic.id));
    }
  }
  for (const BoardText& text : board.texts) {
    if (!text.id.empty() && !ids.insert(text.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           text.id));
    }
  }
  for (const BoardZone& zone : board.zones) {
    if (!zone.id.empty() && !ids.insert(zone.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           zone.id));
    }
  }
  for (const Keepout& keepout : board.keepouts) {
    if (!keepout.id.empty() && !ids.insert(keepout.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           keepout.id));
    }
  }
  for (const PlacementRegion& region : board.placement_regions) {
    if (!region.id.empty() && !ids.insert(region.id).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_PHYSICAL_OBJECT_ID",
                                           "Physical object ID is reused across object types",
                                           region.id));
    }
  }
}

void checkCopperClearance(const Board& board, std::vector<Diagnostic>& diagnostics) {
  const long double copper_clearance =
      static_cast<long double>(board.design_rules.copper_clearance.nanometers);

  for (std::size_t left = 0; left < board.pads.size(); ++left) {
    for (std::size_t right = left + 1; right < board.pads.size(); ++right) {
      const Pad& left_pad = board.pads.at(left);
      const Pad& right_pad = board.pads.at(right);
      if (!hasValidPadGeometry(left_pad) || !hasValidPadGeometry(right_pad) ||
          sameNonEmptyNet(left_pad.net_id, right_pad.net_id) ||
          !differentNonEmptyNets(left_pad.net_id, right_pad.net_id) ||
          !padsShareCopperLayer(board, left_pad, right_pad)) {
        continue;
      }
      if (distanceBetweenPolygons(padCorners(left_pad), padCorners(right_pad)) <
          copper_clearance) {
        addClearanceDiagnostic(diagnostics, right_pad.id, left_pad.id,
                               board.design_rules.copper_clearance);
      }
    }
  }

  for (std::size_t left = 0; left < board.tracks.size(); ++left) {
    for (std::size_t right = left + 1; right < board.tracks.size(); ++right) {
      const TrackSegment& left_track = board.tracks.at(left);
      const TrackSegment& right_track = board.tracks.at(right);
      if (!hasValidTrackGeometry(left_track) || !hasValidTrackGeometry(right_track) ||
          sameNonEmptyNet(left_track.net_id, right_track.net_id) ||
          !differentNonEmptyNets(left_track.net_id, right_track.net_id) ||
          !shareCopperLayer(board, left_track.layer_id, right_track.layer_id)) {
        continue;
      }
      const long double edge_distance =
          distanceBetweenSegments(left_track.start, left_track.end, right_track.start,
                                  right_track.end) -
          (static_cast<long double>(left_track.width.nanometers) / 2.0L) -
          (static_cast<long double>(right_track.width.nanometers) / 2.0L);
      if (edge_distance < copper_clearance) {
        addClearanceDiagnostic(diagnostics, right_track.id, left_track.id,
                               board.design_rules.copper_clearance);
      }
    }
  }

  for (const Pad& pad : board.pads) {
    for (const TrackSegment& track : board.tracks) {
      if (!hasValidPadGeometry(pad) || !hasValidTrackGeometry(track) ||
          sameNonEmptyNet(pad.net_id, track.net_id) ||
          !differentNonEmptyNets(pad.net_id, track.net_id) ||
          !padSharesCopperLayer(board, pad, track.layer_id)) {
        continue;
      }
      const long double edge_distance =
          distanceSegmentToPolygon(track.start, track.end, padCorners(pad)) -
          (static_cast<long double>(track.width.nanometers) / 2.0L);
      if (edge_distance < copper_clearance) {
        addClearanceDiagnostic(diagnostics, track.id, pad.id, board.design_rules.copper_clearance);
      }
    }
  }

  for (std::size_t left = 0; left < board.vias.size(); ++left) {
    for (std::size_t right = left + 1; right < board.vias.size(); ++right) {
      const Via& left_via = board.vias.at(left);
      const Via& right_via = board.vias.at(right);
      if (!hasValidViaGeometry(left_via) || !hasValidViaGeometry(right_via) ||
          sameNonEmptyNet(left_via.net_id, right_via.net_id) ||
          !differentNonEmptyNets(left_via.net_id, right_via.net_id)) {
        continue;
      }
      const long double edge_distance =
          distanceBetweenPoints(left_via.position, right_via.position) -
          (static_cast<long double>(left_via.diameter.nanometers) / 2.0L) -
          (static_cast<long double>(right_via.diameter.nanometers) / 2.0L);
      if (edge_distance < copper_clearance) {
        addClearanceDiagnostic(diagnostics, right_via.id, left_via.id,
                               board.design_rules.copper_clearance);
      }
    }
  }

  for (const Via& via : board.vias) {
    for (const Pad& pad : board.pads) {
      if (!hasValidViaGeometry(via) || !hasValidPadGeometry(pad) ||
          sameNonEmptyNet(via.net_id, pad.net_id) ||
          !differentNonEmptyNets(via.net_id, pad.net_id) || !padOnCopperLayer(board, pad)) {
        continue;
      }
      const long double edge_distance =
          distancePointToPolygon(via.position, padCorners(pad)) -
          (static_cast<long double>(via.diameter.nanometers) / 2.0L);
      if (edge_distance < copper_clearance) {
        addClearanceDiagnostic(diagnostics, via.id, pad.id, board.design_rules.copper_clearance);
      }
    }
    for (const TrackSegment& track : board.tracks) {
      if (!hasValidViaGeometry(via) || !hasValidTrackGeometry(track) ||
          sameNonEmptyNet(via.net_id, track.net_id) ||
          !differentNonEmptyNets(via.net_id, track.net_id) ||
          !isCopperLayer(board, track.layer_id)) {
        continue;
      }
      const long double edge_distance =
          drcDistancePointToSegment(via.position, track.start, track.end) -
          (static_cast<long double>(via.diameter.nanometers) / 2.0L) -
          (static_cast<long double>(track.width.nanometers) / 2.0L);
      if (edge_distance < copper_clearance) {
        addClearanceDiagnostic(diagnostics, via.id, track.id,
                               board.design_rules.copper_clearance);
      }
    }
  }
}

void checkTrackAngles(const Board& board, std::vector<Diagnostic>& diagnostics) {
  const double minimum = board.design_rules.min_track_angle_degrees;
  const double maximum = board.design_rules.max_track_angle_degrees;
  if (minimum <= 0.0 && maximum <= 0.0) return;
  for (std::size_t left = 0; left < board.tracks.size(); ++left) {
    for (std::size_t right = left + 1; right < board.tracks.size(); ++right) {
      const TrackSegment& first = board.tracks.at(left);
      const TrackSegment& second = board.tracks.at(right);
      if (!hasValidTrackGeometry(first) || !hasValidTrackGeometry(second) ||
          first.net_id.empty() || first.net_id != second.net_id ||
          first.layer_id != second.layer_id) continue;
      Point vertex{};
      Point first_other{};
      Point second_other{};
      if (samePoint(first.start, second.start)) {
        vertex = first.start; first_other = first.end; second_other = second.end;
      } else if (samePoint(first.start, second.end)) {
        vertex = first.start; first_other = first.end; second_other = second.start;
      } else if (samePoint(first.end, second.start)) {
        vertex = first.end; first_other = first.start; second_other = second.end;
      } else if (samePoint(first.end, second.end)) {
        vertex = first.end; first_other = first.start; second_other = second.start;
      } else continue;
      const long double ax = static_cast<long double>(first_other.x.nanometers - vertex.x.nanometers);
      const long double ay = static_cast<long double>(first_other.y.nanometers - vertex.y.nanometers);
      const long double bx = static_cast<long double>(second_other.x.nanometers - vertex.x.nanometers);
      const long double by = static_cast<long double>(second_other.y.nanometers - vertex.y.nanometers);
      const long double lengths = std::hypotl(ax, ay) * std::hypotl(bx, by);
      if (lengths <= 0.0L) continue;
      const long double cosine = std::clamp((ax * bx + ay * by) / lengths, -1.0L, 1.0L);
      const double angle = static_cast<double>(std::acos(cosine) * 180.0L / std::acos(-1.0L));
      if ((minimum > 0.0 && angle < minimum) || (maximum > 0.0 && angle > maximum))
        diagnostics.push_back(makeDiagnostic("TRACK_ANGLE", "Connected track angle violates configured range", second.id));
    }
  }
}

void checkTrackSegmentLengths(const Board& board, std::vector<Diagnostic>& diagnostics) {
  const auto minimum = board.design_rules.min_track_segment_length.nanometers;
  const auto maximum = board.design_rules.max_track_segment_length.nanometers;
  if (minimum <= 0 && maximum <= 0) return;
  for (const TrackSegment& track : board.tracks) {
    if (!hasValidTrackGeometry(track)) continue;
    const auto length = static_cast<std::int64_t>(std::llround(
        std::hypotl(static_cast<long double>(track.end.x.nanometers - track.start.x.nanometers),
                    static_cast<long double>(track.end.y.nanometers - track.start.y.nanometers))));
    if ((minimum > 0 && length < minimum) || (maximum > 0 && length > maximum))
      diagnostics.push_back(makeDiagnostic("TRACK_SEGMENT_LENGTH",
                                           "Track segment length violates configured range",
                                           track.id));
  }
  for (const TrackArc& arc : board.track_arcs) {
    if (arc.id.empty()) continue;
    const long double ax = static_cast<long double>(arc.start.x.nanometers);
    const long double ay = static_cast<long double>(arc.start.y.nanometers);
    const long double bx = static_cast<long double>(arc.mid.x.nanometers);
    const long double by = static_cast<long double>(arc.mid.y.nanometers);
    const long double cx = static_cast<long double>(arc.end.x.nanometers);
    const long double cy = static_cast<long double>(arc.end.y.nanometers);
    const long double cross = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    const long double denom = 2.0L * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    long double length = 0.0L;
    if (std::abs(cross) < 1e-9L || std::abs(denom) < 1e-9L) {
      length = std::hypotl(bx - ax, by - ay) + std::hypotl(cx - bx, cy - by);
    } else {
      const long double center_x = ((ax * ax + ay * ay) * (by - cy) +
                                    (bx * bx + by * by) * (cy - ay) +
                                    (cx * cx + cy * cy) * (ay - by)) / denom;
      const long double center_y = ((ax * ax + ay * ay) * (cx - bx) +
                                    (bx * bx + by * by) * (ax - cx) +
                                    (cx * cx + cy * cy) * (bx - ax)) / denom;
      const long double radius = std::hypotl(ax - center_x, ay - center_y);
      const long double start_angle = std::atan2(ay - center_y, ax - center_x);
      const long double mid_angle = std::atan2(by - center_y, bx - center_x);
      const long double end_angle = std::atan2(cy - center_y, cx - center_x);
      const long double direction = cross > 0.0L ? 1.0L : -1.0L;
      long double sweep = direction > 0.0L ? end_angle - start_angle : start_angle - end_angle;
      long double mid_sweep = direction > 0.0L ? mid_angle - start_angle : start_angle - mid_angle;
      constexpr long double two_pi = 6.283185307179586476925286766559L;
      while (sweep < 0.0L) sweep += two_pi;
      while (mid_sweep < 0.0L) mid_sweep += two_pi;
      if (mid_sweep > sweep) sweep += two_pi;
      length = radius * sweep;
    }
    const auto length_nm = static_cast<std::int64_t>(std::llround(length));
    if ((minimum > 0 && length_nm < minimum) || (maximum > 0 && length_nm > maximum))
      diagnostics.push_back(makeDiagnostic("TRACK_SEGMENT_LENGTH",
                                           "Track arc length violates configured range", arc.id));
  }
}

void checkSchematicFootprintParity(const Project& project, const Board& board,
                                   std::vector<Diagnostic>& diagnostics) {
  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr || board.footprints.empty()) return;
  std::set<std::string> board_refs;
  for (const BoardFootprint& footprint : board.footprints) {
    if (footprint.reference.empty()) continue;
    if (!board_refs.insert(footprint.reference).second) {
      diagnostics.push_back(makeDiagnostic("DUPLICATE_FOOTPRINT",
                                           "Board contains duplicate footprint reference",
                                           footprint.reference));
    }
  }
  // Imported/hand-authored boards may carry pads before optional footprint metadata.
  // Their component_id still proves physical footprint presence for parity purposes.
  for (const Pad& pad : board.pads) {
    if (!pad.component_id.empty()) board_refs.insert(pad.component_id);
  }
  std::set<std::string> schematic_refs;
  for (const SchSymbol& symbol : schematic->symbols) {
    const std::string reference = symbol.reference.empty() ? symbol.id : symbol.reference;
    if (reference.empty() || reference.ends_with("?")) continue;
    schematic_refs.insert(reference);
    if (symbol.on_board && !board_refs.contains(reference)) {
      diagnostics.push_back(makeDiagnostic("MISSING_FOOTPRINT",
                                           "Schematic component has no board footprint",
                                           reference));
    } else {
      const auto footprint = std::find_if(board.footprints.begin(), board.footprints.end(),
                                          [&reference](const BoardFootprint& candidate) {
                                            return candidate.reference == reference;
                                          });
      if (footprint != board.footprints.end() && symbol.in_bom == footprint->exclude_from_bom) {
        diagnostics.push_back(makeDiagnostic("FOOTPRINT_BOM_PARITY",
                                             "Schematic BOM inclusion disagrees with board footprint exclusion",
                                             reference));
      }
      if (footprint != board.footprints.end() && symbol.on_board) {
        for (const SchPin& pin : symbol.pins) {
          const std::string pin_key = pin.number.empty() ? pin.name : pin.number;
          if (pin_key.empty()) continue;
          const bool has_pad = std::any_of(board.pads.begin(), board.pads.end(),
                                           [&reference, &pin_key](const Pad& pad) {
                                             return pad.component_id == reference &&
                                                    pad.pin_name == pin_key;
                                           });
          if (!has_pad) {
            diagnostics.push_back(makeDiagnostic("MISSING_PAD",
                                                 "Board footprint has no pad for schematic pin",
                                                 reference + "." + pin_key));
          }
        }
      }
    }
  }
  for (const BoardFootprint& footprint : board.footprints) {
    if (!footprint.reference.empty() && !schematic_refs.contains(footprint.reference)) {
      diagnostics.push_back(makeDiagnostic("EXTRA_FOOTPRINT",
                                           "Board footprint has no schematic component",
                                           footprint.reference));
    }
  }
}

void checkSilkClearance(const Board& board, std::vector<Diagnostic>& diagnostics) {
  const long double clearance = static_cast<long double>(board.design_rules.silk_clearance.nanometers);
  if (clearance <= 0) return;
  for (const BoardText& text : board.texts) {
    if (text.layer_id != "F.SilkS" && text.layer_id != "B.SilkS") continue;
    const Rect text_area{.origin = {.x = nanometers(text.position.x.nanometers - text.size.width.nanometers / 2),
                                    .y = nanometers(text.position.y.nanometers - text.size.height.nanometers / 2)},
                         .size = text.size};
    for (const Point& corner : rectCorners(text_area)) {
      if (distanceToBoardEdge(board, corner) < clearance) {
        diagnostics.push_back(makeDiagnostic("SILK_CLEARANCE",
                                             "Silkscreen text is too close to board edge",
                                             text.id));
        break;
      }
    }
    for (const Pad& pad : board.pads) {
      if (!hasValidPadGeometry(pad) || !padOnCopperLayer(board, pad)) continue;
      if (distanceBetweenPolygons(rectCorners(text_area), padCorners(pad)) < clearance) {
        diagnostics.push_back(makeDiagnostic("SILK_CLEARANCE",
                                             "Silkscreen text is too close to copper pad",
                                             text.id));
      }
    }
    for (const Via& via : board.vias) {
      if (!hasValidViaGeometry(via)) continue;
      if (distanceBetweenPoints(via.position, text.position) -
              static_cast<long double>(via.diameter.nanometers) / 2.0L < clearance) {
        diagnostics.push_back(makeDiagnostic("SILK_CLEARANCE",
                                             "Silkscreen text is too close to copper via",
                                             text.id));
      }
    }
    for (const TrackSegment& track : board.tracks) {
      if (!hasValidTrackGeometry(track) || !isCopperLayer(board, track.layer_id)) continue;
      const long double edge_distance =
          distanceSegmentToPolygon(track.start, track.end, rectCorners(text_area)) -
          static_cast<long double>(track.width.nanometers) / 2.0L;
      if (edge_distance < clearance) {
        diagnostics.push_back(makeDiagnostic("SILK_CLEARANCE",
                                             "Silkscreen text is too close to copper track",
                                             text.id));
      }
    }
    for (const BoardZone& zone : board.zones) {
      if (zone.outline.size() < 3) continue;
      const std::vector<Point> text_polygon = rectCorners(text_area);
      bool inside = pointInPolygon(text.position, zone.outline);
      for (const Point& corner : text_polygon) inside = inside || pointInPolygon(corner, zone.outline);
      if (inside || distanceBetweenPolygons(text_polygon, zone.outline) < clearance) {
        diagnostics.push_back(makeDiagnostic("SILK_CLEARANCE",
                                             "Silkscreen text is too close to copper zone",
                                             text.id));
      }
    }
  }
}

void checkSolderMaskBridges(const Board& board, std::vector<Diagnostic>& diagnostics) {
  const long double min_web = static_cast<long double>(board.design_rules.solder_mask_min_width.nanometers);
  if (min_web <= 0) return;
  auto maskCorners = [&board](const Pad& pad) {
    std::vector<Point> corners = padCorners(pad);
    const long double expansion = static_cast<long double>(board.design_rules.solder_mask_expansion.nanometers);
    long double min_x = static_cast<long double>(corners.front().x.nanometers), max_x = min_x;
    long double min_y = static_cast<long double>(corners.front().y.nanometers), max_y = min_y;
    for (const Point& point : corners) {
      min_x = std::min(min_x, static_cast<long double>(point.x.nanometers));
      max_x = std::max(max_x, static_cast<long double>(point.x.nanometers));
      min_y = std::min(min_y, static_cast<long double>(point.y.nanometers));
      max_y = std::max(max_y, static_cast<long double>(point.y.nanometers));
    }
    return rectCorners(Rect{.origin = {.x = nanometers(static_cast<std::int64_t>(min_x - expansion)),
                                       .y = nanometers(static_cast<std::int64_t>(min_y - expansion))},
                            .size = {.width = nanometers(static_cast<std::int64_t>(max_x - min_x + 2 * expansion)),
                                     .height = nanometers(static_cast<std::int64_t>(max_y - min_y + 2 * expansion))}});
  };
  for (std::size_t i = 0; i < board.pads.size(); ++i) {
    for (std::size_t j = i + 1; j < board.pads.size(); ++j) {
      const Pad& left = board.pads.at(i); const Pad& right = board.pads.at(j);
      if (!hasValidPadGeometry(left) || !hasValidPadGeometry(right) ||
          !differentNonEmptyNets(left.net_id, right.net_id) ||
          !padsShareCopperLayer(board, left, right)) continue;
      if (distanceBetweenPolygons(maskCorners(left), maskCorners(right)) < min_web) {
        diagnostics.push_back(makeDiagnostic("SOLDERMASK_BRIDGE",
                                             "Different-net solder mask apertures are too close",
                                             right.id));
      }
    }
  }
}

}  // namespace

std::vector<Diagnostic> runDrc(const Project& project) {
  std::vector<Diagnostic> diagnostics;
  if (project.boards.empty()) {
    return diagnostics;
  }

  checkProjectNets(project, diagnostics);
  const Board& board = project.boards[0];
  checkBoardOutline(board, diagnostics);
  checkDesignRules(board, diagnostics);
  checkLayers(board, diagnostics);
  checkPads(project, board, diagnostics);
  checkVias(project, board, diagnostics);
  checkHoleToHole(board, diagnostics);
  checkTracks(project, board, diagnostics);
  checkBoardGraphics(board, diagnostics);
  checkBoardTexts(board, diagnostics);
  checkSchematicFootprintParity(project, board, diagnostics);
  checkBoardZones(project, board, diagnostics);
  checkRouteRequests(project, board, diagnostics);
  checkPlacementRegions(board, diagnostics);
  checkKeepouts(board, diagnostics);
  checkPhysicalObjectIds(board, diagnostics);
  checkCopperClearance(board, diagnostics);
  checkTrackAngles(board, diagnostics);
  checkTrackSegmentLengths(board, diagnostics);
  checkSilkClearance(board, diagnostics);
  checkSolderMaskBridges(board, diagnostics);
  if (!board.teardrops.empty()) {
    diagnostics.push_back(makeWarning("TEARDROP_CLEARANCE_UNVERIFIED",
        "Teardrop copper clearance and fabrication export are not yet supported; "
        "this DRC result does not certify teardrop manufacturability.", board.teardrops.front().id));
  }
  return diagnostics;
}

}  // namespace ccad
