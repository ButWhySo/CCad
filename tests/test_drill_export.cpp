#include "ccad_core/drill_export.hpp"
#include "test_support.hpp"

#include <iostream>
#include <string>

int main() {
  ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.schematics.push_back(ccad::Schematic{});
  project.id = "proj_123";

  ccad::Board board;
  
  // Pad with drill 1.0mm
  board.pads.push_back(ccad::Pad{
      .id = "pad1",
      .component_id = "U1",
      .pin_name = "1",
      .position = ccad::Point{.x = ccad::millimeters(25.4), .y = ccad::millimeters(50.8)},
      .padstack = ccad::Padstack{
         .drill = ccad::PadstackDrillProps{.size = ccad::millimeters(1.0)}
      }
  });

  // Via with drill 0.4mm
  board.vias.push_back(ccad::Via{
      .id = "via1",
      .position = ccad::Point{.x = ccad::millimeters(12.7), .y = ccad::millimeters(25.4)},
      .diameter = ccad::millimeters(0.8),
      .drill = ccad::millimeters(0.4),
      .start_layer_id = "F.Cu",
      .end_layer_id = "In1.Cu",
      .via_type = "blind"
  });
  
  // Pad without drill (SMD) should be ignored
  board.pads.push_back(ccad::Pad{
      .id = "pad2",
      .component_id = "U1",
      .pin_name = "2",
      .position = ccad::Point{.x = ccad::millimeters(0.0), .y = ccad::millimeters(0.0)}
  });

  project.boards.clear(); project.boards.push_back(board);

  std::string exported = ccad::exportToDrillExcellon(project);
  std::cout << exported << "\n";

  // Assertions
  require(exported.find("M48") != std::string::npos, "Output must contain M48 header");
  require(exported.find("INCH") != std::string::npos, "Output must use INCH mode");
  
  // 1.0mm is ~0.039 inch, 0.4mm is ~0.016 inch
  require(exported.find("C0.039") != std::string::npos, "Output must declare tool for 1.0mm");
  require(exported.find("C0.016") != std::string::npos, "Output must declare tool for 0.4mm");

  // 25.4mm = 1 inch -> 10000 format
  // 50.8mm = 2 inch -> 20000 format
  require(exported.find("X010000Y020000") != std::string::npos, "Output must contain coordinates for pad");

  // 12.7mm = 0.5 inch -> 05000 format
  // 25.4mm = 1 inch -> 10000 format
  require(exported.find("X005000Y010000") != std::string::npos, "Output must contain coordinates for via");
  require(exported.find("; CCAD_VIA id=via1 type=blind start=F.Cu stop=In1.Cu") != std::string::npos,
          "Output must preserve non-through via metadata comment");
  
  // SMD pad should not be in the output
  require(exported.find("X000000Y000000") == std::string::npos, "Output must not contain SMD pad");

  std::cout << "All drill export tests passed!\n";
  return 0;
}
