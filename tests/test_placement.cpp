#include "ccad_core/placement.hpp"
#include "ccad_core/model.hpp"

#include <iostream>
#include <stdexcept>

void test_basic_placement() {
  ccad::Project project;
  project.board = ccad::Board{};
  project.board->outline.origin = {ccad::millimeters(0)};
  project.board->outline.origin.y = ccad::millimeters(0);
  project.board->outline.size = {ccad::millimeters(100), ccad::millimeters(100)};
  project.board->layers.push_back({.id = "F.Cu", .kind = "copper"});

  ccad::Footprint fp;
  fp.pads.push_back({
      .number = "1",
      .position = {ccad::millimeters(0), ccad::millimeters(0)},
      .rotation_degrees = 0,
      .size = {ccad::millimeters(1), ccad::millimeters(1)},
  });

  ccad::placeFootprint(project, fp, "U1", {ccad::millimeters(50), ccad::millimeters(50)}, 0, "F.Cu");

  if (project.board->pads.size() != 1) {
    throw std::runtime_error("Pad count should be 1");
  }
  if (project.board->pads[0].position.x.nanometers != ccad::millimeters(50).nanometers) {
    throw std::runtime_error("Pad X should be 50mm");
  }
}

int main() {
  try {
    test_basic_placement();
    std::cout << "All tests passed!\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed: " << e.what() << "\n";
    return 1;
  }
}
