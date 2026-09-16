#include "drc_test_provider_courtyard.hpp"
#include "model.hpp"

#include <algorithm>
#include <cstdint>

namespace {
using ccad::Point;

int orientation(const Point& a, const Point& b, const Point& c) {
  const auto cross = (b.x.nanometers - a.x.nanometers) * (c.y.nanometers - a.y.nanometers) -
                     (b.y.nanometers - a.y.nanometers) * (c.x.nanometers - a.x.nanometers);
  return (cross > 0) - (cross < 0);
}

bool onSegment(const Point& a, const Point& b, const Point& p) {
  return p.x.nanometers >= std::min(a.x.nanometers, b.x.nanometers) &&
         p.x.nanometers <= std::max(a.x.nanometers, b.x.nanometers) &&
         p.y.nanometers >= std::min(a.y.nanometers, b.y.nanometers) &&
         p.y.nanometers <= std::max(a.y.nanometers, b.y.nanometers) && orientation(a, b, p) == 0;
}

bool segmentsIntersect(const Point& a, const Point& b, const Point& c, const Point& d) {
  const int ab_c = orientation(a, b, c), ab_d = orientation(a, b, d);
  const int cd_a = orientation(c, d, a), cd_b = orientation(c, d, b);
  return (ab_c != ab_d && cd_a != cd_b) ||
         (ab_c == 0 && onSegment(a, b, c)) || (ab_d == 0 && onSegment(a, b, d)) ||
         (cd_a == 0 && onSegment(c, d, a)) || (cd_b == 0 && onSegment(c, d, b));
}

bool inside(const Point& point, const std::vector<Point>& polygon) {
  bool result = false;
  for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const auto& a = polygon[i];
    const auto& b = polygon[j];
    if (onSegment(a, b, point)) return true;
    if ((a.y.nanometers > point.y.nanometers) != (b.y.nanometers > point.y.nanometers)) {
      const long double x = static_cast<long double>(b.x.nanometers - a.x.nanometers) *
                                (point.y.nanometers - a.y.nanometers) /
                                static_cast<long double>(b.y.nanometers - a.y.nanometers) +
                            a.x.nanometers;
      if (x >= point.x.nanometers) result = !result;
    }
  }
  return result;
}

bool polygonsOverlap(const std::vector<Point>& a, const std::vector<Point>& b) {
  if (a.size() < 3 || b.size() < 3) return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    for (std::size_t j = 0; j < b.size(); ++j)
      if (segmentsIntersect(a[i], a[(i + 1) % a.size()], b[j], b[(j + 1) % b.size()])) return true;
  return inside(a.front(), b) || inside(b.front(), a);
}
}

namespace ccad {

void DrcTestProviderCourtyard::run(const Board& board, std::vector<DrcItem>& violations) {
  for (std::size_t i = 0; i < board.footprints.size(); ++i) {
    const auto& left = board.footprints[i];
    for (std::size_t j = i + 1; j < board.footprints.size(); ++j) {
      const auto& right = board.footprints[j];
      const bool front = std::any_of(left.front_courtyard.begin(), left.front_courtyard.end(),
                                     [&](const auto& polygon) {
                                       return std::any_of(right.front_courtyard.begin(), right.front_courtyard.end(),
                                                          [&](const auto& other) { return polygonsOverlap(polygon, other); });
                                     });
      const bool back = std::any_of(left.back_courtyard.begin(), left.back_courtyard.end(),
                                    [&](const auto& polygon) {
                                      return std::any_of(right.back_courtyard.begin(), right.back_courtyard.end(),
                                                         [&](const auto& other) { return polygonsOverlap(polygon, other); });
                                    });
      if (front || back)
        violations.emplace_back(3, "Courtyard overlap: " + left.reference + " / " + right.reference);
    }
  }
}

} // namespace ccad
