#include "ccad_core/autoplacer.hpp"

#include "ccad_core/footprint.hpp"
#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

double mm(ccad::Length value) {
  return static_cast<double>(value.nanometers) / 1000000.0;
}

ccad::Board boardWithOutline(double width_mm, double height_mm) {
  ccad::Board board;
  board.outline.origin = {ccad::millimeters(0), ccad::millimeters(0)};
  board.outline.size = {ccad::millimeters(width_mm), ccad::millimeters(height_mm)};
  board.layers.push_back({.id = "F.Cu", .kind = "copper", .visible = true});
  return board;
}

ccad::Footprint onePadFootprint() {
  ccad::Footprint footprint;
  footprint.name = "U";
  footprint.pads.push_back(ccad::FootprintPad{.number = "1",
                                              .type = "smd",
                                              .shape = "rect",
                                              .position = {ccad::millimeters(0), ccad::millimeters(0)},
                                              .size = {ccad::millimeters(1), ccad::millimeters(1)},
                                              .layers = {"F.Cu"}});
  return footprint;
}

void test_autoplacer_avoids_existing_pad_occupancy() {
  ccad::Board board = boardWithOutline(6, 4);
  board.pads.push_back(ccad::Pad{.id = "U1.1",
                                 .component_id = "U1",
                                 .pin_name = "1",
                                 .net_id = "N1",
                                 .layers = {"F.Cu"},
                                 .shape = "rect",
                                 .position = {ccad::millimeters(0), ccad::millimeters(0)},
                                 .size = {ccad::millimeters(1), ccad::millimeters(1)}});

  const ccad::AutoPlacementPlan plan = ccad::planFootprintAutoPlacement(
      board, onePadFootprint(), {{"1", "N1"}}, "F.Cu", ccad::millimeters(1));

  require(plan.placeable, "autoplacer should find a free candidate");
  require(!(mm(plan.origin.x) == 0.0 && mm(plan.origin.y) == 0.0),
          "autoplacer should not place on an occupied pad");
}

void test_autoplacer_prefers_low_same_net_ratsnest_cost() {
  ccad::Board board = boardWithOutline(12, 4);
  board.pads.push_back(ccad::Pad{.id = "J1.1",
                                 .component_id = "J1",
                                 .pin_name = "1",
                                 .net_id = "N1",
                                 .layers = {"F.Cu"},
                                 .shape = "rect",
                                 .position = {ccad::millimeters(10), ccad::millimeters(2)},
                                 .size = {ccad::millimeters(1), ccad::millimeters(1)}});

  const ccad::AutoPlacementPlan plan = ccad::planFootprintAutoPlacement(
      board, onePadFootprint(), {{"1", "N1"}}, "F.Cu", ccad::millimeters(1));

  require(plan.placeable, "autoplacer should find a low-cost candidate");
  require(mm(plan.origin.x) >= 8.0,
          "autoplacer should prefer a candidate near the same-net pad instead of the first free cell");
}

void test_autoplacer_avoids_placement_keepouts() {
  ccad::Board board = boardWithOutline(5, 5);
  board.keepouts.push_back(ccad::Keepout{.id = "K1",
                                         .kind = "placement",
                                         .area = {.origin = {ccad::millimeters(0), ccad::millimeters(0)},
                                                  .size = {ccad::millimeters(3), ccad::millimeters(3)}}});

  const ccad::AutoPlacementPlan plan = ccad::planFootprintAutoPlacement(
      board, onePadFootprint(), {}, "F.Cu", ccad::millimeters(1));

  require(plan.placeable, "autoplacer should find a candidate outside the keepout");
  require(mm(plan.origin.x) >= 3.0 || mm(plan.origin.y) >= 3.0,
          "autoplacer should not place inside a placement keepout");
}

void test_autoplacer_prefers_lower_existing_pad_keepout_cost() {
  ccad::Board board = boardWithOutline(7, 4);
  board.pads.push_back(ccad::Pad{.id = "J1.1",
                                 .component_id = "J1",
                                 .pin_name = "1",
                                 .net_id = "",
                                 .layers = {"F.Cu"},
                                 .shape = "rect",
                                 .position = {ccad::millimeters(2), ccad::millimeters(2)},
                                 .size = {ccad::millimeters(1), ccad::millimeters(1)}});

  const ccad::AutoPlacementPlan plan = ccad::planFootprintAutoPlacement(
      board, onePadFootprint(), {}, "F.Cu", ccad::millimeters(1));

  require(plan.placeable, "autoplacer should find a candidate with keepout costs");
  require(mm(plan.origin.x) >= 4.0,
          "autoplacer should prefer a lower-cost location away from existing pad clearance");
}

}  // namespace

int main() {
  try {
    test_autoplacer_avoids_existing_pad_occupancy();
    test_autoplacer_prefers_low_same_net_ratsnest_cost();
    test_autoplacer_avoids_placement_keepouts();
    test_autoplacer_prefers_lower_existing_pad_keepout_cost();
    std::cout << "All tests passed!\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Test failed: " << error.what() << "\n";
    return 1;
  }
}
