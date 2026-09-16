#include "router_tool.hpp"
#include "model.hpp"
#include "geometry.hpp"
#include <string>
#include <cmath>
#include <algorithm>
namespace ccad {
namespace {
long double cross(const Point& a, const Point& b, const Point& c) {
  return static_cast<long double>(b.x.nanometers - a.x.nanometers) *
         (c.y.nanometers - a.y.nanometers) -
         static_cast<long double>(b.y.nanometers - a.y.nanometers) *
         (c.x.nanometers - a.x.nanometers);
}
bool onSegment(const Point& a, const Point& b, const Point& p) {
  return p.x.nanometers >= std::min(a.x.nanometers, b.x.nanometers) && p.x.nanometers <= std::max(a.x.nanometers, b.x.nanometers) &&
         p.y.nanometers >= std::min(a.y.nanometers, b.y.nanometers) && p.y.nanometers <= std::max(a.y.nanometers, b.y.nanometers);
}
bool intersects(const Point& a, const Point& b, const Point& c, const Point& d) {
  const long double ab_c = cross(a, b, c), ab_d = cross(a, b, d);
  const long double cd_a = cross(c, d, a), cd_b = cross(c, d, b);
  if (((ab_c > 0 && ab_d < 0) || (ab_c < 0 && ab_d > 0)) && ((cd_a > 0 && cd_b < 0) || (cd_a < 0 && cd_b > 0))) return true;
  return (ab_c == 0 && onSegment(a, b, c)) || (ab_d == 0 && onSegment(a, b, d)) || (cd_a == 0 && onSegment(c, d, a)) || (cd_b == 0 && onSegment(c, d, b));
}
bool pointInsidePolygon(const Point& p, const std::vector<Point>& polygon) {
  bool inside = false;
  for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const bool crosses = ((polygon[i].y.nanometers > p.y.nanometers) != (polygon[j].y.nanometers > p.y.nanometers));
    if (crosses && static_cast<long double>(p.x.nanometers) <
        static_cast<long double>(polygon[j].x.nanometers - polygon[i].x.nanometers) *
        (p.y.nanometers - polygon[i].y.nanometers) /
        (polygon[j].y.nanometers - polygon[i].y.nanometers) + polygon[i].x.nanometers) inside = !inside;
  }
  return inside;
}
bool segmentTouchesPolygon(const Point& start, const Point& end, const std::vector<Point>& polygon, double clearance) {
  if (polygon.size() < 3 || pointInsidePolygon(start, polygon) || pointInsidePolygon(end, polygon)) return true;
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const Point& a = polygon[i];
    const Point& b = polygon[(i + 1) % polygon.size()];
    if (intersects(start, end, a, b) || distancePointToSegment(a, start, end) < clearance ||
        distancePointToSegment(b, start, end) < clearance) return true;
  }
  return false;
}
bool segmentTouchesArc(const Point& start, const Point& end, const TrackArc& arc, double clearance) {
  const long double ax = arc.start.x.nanometers, ay = arc.start.y.nanometers;
  const long double bx = arc.mid.x.nanometers, by = arc.mid.y.nanometers;
  const long double cx = arc.end.x.nanometers, cy = arc.end.y.nanometers;
  const long double determinant = 2.0L * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
  const bool circular = std::abs(determinant) > 1e-9L;
  long double center_x = 0.0L, center_y = 0.0L, radius = 0.0L, start_angle = 0.0L, sweep = 0.0L;
  if (circular) {
    center_x = ((ax * ax + ay * ay) * (by - cy) + (bx * bx + by * by) * (cy - ay) + (cx * cx + cy * cy) * (ay - by)) / determinant;
    center_y = ((ax * ax + ay * ay) * (cx - bx) + (bx * bx + by * by) * (ax - cx) + (cx * cx + cy * cy) * (bx - ax)) / determinant;
    radius = std::hypotl(ax - center_x, ay - center_y);
    start_angle = std::atan2(ay - center_y, ax - center_x);
    const long double mid_angle = std::atan2(by - center_y, bx - center_x);
    const long double end_angle = std::atan2(cy - center_y, cx - center_x);
    const long double two_pi = 2.0L * std::acos(-1.0L);
    const bool ccw = cross(arc.start, arc.mid, arc.end) > 0.0L;
    sweep = ccw ? end_angle - start_angle : start_angle - end_angle;
    long double mid_sweep = ccw ? mid_angle - start_angle : start_angle - mid_angle;
    while (sweep < 0.0L) sweep += two_pi;
    while (mid_sweep < 0.0L) mid_sweep += two_pi;
    if (mid_sweep > sweep) sweep += two_pi;
    if (!ccw) sweep = -sweep;
  }
  Point previous = arc.start;
  for (int step = 1; step <= 16; ++step) {
    const long double t = static_cast<long double>(step) / 16.0L;
    const long double one_minus_t = 1.0L - t;
    const long double angle = circular ? start_angle + sweep * t : 0.0L;
    const long double x = circular ? center_x + radius * std::cos(angle) : one_minus_t * one_minus_t * ax + 2.0L * one_minus_t * t * bx + t * t * cx;
    const long double y = circular ? center_y + radius * std::sin(angle) : one_minus_t * one_minus_t * ay + 2.0L * one_minus_t * t * by + t * t * cy;
    const Point current{nanometers(static_cast<std::int64_t>(std::llround(x))),
                        nanometers(static_cast<std::int64_t>(std::llround(y)))};
    if (intersects(start, end, previous, current) || distancePointToSegment(previous, start, end) < clearance) return true;
    previous = current;
  }
  return distancePointToSegment(previous, start, end) < clearance;
}
}
void RouterTool::setBoard(Board* board) { board_ = board; }
void RouterTool::setActiveNet(const std::string& net_id) { active_net_id_ = net_id; }
void RouterTool::routeTrack(double x1,double y1,double x2,double y2) {
  routeTrack(x1, y1, x2, y2, 0);
}
void RouterTool::routeTrack(double x1,double y1,double x2,double y2, int layer) {
  if (!board_) return;
  startRouting(x1, y1, layer);
  if (x1 != x2 && y1 != y2) {
    updateRouting(x2, y1);
    commitRouting();
    if (last_commit_blocked_) return;
    startRouting(x2, y1, layer);
  }
  updateRouting(x2, y2);
  commitRouting();
}
void RouterTool::startRouting(double x,double y,int layer) { if (!board_) return; routing_=true; last_commit_blocked_=false; blocked_reason_.clear(); start_x_=x; start_y_=y; layer_=layer; current_x_=x; current_y_=y; }
void RouterTool::updateRouting(double x,double y) {
  if (!board_ || !routing_) return;
  current_x_=x; current_y_=y;
  double best = 0.75;
  for (const Pad& pad : board_->pads) {
    if (!active_net_id_.empty() && pad.net_id != active_net_id_) continue;
    const double px = static_cast<double>(pad.position.x.nanometers) / 1'000'000.0;
    const double py = static_cast<double>(pad.position.y.nanometers) / 1'000'000.0;
    const double distance = std::hypot(current_x_ - px, current_y_ - py);
    if (distance <= best) { best = distance; current_x_ = px; current_y_ = py; }
  }
  for (const Via& via : board_->vias) {
    if (!active_net_id_.empty() && via.net_id != active_net_id_) continue;
    const double px = static_cast<double>(via.position.x.nanometers) / 1'000'000.0;
    const double py = static_cast<double>(via.position.y.nanometers) / 1'000'000.0;
    const double distance = std::hypot(current_x_ - px, current_y_ - py);
    if (distance <= best) { best = distance; current_x_ = px; current_y_ = py; }
  }
}
void RouterTool::commitRouting() {
  if (!board_ || !routing_) return;
  const Point start{millimeters(start_x_), millimeters(start_y_)};
  const Point end{millimeters(current_x_), millimeters(current_y_)};
  const double required = static_cast<double>(board_->design_rules.copper_clearance.nanometers) / 1'000'000.0 + 0.125;
  for (const Pad& pad : board_->pads) {
    if (active_net_id_.empty() || pad.net_id.empty() || pad.net_id == active_net_id_) continue;
    if (distancePointToSegment(pad.position, start, end) < required) { routing_ = false; last_commit_blocked_=true; blocked_reason_="pad"; return; }
  }
  for (const Via& via : board_->vias) {
    if (active_net_id_.empty() || via.net_id.empty() || via.net_id == active_net_id_) continue;
    const double via_radius = static_cast<double>(via.diameter.nanometers) / 2'000'000.0;
    if (distancePointToSegment(via.position, start, end) < required + via_radius) {
      routing_ = false;
      last_commit_blocked_ = true;
      blocked_reason_="via";
      return;
    }
  }
  const std::string layer_id = layer_ == 0 ? "F.Cu" : "B.Cu";
  for (const TrackArc& arc : board_->track_arcs) {
    if (arc.layer_id != layer_id || active_net_id_.empty() || arc.net_id.empty() || arc.net_id == active_net_id_) continue;
    const double arc_clearance = required + static_cast<double>(arc.width.nanometers) / 2'000'000.0;
    if (segmentTouchesArc(start, end, arc, arc_clearance)) {
      routing_ = false;
      last_commit_blocked_ = true;
      blocked_reason_="arc";
      return;
    }
  }
  for (const BoardZone& zone : board_->zones) {
    if (!zone.fill_enabled || zone.net_id.empty() || zone.net_id == active_net_id_ ||
        std::find(zone.layer_ids.begin(), zone.layer_ids.end(), layer_id) == zone.layer_ids.end()) continue;
    const double zone_clearance = static_cast<double>(zone.clearance.nanometers) / 1'000'000.0 + 0.125;
    if (segmentTouchesPolygon(start, end, zone.outline, zone_clearance)) {
      routing_ = false;
      last_commit_blocked_ = true;
      blocked_reason_="zone";
      return;
    }
  }
  for (const TrackSegment& track : board_->tracks) {
    if (track.layer_id != layer_id || active_net_id_.empty() || track.net_id.empty() ||
        track.net_id == active_net_id_) continue;
    if (intersects(start, end, track.start, track.end) ||
        distancePointToSegment(track.start, start, end) < required ||
        distancePointToSegment(track.end, start, end) < required) {
      routing_ = false;
      last_commit_blocked_=true;
      blocked_reason_="track";
      return;
    }
  }
  if (start_x_ != current_x_ || start_y_ != current_y_)
    board_->tracks.push_back(TrackSegment{.id="interactive-track-"+std::to_string(board_->tracks.size()+1),.net_id=active_net_id_,.layer_id=layer_id,.start=start,.end=end,.width=millimeters(0.25),.source_route_request_id=""});
  routing_=false;
}
void RouterTool::cancelRouting() { if (board_) routing_=false; }
} // namespace ccad
