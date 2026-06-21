#include "ccad_core/fix_board_shape.hpp"
#include "ccad_core/geometry.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ccad;

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_connect_adjacent_graphics() {
  BoardGraphic g1{"g1", "line", "F.Cu", {millimeters(0.0), millimeters(0.0)}, {millimeters(10.0), millimeters(0.0)}, millimeters(0.1), false};
  BoardGraphic g2{"g2", "line", "F.Cu", {millimeters(10.05), millimeters(0.0)}, {millimeters(20.0), millimeters(0.0)}, millimeters(0.1), false};

  std::vector<BoardGraphic*> graphics = {&g1, &g2};

  // Does not connect when epsilon is too small
  connectBoardShapes(graphics, millimeters(0.01));
  require(g1.end.x.nanometers == millimeters(10.0).nanometers, "small epsilon does not connect end X");
  require(g2.start.x.nanometers == millimeters(10.05).nanometers, "small epsilon does not connect start X");

  // Connects when epsilon is large enough
  connectBoardShapes(graphics, millimeters(0.1));
  require(g1.end.x.nanometers == millimeters(10.025).nanometers, "large epsilon connects end X");
  require(g2.start.x.nanometers == millimeters(10.025).nanometers, "large epsilon connects start X");
}

void test_connect_locked_shapes_skipped() {
  BoardGraphic g1{"g1", "line", "F.Cu", {millimeters(0.0), millimeters(0.0)}, {millimeters(10.0), millimeters(0.0)}, millimeters(0.1), true};
  BoardGraphic g2{"g2", "line", "F.Cu", {millimeters(10.05), millimeters(0.0)}, {millimeters(20.0), millimeters(0.0)}, millimeters(0.1), false};

  std::vector<BoardGraphic*> graphics = {&g1, &g2};

  connectBoardShapes(graphics, millimeters(0.1));
  require(g1.end.x.nanometers == millimeters(10.0).nanometers, "locked shape is skipped");
  require(g2.start.x.nanometers == millimeters(10.05).nanometers, "locked shape is skipped");
}

void test_connect_closed_loop() {
  BoardGraphic g1{"g1", "line", "F.Cu", {millimeters(0.0), millimeters(0.0)}, {millimeters(10.0), millimeters(0.0)}, millimeters(0.1), false};
  BoardGraphic g2{"g2", "line", "F.Cu", {millimeters(10.0), millimeters(0.0)}, {millimeters(10.0), millimeters(10.0)}, millimeters(0.1), false};
  BoardGraphic g3{"g3", "line", "F.Cu", {millimeters(10.0), millimeters(10.0)}, {millimeters(0.0), millimeters(10.0)}, millimeters(0.1), false};
  BoardGraphic g4{"g4", "line", "F.Cu", {millimeters(0.0), millimeters(10.0)}, {millimeters(0.0), millimeters(0.05)}, millimeters(0.1), false};

  std::vector<BoardGraphic*> graphics = {&g1, &g2, &g3, &g4};

  connectBoardShapes(graphics, millimeters(0.1));
  require(g1.start.y.nanometers == millimeters(0.025).nanometers, "closed loop snaps start Y");
  require(g4.end.y.nanometers == millimeters(0.025).nanometers, "closed loop snaps end Y");
}

} // namespace

int main() {
  try {
    test_connect_adjacent_graphics();
    test_connect_locked_shapes_skipped();
    test_connect_closed_loop();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
