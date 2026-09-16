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
  const std::string layer_id = layer_ == 0 ? "F.Cu" : "B.Cu";
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
