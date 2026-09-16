#ifndef CCAD_CORE_ZONE_FILL_HPP
#define CCAD_CORE_ZONE_FILL_HPP

#include "model.hpp"

#include <string>
#include <vector>

namespace ccad {

// Deterministic first-stage zone fill result. Contours retain outer/holes
// separately so later clearance and thermal passes can replace them without
// changing the agent-facing contract.
struct ZoneFillResult {
  bool filled = false;
  std::vector<std::vector<Point>> contours;
  int64_t area_square_nanometers = 0;
  std::vector<std::string> diagnostics;
};

struct ZoneThermalSpoke {
  Point start;
  Point end;
  Length width;
};

std::vector<ZoneThermalSpoke> buildRectangularThermalSpokes(
    const BoardZone& zone, const Point& pad_center, Length pad_radius,
    Length gap, Length spoke_width);

ZoneFillResult calculateZoneFill(const BoardZone& zone);

}  // namespace ccad

#endif  // CCAD_CORE_ZONE_FILL_HPP
