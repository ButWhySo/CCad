#include "ccad_core/bom_export.hpp"
#include "test_support.hpp"

#include <iostream>
#include <string>

int main() {
  ccad::Project project;
  project.id = "proj_123";
  project.name = "Test Project";

  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].components = {
      ccad::Component{.id = "U1", .part = "NE555"},
      ccad::Component{.id = "R1", .part = "10k"},
      ccad::Component{.id = "C1", .part = "100nF"}
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

  std::cout << "All BOM export tests passed!\n";
  return 0;
}
