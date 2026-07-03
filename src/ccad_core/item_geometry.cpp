#define _USE_MATH_DEFINES
#include "ccad_core/item_geometry.hpp"

#include <cmath>
#include <algorithm>
#include <limits>

namespace ccad {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Rotate a point around the origin by angleDeg degrees
static Point rotatePoint(Point p, double angleDeg) {
  if (std::abs(angleDeg) < 1e-9) return p;
  double rad = angleDeg * M_PI / 180.0;
  double cosA = std::cos(rad);
  double sinA = std::sin(rad);
  double px = static_cast<double>(p.x.nanometers);
  double py = static_cast<double>(p.y.nanometers);
  return Point{
      .x = nanometers(static_cast<int64_t>(std::round(px * cosA - py * sinA))),
      .y = nanometers(static_cast<int64_t>(std::round(px * sinA + py * cosA)))};
}

// Get the primary pad shape size in nanometers from padstack
static std::pair<int64_t, int64_t> getPadShapeSize(const Pad& pad) {
  // Look for front copper layer props first, then fall back to any available
  for (const auto& [layer, props] : pad.padstack.copper_props) {
    return {props.shape.size.width.nanometers, props.shape.size.height.nanometers};
  }
  // Fallback: use padstack drill as a proxy if no copper props
  return {pad.padstack.drill.size.width.nanometers,
          pad.padstack.drill.size.height.nanometers};
}

static PadShape getPadShape(const Pad& pad) {
  for (const auto& [layer, props] : pad.padstack.copper_props) {
    return props.shape.shape;
  }
  return PadShape::Circle;
}

// ---------------------------------------------------------------------------
// Bounding Box: Pad
// Mapped from KiCad PAD::GetBoundingBox
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const Pad& pad) {
  auto [w, h] = getPadShapeSize(pad);
  if (w <= 0 && h <= 0) {
    // Use drill size as fallback
    w = pad.padstack.drill.size.width.nanometers;
    h = pad.padstack.drill.size.height.nanometers;
  }
  if (w <= 0 && h <= 0) {
    return BoundingBox{};
  }

  double halfW = w / 2.0;
  double halfH = h / 2.0;

  // For rotated pads, compute the axis-aligned bounding box of the rotated shape
  double rot = pad.rotation_degrees;
  double rad = rot * M_PI / 180.0;
  double cosA = std::abs(std::cos(rad));
  double sinA = std::abs(std::sin(rad));

  double bbHalfW = halfW * cosA + halfH * sinA;
  double bbHalfH = halfW * sinA + halfH * cosA;

  int64_t cx = pad.position.x.nanometers;
  int64_t cy = pad.position.y.nanometers;

  BoundingBox bb;
  bb.min.x = nanometers(static_cast<int64_t>(cx - bbHalfW));
  bb.min.y = nanometers(static_cast<int64_t>(cy - bbHalfH));
  bb.max.x = nanometers(static_cast<int64_t>(cx + bbHalfW));
  bb.max.y = nanometers(static_cast<int64_t>(cy + bbHalfH));
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: Via
// Mapped from KiCad PCB_VIA::GetBoundingBox
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const Via& via) {
  int64_t r = via.diameter.nanometers / 2;
  BoundingBox bb;
  bb.min.x = nanometers(via.position.x.nanometers - r);
  bb.min.y = nanometers(via.position.y.nanometers - r);
  bb.max.x = nanometers(via.position.x.nanometers + r);
  bb.max.y = nanometers(via.position.y.nanometers + r);
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: TrackSegment
// Mapped from KiCad PCB_TRACK::GetBoundingBox
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const TrackSegment& track) {
  int64_t halfW = track.width.nanometers / 2;
  BoundingBox bb;
  bb.min.x = nanometers(std::min(track.start.x.nanometers, track.end.x.nanometers) - halfW);
  bb.min.y = nanometers(std::min(track.start.y.nanometers, track.end.y.nanometers) - halfW);
  bb.max.x = nanometers(std::max(track.start.x.nanometers, track.end.x.nanometers) + halfW);
  bb.max.y = nanometers(std::max(track.start.y.nanometers, track.end.y.nanometers) + halfW);
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: TrackArc
// Mapped from KiCad PCB_ARC::GetBoundingBox
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const TrackArc& arc) {
  int64_t halfW = arc.width.nanometers / 2;

  // Conservative bounding box: envelope of start, mid, end points + half width
  int64_t minX = std::min({arc.start.x.nanometers, arc.mid.x.nanometers, arc.end.x.nanometers});
  int64_t minY = std::min({arc.start.y.nanometers, arc.mid.y.nanometers, arc.end.y.nanometers});
  int64_t maxX = std::max({arc.start.x.nanometers, arc.mid.x.nanometers, arc.end.x.nanometers});
  int64_t maxY = std::max({arc.start.y.nanometers, arc.mid.y.nanometers, arc.end.y.nanometers});

  // Compute arc center from 3 points to get tighter bounds
  double ax = static_cast<double>(arc.start.x.nanometers);
  double ay = static_cast<double>(arc.start.y.nanometers);
  double bx = static_cast<double>(arc.mid.x.nanometers);
  double by = static_cast<double>(arc.mid.y.nanometers);
  double cx = static_cast<double>(arc.end.x.nanometers);
  double cy = static_cast<double>(arc.end.y.nanometers);

  double D = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
  if (std::abs(D) > 1e-6) {
    double aSq = ax * ax + ay * ay;
    double bSq = bx * bx + by * by;
    double cSq = cx * cx + cy * cy;
    double centerX = (aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / D;
    double centerY = (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / D;
    double radius = std::sqrt((ax - centerX) * (ax - centerX) + (ay - centerY) * (ay - centerY));

    // Use full circle envelope as conservative bound
    int64_t rNm = static_cast<int64_t>(std::ceil(radius));
    int64_t cxNm = static_cast<int64_t>(std::round(centerX));
    int64_t cyNm = static_cast<int64_t>(std::round(centerY));
    minX = std::min(minX, cxNm - rNm);
    minY = std::min(minY, cyNm - rNm);
    maxX = std::max(maxX, cxNm + rNm);
    maxY = std::max(maxY, cyNm + rNm);
  }

  BoundingBox bb;
  bb.min.x = nanometers(minX - halfW);
  bb.min.y = nanometers(minY - halfW);
  bb.max.x = nanometers(maxX + halfW);
  bb.max.y = nanometers(maxY + halfW);
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: BoardGraphic
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const BoardGraphic& graphic) {
  int64_t halfW = graphic.width.nanometers / 2;

  if (graphic.kind == "circle") {
    // start is center, end is point on circumference
    double radius = distancePoints(graphic.start, graphic.end);
    int64_t rNm = static_cast<int64_t>(std::ceil(radius));
    BoundingBox bb;
    bb.min.x = nanometers(graphic.start.x.nanometers - rNm - halfW);
    bb.min.y = nanometers(graphic.start.y.nanometers - rNm - halfW);
    bb.max.x = nanometers(graphic.start.x.nanometers + rNm + halfW);
    bb.max.y = nanometers(graphic.start.y.nanometers + rNm + halfW);
    bb.valid = true;
    return bb;
  }

  // Line, rectangle, or polygon: use start/end envelope
  BoundingBox bb;
  bb.min.x = nanometers(std::min(graphic.start.x.nanometers, graphic.end.x.nanometers) - halfW);
  bb.min.y = nanometers(std::min(graphic.start.y.nanometers, graphic.end.y.nanometers) - halfW);
  bb.max.x = nanometers(std::max(graphic.start.x.nanometers, graphic.end.x.nanometers) + halfW);
  bb.max.y = nanometers(std::max(graphic.start.y.nanometers, graphic.end.y.nanometers) + halfW);
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: BoardText
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const BoardText& text) {
  BoundingBox bb;
  bb.min.x = text.position.x;
  bb.min.y = text.position.y;
  bb.max.x = nanometers(text.position.x.nanometers + text.size.width.nanometers);
  bb.max.y = nanometers(text.position.y.nanometers + text.size.height.nanometers);
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: BoardZone
// Mapped from KiCad ZONE::GetBoundingBox
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const BoardZone& zone) {
  if (zone.outline.empty()) return BoundingBox{};

  int64_t minX = std::numeric_limits<int64_t>::max();
  int64_t minY = std::numeric_limits<int64_t>::max();
  int64_t maxX = std::numeric_limits<int64_t>::min();
  int64_t maxY = std::numeric_limits<int64_t>::min();

  for (const Point& pt : zone.outline) {
    minX = std::min(minX, pt.x.nanometers);
    minY = std::min(minY, pt.y.nanometers);
    maxX = std::max(maxX, pt.x.nanometers);
    maxY = std::max(maxY, pt.y.nanometers);
  }

  BoundingBox bb;
  bb.min.x = nanometers(minX);
  bb.min.y = nanometers(minY);
  bb.max.x = nanometers(maxX);
  bb.max.y = nanometers(maxY);
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: Keepout
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const Keepout& keepout) {
  return rectBoundingBox(keepout.area);
}

// ---------------------------------------------------------------------------
// Bounding Box: BoardDimension
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const BoardDimension& dim) {
  BoundingBox bb;
  bb.min.x = nanometers(std::min({dim.start.x.nanometers, dim.end.x.nanometers, dim.text_position.x.nanometers}));
  bb.min.y = nanometers(std::min({dim.start.y.nanometers, dim.end.y.nanometers, dim.text_position.y.nanometers}));
  bb.max.x = nanometers(std::max({dim.start.x.nanometers, dim.end.x.nanometers, dim.text_position.x.nanometers}));
  bb.max.y = nanometers(std::max({dim.start.y.nanometers, dim.end.y.nanometers, dim.text_position.y.nanometers}));
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: BoardBarcode
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const BoardBarcode& barcode) {
  int64_t halfW = barcode.size.width.nanometers / 2;
  int64_t halfH = barcode.size.height.nanometers / 2;

  double rot = barcode.rotation_degrees;
  double rad = rot * M_PI / 180.0;
  double cosA = std::abs(std::cos(rad));
  double sinA = std::abs(std::sin(rad));

  double bbHalfW = halfW * cosA + halfH * sinA;
  double bbHalfH = halfW * sinA + halfH * cosA;

  BoundingBox bb;
  bb.min.x = nanometers(static_cast<int64_t>(barcode.position.x.nanometers - bbHalfW));
  bb.min.y = nanometers(static_cast<int64_t>(barcode.position.y.nanometers - bbHalfH));
  bb.max.x = nanometers(static_cast<int64_t>(barcode.position.x.nanometers + bbHalfW));
  bb.max.y = nanometers(static_cast<int64_t>(barcode.position.y.nanometers + bbHalfH));
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Bounding Box: BoardTarget
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const BoardTarget& target) {
  int64_t halfSize = target.size.nanometers / 2;
  BoundingBox bb;
  bb.min.x = nanometers(target.position_x.nanometers - halfSize);
  bb.min.y = nanometers(target.position_y.nanometers - halfSize);
  bb.max.x = nanometers(target.position_x.nanometers + halfSize);
  bb.max.y = nanometers(target.position_y.nanometers + halfSize);
  bb.valid = true;
  return bb;
}

// ---------------------------------------------------------------------------
// Hit-Test: Pad
// Mapped from KiCad PAD::HitTest
// ---------------------------------------------------------------------------

bool itemHitTest(const Pad& pad, Point testPoint) {
  auto [w, h] = getPadShapeSize(pad);
  if (w <= 0 && h <= 0) return false;

  PadShape shape = getPadShape(pad);

  // Transform test point to pad-local coordinates (origin at pad center, rotation removed)
  Point local;
  local.x = nanometers(testPoint.x.nanometers - pad.position.x.nanometers);
  local.y = nanometers(testPoint.y.nanometers - pad.position.y.nanometers);

  if (std::abs(pad.rotation_degrees) > 1e-9) {
    local = rotatePoint(local, -pad.rotation_degrees);
  }

  double lx = static_cast<double>(local.x.nanometers);
  double ly = static_cast<double>(local.y.nanometers);
  double halfW = w / 2.0;
  double halfH = h / 2.0;

  switch (shape) {
    case PadShape::Circle: {
      double r = std::max(halfW, halfH);
      return (lx * lx + ly * ly) <= (r * r);
    }
    case PadShape::Oval: {
      // Oval: union of two semicircles and a rectangle
      if (halfW > halfH) {
        double capsuleHalfLen = halfW - halfH;
        if (std::abs(lx) <= capsuleHalfLen && std::abs(ly) <= halfH) return true;
        double dx1 = lx - capsuleHalfLen;
        double dx2 = lx + capsuleHalfLen;
        if ((dx1 * dx1 + ly * ly) <= halfH * halfH) return true;
        if ((dx2 * dx2 + ly * ly) <= halfH * halfH) return true;
        return false;
      } else {
        double capsuleHalfLen = halfH - halfW;
        if (std::abs(ly) <= capsuleHalfLen && std::abs(lx) <= halfW) return true;
        double dy1 = ly - capsuleHalfLen;
        double dy2 = ly + capsuleHalfLen;
        if ((lx * lx + dy1 * dy1) <= halfW * halfW) return true;
        if ((lx * lx + dy2 * dy2) <= halfW * halfW) return true;
        return false;
      }
    }
    case PadShape::Rectangle:
    case PadShape::RoundRect:
    case PadShape::ChamferedRect:
    case PadShape::Trapezoid:
    default:
      // Conservative rectangular hit-test
      return std::abs(lx) <= halfW && std::abs(ly) <= halfH;
  }
}

// ---------------------------------------------------------------------------
// Hit-Test: Via
// Mapped from KiCad PCB_VIA::HitTest
// ---------------------------------------------------------------------------

bool itemHitTest(const Via& via, Point testPoint) {
  double dx = static_cast<double>(testPoint.x.nanometers - via.position.x.nanometers);
  double dy = static_cast<double>(testPoint.y.nanometers - via.position.y.nanometers);
  double r = via.diameter.nanometers / 2.0;
  return (dx * dx + dy * dy) <= (r * r);
}

// ---------------------------------------------------------------------------
// Hit-Test: TrackSegment
// Mapped from KiCad PCB_TRACK::HitTest
// ---------------------------------------------------------------------------

bool itemHitTest(const TrackSegment& track, Point testPoint, int64_t accuracy_nm) {
  double halfW = track.width.nanometers / 2.0;
  double threshold = halfW + static_cast<double>(accuracy_nm);
  double dist = distancePointToSegment(testPoint, track.start, track.end);
  return dist <= threshold;
}

// ---------------------------------------------------------------------------
// Hit-Test: TrackArc
// Conservative hit-test using bounding box + distance to center arc
// ---------------------------------------------------------------------------

bool itemHitTest(const TrackArc& arc, Point testPoint, int64_t accuracy_nm) {
  // Compute arc center from 3 points
  double ax = static_cast<double>(arc.start.x.nanometers);
  double ay = static_cast<double>(arc.start.y.nanometers);
  double bx = static_cast<double>(arc.mid.x.nanometers);
  double by = static_cast<double>(arc.mid.y.nanometers);
  double cx = static_cast<double>(arc.end.x.nanometers);
  double cy = static_cast<double>(arc.end.y.nanometers);

  double D = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
  if (std::abs(D) < 1e-6) {
    // Degenerate arc: fall back to segment hit-test
    TrackSegment fallback;
    fallback.start = arc.start;
    fallback.end = arc.end;
    fallback.width = arc.width;
    return itemHitTest(fallback, testPoint, accuracy_nm);
  }

  double aSq = ax * ax + ay * ay;
  double bSq = bx * bx + by * by;
  double cSq = cx * cx + cy * cy;
  double centerX = (aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / D;
  double centerY = (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / D;
  double radius = std::sqrt((ax - centerX) * (ax - centerX) + (ay - centerY) * (ay - centerY));

  double px = static_cast<double>(testPoint.x.nanometers);
  double py = static_cast<double>(testPoint.y.nanometers);
  double distToCenter = std::sqrt((px - centerX) * (px - centerX) + (py - centerY) * (py - centerY));

  double halfW = arc.width.nanometers / 2.0;
  double threshold = halfW + static_cast<double>(accuracy_nm);

  // Check if point is within the annular region of the arc
  if (std::abs(distToCenter - radius) > threshold) return false;

  // Check angular range: test if point angle is between start and end angles
  double startAngle = std::atan2(ay - centerY, ax - centerX);
  double midAngle = std::atan2(by - centerY, bx - centerX);
  double endAngle = std::atan2(cy - centerY, cx - centerX);
  double testAngle = std::atan2(py - centerY, px - centerX);

  // Normalize angles relative to start
  auto normalizeAngle = [](double angle, double ref) -> double {
    double d = angle - ref;
    while (d < -M_PI) d += 2.0 * M_PI;
    while (d > M_PI) d -= 2.0 * M_PI;
    return d;
  };

  double midNorm = normalizeAngle(midAngle, startAngle);
  double endNorm = normalizeAngle(endAngle, startAngle);
  double testNorm = normalizeAngle(testAngle, startAngle);

  // The arc goes from start through mid to end.
  // Use midNorm to determine the sweep direction.
  if (midNorm > 0 && endNorm > 0 && midNorm < endNorm) {
    // CCW arc: mid is between start and end in CCW direction
    return testNorm >= -0.01 && testNorm <= endNorm + 0.01;
  } else if (midNorm < 0 && endNorm < 0 && midNorm > endNorm) {
    // CW arc: mid is between start and end in CW direction
    return testNorm <= 0.01 && testNorm >= endNorm - 0.01;
  } else {
    // Arc wraps around: check that test is NOT in the excluded range
    if (endNorm > 0) {
      return testNorm >= -0.01 || testNorm <= endNorm + 0.01;
    } else {
      return testNorm <= 0.01 || testNorm >= endNorm - 0.01;
    }
  }
}

// ---------------------------------------------------------------------------
// Hit-Test: BoardZone
// Point-in-polygon using ray casting
// ---------------------------------------------------------------------------

bool itemHitTest(const BoardZone& zone, Point testPoint) {
  if (zone.outline.size() < 3) return false;

  double px = static_cast<double>(testPoint.x.nanometers);
  double py = static_cast<double>(testPoint.y.nanometers);
  bool inside = false;
  size_t n = zone.outline.size();
  for (size_t i = 0, j = n - 1; i < n; j = i++) {
    double yi = static_cast<double>(zone.outline[i].y.nanometers);
    double yj = static_cast<double>(zone.outline[j].y.nanometers);
    double xi = static_cast<double>(zone.outline[i].x.nanometers);
    double xj = static_cast<double>(zone.outline[j].x.nanometers);

    if (((yi > py) != (yj > py)) &&
        (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
      inside = !inside;
    }
  }
  return inside;
}

// ---------------------------------------------------------------------------
// Hit-Test: Keepout
// ---------------------------------------------------------------------------

bool itemHitTest(const Keepout& keepout, Point testPoint) {
  Point rectMin = keepout.area.origin;
  Point rectMax = maxPoint(keepout.area);
  return pointInsideRect(testPoint, rectMin, rectMax);
}

// ---------------------------------------------------------------------------
// Length: TrackSegment
// Mapped from KiCad PCB_TRACK::GetLength
// ---------------------------------------------------------------------------

double itemLength(const TrackSegment& track) {
  double dist = distancePoints(track.start, track.end);
  return dist / 1000000.0; // nanometers to millimeters
}

// ---------------------------------------------------------------------------
// Length: TrackArc
// Mapped from KiCad PCB_ARC::GetLength = radius * |angle|
// ---------------------------------------------------------------------------

double itemLength(const TrackArc& arc) {
  double ax = static_cast<double>(arc.start.x.nanometers);
  double ay = static_cast<double>(arc.start.y.nanometers);
  double bx = static_cast<double>(arc.mid.x.nanometers);
  double by = static_cast<double>(arc.mid.y.nanometers);
  double cx = static_cast<double>(arc.end.x.nanometers);
  double cy = static_cast<double>(arc.end.y.nanometers);

  double D = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
  if (std::abs(D) < 1e-6) {
    // Degenerate arc: fall back to chord length
    double dist = distancePoints(arc.start, arc.end);
    return dist / 1000000.0;
  }

  double aSq = ax * ax + ay * ay;
  double bSq = bx * bx + by * by;
  double cSq = cx * cx + cy * cy;
  double centerX = (aSq * (by - cy) + bSq * (cy - ay) + cSq * (ay - by)) / D;
  double centerY = (aSq * (cx - bx) + bSq * (ax - cx) + cSq * (bx - ax)) / D;
  double radius = std::sqrt((ax - centerX) * (ax - centerX) + (ay - centerY) * (ay - centerY));

  double startAngle = std::atan2(ay - centerY, ax - centerX);
  double endAngle = std::atan2(cy - centerY, cx - centerX);
  double midAngle = std::atan2(by - centerY, bx - centerX);

  // Compute the sweep angle through mid
  auto normAngle = [](double a, double ref) -> double {
    double d = a - ref;
    while (d < -M_PI) d += 2.0 * M_PI;
    while (d > M_PI) d -= 2.0 * M_PI;
    return d;
  };

  double midNorm = normAngle(midAngle, startAngle);
  double endNorm = normAngle(endAngle, startAngle);

  double sweepAngle;
  if ((midNorm > 0 && endNorm > 0 && midNorm < endNorm) ||
      (midNorm < 0 && endNorm < 0 && midNorm > endNorm)) {
    sweepAngle = std::abs(endNorm);
  } else {
    // Arc wraps around
    sweepAngle = 2.0 * M_PI - std::abs(endNorm);
  }

  double arcLen = radius * sweepAngle;
  return arcLen / 1000000.0; // nanometers to millimeters
}

// ---------------------------------------------------------------------------
// Annular Ring: Pad
// ---------------------------------------------------------------------------

int64_t padAnnularRing(const Pad& pad) {
  auto [w, h] = getPadShapeSize(pad);
  int64_t drillW = pad.padstack.drill.size.width.nanometers;
  int64_t drillH = pad.padstack.drill.size.height.nanometers;
  int64_t drillMax = std::max(drillW, drillH);
  if (drillMax <= 0) return 0;

  int64_t padMin = std::min(w, h);
  if (padMin <= 0) return 0;

  return (padMin - drillMax) / 2;
}

// ---------------------------------------------------------------------------
// Annular Ring: Via
// ---------------------------------------------------------------------------

int64_t viaAnnularRing(const Via& via) {
  int64_t drillDia = via.drill.nanometers;
  int64_t viaDia = via.diameter.nanometers;
  if (drillDia <= 0 || viaDia <= 0) return 0;
  return (viaDia - drillDia) / 2;
}

// ---------------------------------------------------------------------------
// Schematic Item Geometry
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const SchWire& wire) {
  BoundingBox bb;
  bb.min.x = nanometers(std::min(wire.start.x.nanometers, wire.end.x.nanometers));
  bb.min.y = nanometers(std::min(wire.start.y.nanometers, wire.end.y.nanometers));
  bb.max.x = nanometers(std::max(wire.start.x.nanometers, wire.end.x.nanometers));
  bb.max.y = nanometers(std::max(wire.start.y.nanometers, wire.end.y.nanometers));
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchBus& bus) {
  BoundingBox bb;
  bb.min.x = nanometers(std::min(bus.start.x.nanometers, bus.end.x.nanometers));
  bb.min.y = nanometers(std::min(bus.start.y.nanometers, bus.end.y.nanometers));
  bb.max.x = nanometers(std::max(bus.start.x.nanometers, bus.end.x.nanometers));
  bb.max.y = nanometers(std::max(bus.start.y.nanometers, bus.end.y.nanometers));
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchGraphic& graphic) {
  int64_t halfW = graphic.width.nanometers / 2;
  BoundingBox bb;
  bb.min.x = nanometers(std::min(graphic.start.x.nanometers, graphic.end.x.nanometers) - halfW);
  bb.min.y = nanometers(std::min(graphic.start.y.nanometers, graphic.end.y.nanometers) - halfW);
  bb.max.x = nanometers(std::max(graphic.start.x.nanometers, graphic.end.x.nanometers) + halfW);
  bb.max.y = nanometers(std::max(graphic.start.y.nanometers, graphic.end.y.nanometers) + halfW);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchJunction& junction) {
  int64_t radius = junction.diameter.nanometers / 2;
  BoundingBox bb;
  bb.min.x = nanometers(junction.position.x.nanometers - radius);
  bb.min.y = nanometers(junction.position.y.nanometers - radius);
  bb.max.x = nanometers(junction.position.x.nanometers + radius);
  bb.max.y = nanometers(junction.position.y.nanometers + radius);
  bb.valid = true;
  return bb;
}

bool itemHitTest(const SchWire& wire, Point testPoint, int64_t accuracy_nm) {
  double threshold = static_cast<double>(accuracy_nm);
  if (threshold < 1e-9) threshold = 250000.0; // 0.25mm default schematic wire width
  double dist = distancePointToSegment(testPoint, wire.start, wire.end);
  return dist <= threshold;
}

bool itemHitTest(const SchJunction& junction, Point testPoint) {
  double dx = static_cast<double>(testPoint.x.nanometers - junction.position.x.nanometers);
  double dy = static_cast<double>(testPoint.y.nanometers - junction.position.y.nanometers);
  double r = junction.diameter.nanometers / 2.0;
  if (r < 1e-9) r = 500000.0; // 0.5mm default junction diameter
  return (dx * dx + dy * dy) <= (r * r);
}

BoundingBox itemBoundingBox(const SchText& text) {
  BoundingBox bb;
  bb.min.x = text.position.x;
  bb.min.y = text.position.y;
  bb.max.x = nanometers(text.position.x.nanometers + text.size.width.nanometers);
  bb.max.y = nanometers(text.position.y.nanometers + text.size.height.nanometers);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchTextBox& textbox) {
  BoundingBox bb;
  bb.min.x = textbox.area.origin.x;
  bb.min.y = textbox.area.origin.y;
  bb.max.x = nanometers(textbox.area.origin.x.nanometers + textbox.area.size.width.nanometers);
  bb.max.y = nanometers(textbox.area.origin.y.nanometers + textbox.area.size.height.nanometers);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchLabel& label) {
  BoundingBox bb;
  int64_t default_size = 1250000; // 1.25mm
  bb.min.x = nanometers(label.position.x.nanometers - default_size);
  bb.min.y = nanometers(label.position.y.nanometers - default_size);
  bb.max.x = nanometers(label.position.x.nanometers + default_size);
  bb.max.y = nanometers(label.position.y.nanometers + default_size);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchPowerSymbol& psym) {
  BoundingBox bb;
  int64_t default_size = 2500000; // 2.5mm
  bb.min.x = nanometers(psym.position.x.nanometers - default_size);
  bb.min.y = nanometers(psym.position.y.nanometers - default_size);
  bb.max.x = nanometers(psym.position.x.nanometers + default_size);
  bb.max.y = nanometers(psym.position.y.nanometers + default_size);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchSymbol& symbol) {
  BoundingBox bb;
  int64_t env = 2500000;
  bb.min.x = nanometers(symbol.position.x.nanometers - env);
  bb.min.y = nanometers(symbol.position.y.nanometers - env);
  bb.max.x = nanometers(symbol.position.x.nanometers + env);
  bb.max.y = nanometers(symbol.position.y.nanometers + env);
  bb.valid = true;
  for (const auto& f : symbol.fields) {
    if (f.visible) {
      bb.min.x = nanometers(std::min(bb.min.x.nanometers, f.position.x.nanometers));
      bb.min.y = nanometers(std::min(bb.min.y.nanometers, f.position.y.nanometers));
      bb.max.x = nanometers(std::max(bb.max.x.nanometers, f.position.x.nanometers + f.size.width.nanometers));
      bb.max.y = nanometers(std::max(bb.max.y.nanometers, f.position.y.nanometers + f.size.height.nanometers));
    }
  }
  return bb;
}

BoundingBox itemBoundingBox(const SchSheet& sheet) {
  BoundingBox bb;
  bb.min.x = sheet.position.x;
  bb.min.y = sheet.position.y;
  bb.max.x = nanometers(sheet.position.x.nanometers + sheet.size.width.nanometers);
  bb.max.y = nanometers(sheet.position.y.nanometers + sheet.size.height.nanometers);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchMarker& marker) {
  BoundingBox bb;
  int64_t r = 1000000; // 1mm
  bb.min.x = nanometers(marker.position.x.nanometers - r);
  bb.min.y = nanometers(marker.position.y.nanometers - r);
  bb.max.x = nanometers(marker.position.x.nanometers + r);
  bb.max.y = nanometers(marker.position.y.nanometers + r);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchNoConnect& nc) {
  BoundingBox bb;
  int64_t r = 1000000; // 1mm
  bb.min.x = nanometers(nc.position.x.nanometers - r);
  bb.min.y = nanometers(nc.position.y.nanometers - r);
  bb.max.x = nanometers(nc.position.x.nanometers + r);
  bb.max.y = nanometers(nc.position.y.nanometers + r);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchSheetPin& sheet_pin) {
  BoundingBox bb;
  int64_t r = 1000000; // 1mm radius placeholder for sheet pin point/text
  bb.min.x = nanometers(sheet_pin.position.x.nanometers - r);
  bb.min.y = nanometers(sheet_pin.position.y.nanometers - r);
  bb.max.x = nanometers(sheet_pin.position.x.nanometers + r);
  bb.max.y = nanometers(sheet_pin.position.y.nanometers + r);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchPin& pin) {
  int64_t halfW = pin.length.nanometers / 2;
  int64_t cx = pin.position.x.nanometers;
  int64_t cy = pin.position.y.nanometers;
  switch (pin.orientation) {
    case PinOrientation::Right: cx += halfW; break;
    case PinOrientation::Left:  cx -= halfW; break;
    case PinOrientation::Up:    cy -= halfW; break;
    case PinOrientation::Down:  cy += halfW; break;
    default: break;
  }
  int64_t thick = pin.name_text_size.nanometers;
  if (thick <= 0) thick = 1250000;
  BoundingBox bb;
  bb.min.x = nanometers(cx - halfW - thick);
  bb.min.y = nanometers(cy - halfW - thick);
  bb.max.x = nanometers(cx + halfW + thick);
  bb.max.y = nanometers(cy + halfW + thick);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchField& field) {
  BoundingBox bb;
  if (!field.visible) return bb;
  bb.min.x = field.position.x;
  bb.min.y = field.position.y;
  bb.max.x = nanometers(field.position.x.nanometers + field.size.width.nanometers);
  bb.max.y = nanometers(field.position.y.nanometers + field.size.height.nanometers);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchBusEntry& bus_entry) {
  BoundingBox bb;
  bb.min.x = nanometers(std::min(bus_entry.position.x.nanometers, bus_entry.position.x.nanometers + bus_entry.size.width.nanometers));
  bb.min.y = nanometers(std::min(bus_entry.position.y.nanometers, bus_entry.position.y.nanometers + bus_entry.size.height.nanometers));
  bb.max.x = nanometers(std::max(bus_entry.position.x.nanometers, bus_entry.position.x.nanometers + bus_entry.size.width.nanometers));
  bb.max.y = nanometers(std::max(bus_entry.position.y.nanometers, bus_entry.position.y.nanometers + bus_entry.size.height.nanometers));
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchBitmap& bitmap) {
  BoundingBox bb;
  int64_t hw = static_cast<int64_t>(5000000 * bitmap.scale);
  bb.min.x = nanometers(bitmap.position.x.nanometers - hw);
  bb.min.y = nanometers(bitmap.position.y.nanometers - hw);
  bb.max.x = nanometers(bitmap.position.x.nanometers + hw);
  bb.max.y = nanometers(bitmap.position.y.nanometers + hw);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchRuleArea& rule_area) {
  if (rule_area.outline.empty()) return BoundingBox{};

  int64_t minX = std::numeric_limits<int64_t>::max();
  int64_t minY = std::numeric_limits<int64_t>::max();
  int64_t maxX = std::numeric_limits<int64_t>::min();
  int64_t maxY = std::numeric_limits<int64_t>::min();

  for (const Point& pt : rule_area.outline) {
    minX = std::min(minX, pt.x.nanometers);
    minY = std::min(minY, pt.y.nanometers);
    maxX = std::max(maxX, pt.x.nanometers);
    maxY = std::max(maxY, pt.y.nanometers);
  }

  BoundingBox bb;
  bb.min.x = nanometers(minX);
  bb.min.y = nanometers(minY);
  bb.max.x = nanometers(maxX);
  bb.max.y = nanometers(maxY);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchTable& table) {
  BoundingBox bb;
  bb.min.x = table.position.x;
  bb.min.y = table.position.y;
  bb.max.x = nanometers(table.position.x.nanometers + table.size.width.nanometers);
  bb.max.y = nanometers(table.position.y.nanometers + table.size.height.nanometers);
  bb.valid = true;
  return bb;
}

BoundingBox itemBoundingBox(const SchGroup& group, const Schematic& sch) {
  BoundingBox bbox;
  
  for (const std::string& member_id : group.members) {
    BoundingBox child_bbox;
    for (const auto& item : sch.symbols) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.wires) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.buses) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.labels) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.power_symbols) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.sheets) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.texts) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.textboxes) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.graphics) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.markers) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.bus_entries) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.bitmaps) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.rule_areas) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.tables) { if (item.id == member_id) { child_bbox = itemBoundingBox(item); break; } }
    for (const auto& item : sch.groups) { if (item.id == member_id && item.id != group.id) { child_bbox = itemBoundingBox(item, sch); break; } }
    
    if (child_bbox.valid) {
      if (!bbox.valid) {
        bbox = child_bbox;
      } else {
        bbox.min.x = nanometers(std::min(bbox.min.x.nanometers, child_bbox.min.x.nanometers));
        bbox.min.y = nanometers(std::min(bbox.min.y.nanometers, child_bbox.min.y.nanometers));
        bbox.max.x = nanometers(std::max(bbox.max.x.nanometers, child_bbox.max.x.nanometers));
        bbox.max.y = nanometers(std::max(bbox.max.y.nanometers, child_bbox.max.y.nanometers));
      }
    }
  }
  
  if (bbox.valid) {
    // inflate by 10 mils (254,000 nm) to match KiCad schIUScale.MilsToIU( 10 )
    bbox.min.x = nanometers(bbox.min.x.nanometers - 254000);
    bbox.min.y = nanometers(bbox.min.y.nanometers - 254000);
    bbox.max.x = nanometers(bbox.max.x.nanometers + 254000);
    bbox.max.y = nanometers(bbox.max.y.nanometers + 254000);
  }
  
  return bbox;
}

bool itemHitTest(const SchText& text, Point testPoint) {
  return pointInsideRect(testPoint, text.position, 
                         Point{nanometers(text.position.x.nanometers + text.size.width.nanometers), 
                               nanometers(text.position.y.nanometers + text.size.height.nanometers)});
}

bool itemHitTest(const SchTextBox& textbox, Point testPoint) {
  return pointInsideRect(testPoint, textbox.area.origin, maxPoint(textbox.area));
}

bool itemHitTest(const SchLabel& label, Point testPoint) {
  int64_t r = 1250000; // 1.25mm
  return (std::abs(testPoint.x.nanometers - label.position.x.nanometers) <= r &&
          std::abs(testPoint.y.nanometers - label.position.y.nanometers) <= r);
}

bool itemHitTest(const SchPowerSymbol& psym, Point testPoint) {
  int64_t r = 2500000; // 2.5mm
  return (std::abs(testPoint.x.nanometers - psym.position.x.nanometers) <= r &&
          std::abs(testPoint.y.nanometers - psym.position.y.nanometers) <= r);
}



bool itemHitTest(const SchSheetPin& sheet_pin, Point testPoint) {
  auto bb = itemBoundingBox(sheet_pin);
  return pointInsideRect(testPoint, bb.min, bb.max);
}

bool itemHitTest(const SchSymbol& symbol, Point testPoint) {
  auto bb = itemBoundingBox(symbol);
  return pointInsideRect(testPoint, bb.min, bb.max);
}

bool itemHitTest(const SchSheet& sheet, Point testPoint) {
  return pointInsideRect(testPoint, sheet.position, 
                         Point{nanometers(sheet.position.x.nanometers + sheet.size.width.nanometers), 
                               nanometers(sheet.position.y.nanometers + sheet.size.height.nanometers)});
}

bool itemHitTest(const SchMarker& marker, Point testPoint) {
  int64_t r = 1000000; // 1.0mm
  return (std::abs(testPoint.x.nanometers - marker.position.x.nanometers) <= r &&
          std::abs(testPoint.y.nanometers - marker.position.y.nanometers) <= r);
}

bool itemHitTest(const SchNoConnect& nc, Point testPoint) {
  int64_t r = 1000000; // 1.0mm
  return (std::abs(testPoint.x.nanometers - nc.position.x.nanometers) <= r &&
          std::abs(testPoint.y.nanometers - nc.position.y.nanometers) <= r);
}

bool itemHitTest(const SchPin& pin, Point testPoint, int64_t accuracy_nm) {
  auto bb = itemBoundingBox(pin);
  if (!bb.valid) return false;
  int64_t acc = std::max(accuracy_nm, static_cast<int64_t>(1250000));
  return pointInsideRect(testPoint, 
                         Point{nanometers(bb.min.x.nanometers - acc), nanometers(bb.min.y.nanometers - acc)}, 
                         Point{nanometers(bb.max.x.nanometers + acc), nanometers(bb.max.y.nanometers + acc)});
}

bool itemHitTest(const SchField& field, Point testPoint) {
  auto bb = itemBoundingBox(field);
  return bb.valid && pointInsideRect(testPoint, bb.min, bb.max);
}

bool itemHitTest(const SchBusEntry& bus_entry, Point testPoint, int64_t accuracy_nm) {
  double threshold = static_cast<double>(accuracy_nm);
  if (threshold < 1e-9) threshold = 250000.0;
  Point end = {nanometers(bus_entry.position.x.nanometers + bus_entry.size.width.nanometers),
               nanometers(bus_entry.position.y.nanometers + bus_entry.size.height.nanometers)};
  double dist = distancePointToSegment(testPoint, bus_entry.position, end);
  return dist <= threshold;
}

bool itemHitTest(const SchBitmap& bitmap, Point testPoint) {
  auto bb = itemBoundingBox(bitmap);
  return bb.valid && pointInsideRect(testPoint, bb.min, bb.max);
}

bool itemHitTest(const SchRuleArea& rule_area, Point testPoint) {
  if (rule_area.outline.size() < 3) return false;

  double px = static_cast<double>(testPoint.x.nanometers);
  double py = static_cast<double>(testPoint.y.nanometers);
  bool inside = false;
  size_t n = rule_area.outline.size();
  for (size_t i = 0, j = n - 1; i < n; j = i++) {
    double yi = static_cast<double>(rule_area.outline[i].y.nanometers);
    double yj = static_cast<double>(rule_area.outline[j].y.nanometers);
    double xi = static_cast<double>(rule_area.outline[i].x.nanometers);
    double xj = static_cast<double>(rule_area.outline[j].x.nanometers);

    if (((yi > py) != (yj > py)) &&
        (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
      inside = !inside;
    }
  }
  return inside;
}

bool itemHitTest(const SchTable& table, Point testPoint) {
  auto bb = itemBoundingBox(table);
  return bb.valid && pointInsideRect(testPoint, bb.min, bb.max);
}

bool itemHitTest(const SchGroup& group, const Schematic& sch, Point testPoint) {
  (void)group;
  (void)sch;
  (void)testPoint;
  // Groups are selected by promoting a selection of one of their children
  return false;
}

double itemLength(const SchWire& wire) {
  return distancePoints(wire.start, wire.end) / 1000000.0;
}

double itemLength(const SchBus& bus) {
  return distancePoints(bus.start, bus.end) / 1000000.0;
}

double itemLength(const SchGraphic& graphic) {
  return distancePoints(graphic.start, graphic.end) / 1000000.0;
}

// ---------------------------------------------------------------------------
// Shape description for agent queries
// ---------------------------------------------------------------------------

std::string padShapeDescription(const Pad& pad) {
  PadShape shape = getPadShape(pad);
  switch (shape) {
    case PadShape::Circle:        return "circle";
    case PadShape::Rectangle:     return "rectangle";
    case PadShape::Oval:          return "oval";
    case PadShape::Trapezoid:     return "trapezoid";
    case PadShape::RoundRect:     return "roundrect";
    case PadShape::ChamferedRect: return "chamfered_rect";
    case PadShape::Custom:        return "custom";
    default:                      return "unknown";
  }
}

std::string itemTypeString(const std::string& objectId, const Board& board) {
  for (const auto& p : board.pads) if (p.id == objectId) return "pad";
  for (const auto& v : board.vias) if (v.id == objectId) return "via";
  for (const auto& t : board.tracks) if (t.id == objectId) return "track";
  for (const auto& a : board.track_arcs) if (a.id == objectId) return "track_arc";
  for (const auto& g : board.graphics) if (g.id == objectId) return "graphic";
  for (const auto& t : board.texts) if (t.id == objectId) return "text";
  for (const auto& z : board.zones) if (z.id == objectId) return "zone";
  for (const auto& k : board.keepouts) if (k.id == objectId) return "keepout";
  for (const auto& d : board.dimensions) if (d.id == objectId) return "dimension";
  for (const auto& b : board.barcodes) if (b.id == objectId) return "barcode";
  for (const auto& t : board.targets) if (t.id == objectId) return "target";
  return "unknown";
}

}  // namespace ccad
