#include "ccad_core/pnp_export.hpp"
#include "test_support.hpp"

#include <iostream>
#include <string>

int main() {
  ccad::Project project;
  project.id = "proj_123";
  project.name = "Test Project";

  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].symbols = {
      ccad::SchSymbol{.id = "U1", .lib_id = "NE555"},
      ccad::SchSymbol{.id = "R1", .lib_id = "10k"}
  };

  ccad::Board board;
  
  // U1 has two pads on Top (F.Cu)
  board.pads.push_back(ccad::Pad{
      .id = "pad1",
      .component_id = "U1",
      .pin_name = "1",
      .type = "smd",
      .position = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(10)},
      .padstack = ccad::Padstack{
        .layer_set = {"F.Cu"},
        .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
      }
  });
  board.pads.push_back(ccad::Pad{
      .id = "pad2",
      .component_id = "U1",
      .pin_name = "2",
      .type = "smd",
      .position = ccad::Point{.x = ccad::millimeters(30), .y = ccad::millimeters(30)},
      .padstack = ccad::Padstack{
        .layer_set = {"F.Cu"},
        .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
      }
  });

  // R1 has one pad on Bottom (B.Cu)
  board.pads.push_back(ccad::Pad{
      .id = "pad3",
      .component_id = "R1",
      .pin_name = "1",
      .type = "smd",
      .position = ccad::Point{.x = ccad::millimeters(50), .y = ccad::millimeters(50)},
      .padstack = ccad::Padstack{
        .layer_set = {"B.Cu"},
        .copper_props = {{"bottom", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
      }
  });

  project.boards.clear(); project.boards.push_back(board);

  std::string exported = ccad::exportToPnpCsv(project);
  std::cout << exported << "\n";

  // Assertions to verify correct content in output
  require(exported.find("Designator,Value,PosX,PosY,Side") != std::string::npos, "Output must contain CSV header");
  
  // R1 at 50,50 Bottom
  require(exported.find("R1,10k,50.0000,50.0000,Bottom") != std::string::npos, "Output must contain R1 with correct centroid and side");
  
  // U1 bounding box is (10,10) to (30,30), center is (20,20)
  require(exported.find("U1,NE555,20.0000,20.0000,Top") != std::string::npos, "Output must contain U1 with correct centroid and side");

  ccad::Project board_only;
  board_only.id = "proj_board_only";
  board_only.name = "Board Only";
  ccad::Board board_only_board;
  board_only_board.pads.push_back(ccad::Pad{
      .id = "pad_board_only",
      .component_id = "J1",
      .pin_name = "1",
      .type = "smd",
      .position = ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(14)},
      .padstack = ccad::Padstack{
        .layer_set = {"F.Cu"},
        .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.0)}}}}}
      }
  });
  board_only.boards.push_back(board_only_board);
  const std::string board_only_exported = ccad::exportToPnpCsv(board_only);
  require(board_only_exported.find("J1,,12.0000,14.0000,Top") != std::string::npos,
          "PnP export emits board-only component placements with blank schematic value");

  std::cout << "All PnP export tests passed!\n";
  return 0;
}
