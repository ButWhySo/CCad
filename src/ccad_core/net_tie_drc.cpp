#include "net_tie_drc.hpp"
#include "model.hpp"

#include <algorithm>

namespace {
bool samePair(const std::string& a, const std::string& b, const std::string& x,
             const std::string& y) {
  return (a == x && b == y) || (a == y && b == x);
}
}

namespace ccad {

NetTieDrc::NetTieDrc(Board* board)
    : board_(board) {
}

void NetTieDrc::registerNetTie(const std::string& footprintId, const std::string& netA, const std::string& netB) {
    netTies_.push_back({footprintId, netA, netB});
}

bool NetTieDrc::isIntersectionPermitted(const std::string& netA, const std::string& netB, double x, double y) const {
    if (!board_) return false;
    for (const NetTieDef& tie : netTies_) {
      if (!samePair(netA, netB, tie.netA, tie.netB)) continue;
      bool found_a = false;
      bool found_b = false;
      double min_x = 1e30, min_y = 1e30, max_x = -1e30, max_y = -1e30;
      for (const Pad& pad : board_->pads) {
        if (pad.component_id != tie.footprintId) continue;
        if (pad.net_id != tie.netA && pad.net_id != tie.netB) continue;
        found_a |= pad.net_id == tie.netA;
        found_b |= pad.net_id == tie.netB;
        const double px = static_cast<double>(pad.position.x.nanometers) / 1'000'000.0;
        const double py = static_cast<double>(pad.position.y.nanometers) / 1'000'000.0;
        min_x = std::min(min_x, px); min_y = std::min(min_y, py);
        max_x = std::max(max_x, px); max_y = std::max(max_y, py);
      }
      if (found_a && found_b && x >= min_x && x <= max_x && y >= min_y && y <= max_y)
        return true;
    }
    return false;
}

std::vector<std::string> NetTieDrc::validateNetTies() const {
    std::vector<std::string> failingTies;
    if (!board_) return failingTies;
    
    for (const NetTieDef& tie : netTies_) {
      bool found_a = false;
      bool found_b = false;
      for (const Pad& pad : board_->pads) {
        if (pad.component_id != tie.footprintId) continue;
        found_a |= pad.net_id == tie.netA;
        found_b |= pad.net_id == tie.netB;
      }
      if (!found_a || !found_b) failingTies.push_back(tie.footprintId);
    }
    
    return failingTies;
}

} // namespace ccad
