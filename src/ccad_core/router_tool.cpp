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
  return intersects(start, end, arc.start, arc.mid) || intersects(start, end, arc.mid, arc.end) ||
         distancePointToSegment(arc.start, start, end) < clearance ||
         distancePointToSegment(arc.mid, start, end) < clearance ||
         distancePointToSegment(arc.end, start, end) < clearance;
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
void RouterTool::startRouting(double x,double y,int layer) { if (!board_) return; routing_=true; last_commit_blocked_=false; start_x_=x; start_y_=y; layer_=layer; current_x_=x; current_y_=y; }
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
    if (distancePointToSegment(pad.position, start, end) < required) { routing_ = false; last_commit_blocked_=true; return; }
  }
  for (const Via& via : board_->vias) {
    if (active_net_id_.empty() || via.net_id.empty() || via.net_id == active_net_id_) continue;
    const double via_radius = static_cast<double>(via.diameter.nanometers) / 2'000'000.0;
    if (distancePointToSegment(via.position, start, end) < required + via_radius) {
      routing_ = false;
      last_commit_blocked_ = true;
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
      return;
    }
  }
  if (start_x_ != current_x_ || start_y_ != current_y_)
    board_->tracks.push_back(TrackSegment{.id="interactive-track-"+std::to_string(board_->tracks.size()+1),.net_id=active_net_id_,.layer_id=layer_id,.start=start,.end=end,.width=millimeters(0.25),.source_route_request_id=""});
  routing_=false;
}
void RouterTool::cancelRouting() { if (board_) routing_=false; }
} // namespace ccad
