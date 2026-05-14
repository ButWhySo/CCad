#include "ccad_core/geometry.hpp"

#include <cmath>

namespace ccad {

Length nanometers(const std::int64_t value) {
  return Length{.nanometers = value};
}

Length millimeters(const double value) {
  return Length{.nanometers = static_cast<std::int64_t>(std::llround(value * 1000000.0))};
}

Length mils(const double value) {
  return Length{.nanometers = static_cast<std::int64_t>(std::llround(value * 25400.0))};
}

Point maxPoint(const Rect& rect) {
  return Point{
      .x = nanometers(rect.origin.x.nanometers + rect.size.width.nanometers),
      .y = nanometers(rect.origin.y.nanometers + rect.size.height.nanometers),
  };
}

}  // namespace ccad

