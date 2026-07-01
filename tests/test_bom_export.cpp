#include "ccad_core/bom_export.hpp"
#include "test_support.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

int main() {
  ccad::Project project;
  project.id = "proj_123";
  project.name = "Test Project";

  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].symbols = {
      ccad::SchSymbol{.id = "U1", .lib_id = "NE555"},
      ccad::SchSymbol{.id = "R1", .lib_id = "10k"},
      ccad::SchSymbol{.id = "C1", .lib_id = "100nF"}
  };

  std::string exported = ccad::exportToBomCsv(project);
  std::cout << exported << "\n";

  // Assertions to verify correct content in output
  require(exported.find("Designator,Part") != std::string::npos, "Output must contain CSV header");
  require(exported.find("C1,100nF") != std::string::npos, "Output must contain C1");
  require(exported.find("R1,10k") != std::string::npos, "Output must contain R1");
  require(exported.find("U1,NE555") != std::string::npos, "Output must contain U1");

  ccad::Project board_only;
  board_only.id = "proj_board_only";
  board_only.name = "Board Only";
  const std::string board_only_exported = ccad::exportToBomCsv(board_only);
  require(board_only_exported == "Designator,Part\n",
          "BOM export emits only the header when no schematic exists");

  ccad::Project board_project;
  board_project.id = "proj_board_bom";
  board_project.name = "Board BOM";
  ccad::Board board;
  board.footprints = {
      ccad::BoardFootprint{.reference = "C10",
                           .value = "47pF",
                           .footprint_name = "C_0603"},
      ccad::BoardFootprint{.reference = "U1",
                           .value = "ATmega328P",
                           .footprint_name = "TQFP-32"},
      ccad::BoardFootprint{.reference = "FID1",
                           .value = "Fiducial",
                           .footprint_name = "Fiducial_1mm",
                           .exclude_from_bom = true},
      ccad::BoardFootprint{.reference = "C2",
                           .value = "47pF",
                           .footprint_name = "C_0603"},
      ccad::BoardFootprint{.reference = "R1",
                           .value = "10k",
                           .footprint_name = "R_0805"}
  };
  board_project.boards.push_back(board);

  const std::string board_bom = ccad::exportBoardToBomCsv(board_project);
  const std::string expected_board_bom =
      "\"Id\";\"Designator\";\"Footprint\";\"Quantity\";\"Designation\";\"Supplier and ref\";\n"
      "1;\"C2, C10\";\"C_0603\";2;\"47pF\";;;\n"
      "2;\"R1\";\"R_0805\";1;\"10k\";;;\n"
      "3;\"U1\";\"TQFP-32\";1;\"ATmega328P\";;;\n";
  require(board_bom == expected_board_bom,
          "board BOM groups by value and footprint, skips excluded footprints, and naturally sorts references");

  try {
    (void) ccad::exportBoardToBomCsv(ccad::Project{});
    require(false, "board BOM rejects projects with no board footprints");
  } catch (const std::runtime_error& error) {
    require(std::string(error.what()).find("there are no footprints") != std::string::npos,
            "board BOM reports KiCad-compatible empty board error");
  }

  std::cout << "All BOM export tests passed!\n";
  return 0;
}
