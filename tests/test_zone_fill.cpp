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
  zone.holes[0] = {p(2, 2), p(4, 2), p(4, 4), p(2, 4)};
  zone.clearance = ccad::nanometers(-1);
  const auto invalid_clearance = ccad::calculateZoneFill(zone);
  require(!invalid_clearance.filled && !invalid_clearance.diagnostics.empty(),
          "negative clearance rejected");
  zone.clearance = ccad::nanometers(0);
  zone.holes[0] = {p(20, 20), p(22, 20), p(22, 22), p(20, 22)};
  const auto outside_hole = ccad::calculateZoneFill(zone);
  require(!outside_hole.filled && !outside_hole.diagnostics.empty(),
          "outside hole rejected");
  zone.clearance = ccad::nanometers(0);
  const auto spokes = ccad::buildRectangularThermalSpokes(
      zone, p(5, 5), ccad::nanometers(1), ccad::nanometers(1), ccad::nanometers(1));
  require(spokes.size() == 4 && spokes.front().start.x.nanometers == 7 &&
              spokes.front().end.x.nanometers == 10,
          "rectangular thermal spokes reach zone boundary");
  zone.holes = {{p(4, 4), p(6, 4), p(6, 6), p(4, 6)}};
  const auto hole_spokes = ccad::buildRectangularThermalSpokes(
      zone, p(5, 5), ccad::nanometers(1), ccad::nanometers(1), ccad::nanometers(1));
  require(hole_spokes.empty(), "thermal spokes reject pad inside zone hole");
  zone.holes = {{p(7, 4), p(8, 4), p(8, 6), p(7, 6)}};
  const auto blocked_spokes = ccad::buildRectangularThermalSpokes(
      zone, p(5, 5), ccad::nanometers(1), ccad::nanometers(1), ccad::nanometers(1));
  require(blocked_spokes.size() == 3, "thermal spokes stop at rectangular hole");
  zone.holes = {{p(7, 4), p(9, 5), p(7, 6)}};
  const auto polygon_blocked_spokes = ccad::buildRectangularThermalSpokes(
      zone, p(5, 5), ccad::nanometers(1), ccad::nanometers(1), ccad::nanometers(1));
  require(polygon_blocked_spokes.size() == 3, "thermal spokes stop at polygon hole");
  zone.holes.clear();
  zone.net_id = "GND";
  ccad::Pad pad;
  pad.net_id = "GND";
  pad.position = p(5, 5);
  pad.padstack.copper_props["F.Cu"].shape.size =
      {ccad::nanometers(2), ccad::nanometers(2)};
  pad.padstack.copper_props["F.Cu"].thermal_gap = ccad::nanometers(2);
  pad.padstack.copper_props["F.Cu"].thermal_spoke_width = ccad::nanometers(3);
  const auto board_fill = ccad::calculateZoneFill(zone, std::vector<ccad::Pad>{pad});
  require(board_fill.filled && board_fill.thermal_spokes.size() == 4,
          "board zone fill reports matching-pad thermal spokes");
  require(board_fill.thermal_spokes.front().start.x.nanometers == 8 &&
              board_fill.thermal_spokes.front().width.nanometers == 3,
          "pad thermal overrides control spoke geometry");
  pad.padstack.copper_props["F.Cu"].zone_connection = "none";
  const auto pad_none_fill = ccad::calculateZoneFill(zone, std::vector<ccad::Pad>{pad});
  require(pad_none_fill.thermal_spokes.empty(), "pad connection override suppresses spokes");
  pad.padstack.copper_props["F.Cu"].zone_connection.reset();
  zone.layer_ids = {"F.Cu"};
  zone.pad_connection = "thermal";
  pad.padstack.layer_set = {"B.Cu"};
  const auto wrong_layer_fill = ccad::calculateZoneFill(zone, std::vector<ccad::Pad>{pad});
  require(wrong_layer_fill.thermal_spokes.empty(), "thermal spokes ignore pad on other layer");
  pad.padstack.layer_set = {"F.Cu"};
  zone.pad_connection = "none";
  const auto none_fill = ccad::calculateZoneFill(zone, std::vector<ccad::Pad>{pad});
  require(none_fill.thermal_spokes.empty(), "none pad connection suppresses thermal spokes");
  std::cout << "Zone fill tests passed!\n";
}
