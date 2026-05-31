#include "ccad_core/pnp_export.hpp"
#include "test_support.hpp"

#include <iostream>
#include <string>

int main() {
  ccad::Project project;
  project.id = "proj_123";
  project.name = "Test Project";

  project.components = {
      ccad::Component{.id = "U1", .part = "NE555"},
      ccad::Component{.id = "R1", .part = "10k"}
  };

  ccad::Board board;
  
  // U1 has two pads on Top (F.Cu)
  board.pads.push_back(ccad::Pad{
      .id = "pad1",
      .component_id = "U1",
      .pin_name = "1",
      .layer_id = "F.Cu",
      .position = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(10)}
  });
  board.pads.push_back(ccad::Pad{
      .id = "pad2",
      .component_id = "U1",
      .pin_name = "2",
      .layer_id = "F.Cu",
      .position = ccad::Point{.x = ccad::millimeters(30), .y = ccad::millimeters(30)}
  });

  // R1 has one pad on Bottom (B.Cu)
  board.pads.push_back(ccad::Pad{
      .id = "pad3",
      .component_id = "R1",
      .pin_name = "1",
      .layer_id = "B.Cu",
      .position = ccad::Point{.x = ccad::millimeters(50), .y = ccad::millimeters(50)}
  });

  project.board = board;

  std::string exported = ccad::exportToPnpCsv(project);
  std::cout << exported << "\n";

  // Assertions to verify correct content in output
  require(exported.find("Designator,Value,PosX,PosY,Side") != std::string::npos, "Output must contain CSV header");
  
  // R1 at 50,50 Bottom
  require(exported.find("R1,10k,50.0000,50.0000,Bottom") != std::string::npos, "Output must contain R1 with correct centroid and side");
  
  // U1 bounding box is (10,10) to (30,30), center is (20,20)
  require(exported.find("U1,NE555,20.0000,20.0000,Top") != std::string::npos, "Output must contain U1 with correct centroid and side");

  std::cout << "All PnP export tests passed!\n";
  return 0;
}
