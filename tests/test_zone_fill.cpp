#include "ccad_core/zone_fill.hpp"

#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
ccad::Point p(int x, int y) { return {ccad::nanometers(x), ccad::nanometers(y)}; }
}

int main() {
  ccad::BoardZone zone;
  zone.outline = {p(0, 0), p(10, 0), p(10, 10), p(0, 10)};
  zone.holes = {{p(2, 2), p(4, 2), p(4, 4), p(2, 4)}};
  const auto filled = ccad::calculateZoneFill(zone);
  require(filled.filled, "enabled zone fills");
  require(filled.contours.size() == 2, "fill retains outer and hole contours");
  require(filled.area_square_nanometers == 96, "fill area subtracts hole");
  zone.clearance = ccad::nanometers(1);
  const auto cleared = ccad::calculateZoneFill(zone);
  require(cleared.filled && cleared.contours.front().at(0).x.nanometers == 1,
          "rectangular fill applies clearance inset");
  require(cleared.area_square_nanometers == 48, "clearance fill expands hole knockout");
  zone.fill_enabled = false;
  require(!ccad::calculateZoneFill(zone).filled, "disabled zone does not fill");
  zone.fill_enabled = true;
  zone.holes[0].pop_back();
  zone.holes[0].pop_back();
  const auto invalid = ccad::calculateZoneFill(zone);
  require(!invalid.filled && !invalid.diagnostics.empty(), "invalid hole rejected");
  std::cout << "Zone fill tests passed!\n";
}
