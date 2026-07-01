#include "ccad_core/spread_footprints.hpp"

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

ccad::Pad pad(const std::string& id, const std::string& component_id, double x_mm, double y_mm) {
  return ccad::Pad{.id = id,
                   .component_id = component_id,
                   .pin_name = "1",
                   .net_id = "",
                   .position = {ccad::millimeters(x_mm), ccad::millimeters(y_mm)},
                   .padstack = ccad::Padstack{
                      .layer_set = {"F.Cu"},
                      .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = {ccad::millimeters(1), ccad::millimeters(1)}}}}}
                   }};
}

void test_spread_moves_symbols_to_non_overlapping_target_area() {
  ccad::Board board;
  board.outline = {.origin = {ccad::millimeters(0), ccad::millimeters(0)},
                   .size = {ccad::millimeters(10), ccad::millimeters(8)}};
  board.pads.push_back(pad("R1.1", "R1", 0, 0));
  board.pads.push_back(pad("R2.1", "R2", 0, 0));

  const std::vector<ccad::SpreadFootprintPlacement> placements =
      ccad::spreadFootprintComponents(board,
                                      ccad::SpreadFootprintRequest{
                                          .component_ids = {},
                                          .target = {ccad::millimeters(12), ccad::millimeters(0)},
                                          .component_gap = ccad::millimeters(1),
                                          .group_gap = ccad::millimeters(1.5)});

  require(placements.size() == 2, "spread should move two component groups");
  require(board.pads.at(0).position.x.nanometers >= ccad::millimeters(12).nanometers,
          "first component should move to target area");
  require(board.pads.at(1).position.x.nanometers > board.pads.at(0).position.x.nanometers,
          "second component should be moved clear of the first");
}

void test_spread_sorts_reference_designators_naturally() {
  ccad::Board board;
  board.pads.push_back(pad("R10.1", "R10", 0, 0));
  board.pads.push_back(pad("R2.1", "R2", 0, 0));
  board.pads.push_back(pad("C1.1", "C1", 0, 0));

  const std::vector<ccad::SpreadFootprintPlacement> placements =
      ccad::spreadFootprintComponents(board,
                                      ccad::SpreadFootprintRequest{
                                          .component_ids = {},
                                          .target = {ccad::millimeters(20), ccad::millimeters(5)},
                                          .component_gap = ccad::millimeters(1),
                                          .group_gap = ccad::millimeters(1.5)});

  require(placements.at(0).component_id == "C1", "capacitor prefix should sort before resistor");
  require(placements.at(1).component_id == "R2", "R2 should sort before R10");
  require(placements.at(2).component_id == "R10", "R10 should sort after R2");
}

void test_spread_can_filter_component_ids() {
  ccad::Board board;
  board.pads.push_back(pad("U1.1", "U1", 0, 0));
  board.pads.push_back(pad("U2.1", "U2", 0, 0));

  const std::vector<ccad::SpreadFootprintPlacement> placements =
      ccad::spreadFootprintComponents(board,
                                      ccad::SpreadFootprintRequest{
                                          .component_ids = {"U2"},
                                          .target = {ccad::millimeters(10), ccad::millimeters(0)},
                                          .component_gap = ccad::millimeters(1),
                                          .group_gap = ccad::millimeters(1.5)});

  require(placements.size() == 1, "spread should move only filtered component");
  require(placements.at(0).component_id == "U2", "spread should report filtered component");
  require(board.pads.at(0).position.x.nanometers == 0, "unselected component should not move");
  require(board.pads.at(1).position.x.nanometers >= ccad::millimeters(10).nanometers,
          "selected component should move");
}

}  // namespace

int main() {
  try {
    test_spread_moves_symbols_to_non_overlapping_target_area();
    test_spread_sorts_reference_designators_naturally();
    test_spread_can_filter_component_ids();
    std::cout << "All tests passed!\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Test failed: " << error.what() << "\n";
    return 1;
  }
}
