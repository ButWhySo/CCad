#include "ccad_core/board_outline_polygon.hpp"

#include <algorithm>
#include <limits>

namespace ccad {
namespace {

struct EdgeCutSegment {
  std::string id;
  Point start;
  Point end;
};

bool samePoint(const Point& left, const Point& right) {
  return left.x.nanometers == right.x.nanometers && left.y.nanometers == right.y.nanometers;
}

std::vector<std::string> pendingKiCadFeatures() {
  return {
      "arc_preserving_polygon_conversion",
      "circle_outline_conversion",
      "rectangle_outline_conversion",
      "poly_outline_conversion",
      "ellipse_outline_conversion",
      "hole_contour_nesting",
      "disjoint_outline_support",
      "self_intersection_reporting",
      "footprint_edge_cut_holes",
      "epsilon_endpoint_chaining",
  };
}

std::vector<Point> rectanglePoints(const Rect& rect) {
  const Point origin = rect.origin;
  const Point right_top{
      .x = nanometers(rect.origin.x.nanometers + rect.size.width.nanometers),
      .y = rect.origin.y,
  };
  const Point right_bottom{
      .x = nanometers(rect.origin.x.nanometers + rect.size.width.nanometers),
      .y = nanometers(rect.origin.y.nanometers + rect.size.height.nanometers),
  };
  const Point left_bottom{
      .x = rect.origin.x,
      .y = nanometers(rect.origin.y.nanometers + rect.size.height.nanometers),
  };
  return {origin, right_top, right_bottom, left_bottom};
}

Rect boundingBoxForPoints(const std::vector<Point>& points) {
  if (points.empty()) {
    return Rect{};
  }
  int64_t min_x = std::numeric_limits<int64_t>::max();
  int64_t min_y = std::numeric_limits<int64_t>::max();
  int64_t max_x = std::numeric_limits<int64_t>::min();
  int64_t max_y = std::numeric_limits<int64_t>::min();
  for (const Point& point : points) {
    min_x = std::min(min_x, point.x.nanometers);
    min_y = std::min(min_y, point.y.nanometers);
    max_x = std::max(max_x, point.x.nanometers);
    max_y = std::max(max_y, point.y.nanometers);
  }
  return Rect{
      .origin = Point{.x = nanometers(min_x), .y = nanometers(min_y)},
      .size = Size{.width = nanometers(max_x - min_x), .height = nanometers(max_y - min_y)},
  };
}

bool hasUsableBoardOutline(const Board& board) {
  return board.outline.size.width.nanometers > 0 && board.outline.size.height.nanometers > 0;
}

void inferFromBoardOutline(BoardOutlinePolygonReport& report, const Board& board) {
  report.used_inferred_outline = true;
  report.valid = true;
  report.outline_count = 1;
  report.bounding_box = board.outline;
  report.points = rectanglePoints(board.outline);
}

}  // namespace

BoardOutlinePolygonReport buildBoardOutlinePolygonReport(
    const Board& board,
    const BoardOutlinePolygonOptions& options) {
  BoardOutlinePolygonReport report;
  report.pending_kicad_features = pendingKiCadFeatures();

  std::vector<EdgeCutSegment> segments;
  for (const BoardGraphic& graphic : board.graphics) {
    if (graphic.layer_id == "Edge.Cuts" && graphic.kind == "line") {
      segments.push_back(EdgeCutSegment{
          .id = graphic.id,
          .start = graphic.start,
          .end = graphic.end,
      });
      report.source_graphic_ids.push_back(graphic.id);
    }
  }

  report.edge_cut_segment_count = segments.size();
  if (segments.empty()) {
    report.diagnostics.push_back("no Edge.Cuts line segments were available");
    if (options.infer_outline_if_necessary && hasUsableBoardOutline(board)) {
      inferFromBoardOutline(report, board);
    }
    return report;
  }

  std::vector<bool> used(segments.size(), false);
  report.points.push_back(segments.front().start);
  report.points.push_back(segments.front().end);
  used.front() = true;
  std::size_t used_count = 1;

  while (used_count < segments.size()) {
    const Point current = report.points.back();
    bool extended = false;
    for (std::size_t index = 0; index < segments.size(); ++index) {
      if (used.at(index)) {
        continue;
      }
      if (samePoint(segments.at(index).start, current)) {
        report.points.push_back(segments.at(index).end);
        used.at(index) = true;
        ++used_count;
        extended = true;
        break;
      }
      if (samePoint(segments.at(index).end, current)) {
        report.points.push_back(segments.at(index).start);
        used.at(index) = true;
        ++used_count;
        extended = true;
        break;
      }
    }
    if (!extended) {
      break;
    }
  }

  report.closed = report.points.size() >= 4 && samePoint(report.points.front(), report.points.back());
  const bool consumed_all_segments = used_count == segments.size();
  if (!consumed_all_segments) {
    report.diagnostics.push_back("Edge.Cuts line segments could not be chained into one contour");
  }
  if (!report.closed) {
    report.diagnostics.push_back("Edge.Cuts contour is not closed");
  }

  if (report.closed && consumed_all_segments) {
    report.points.pop_back();
    report.valid = true;
    report.outline_count = 1;
    report.hole_count = 0;
    report.bounding_box = boundingBoxForPoints(report.points);
    return report;
  }

  if (options.infer_outline_if_necessary && hasUsableBoardOutline(board)) {
    inferFromBoardOutline(report, board);
    return report;
  }

  report.bounding_box = boundingBoxForPoints(report.points);
  return report;
}

}  // namespace ccad
