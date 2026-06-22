#include <iostream>
#include <cassert>
#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/json.hpp"

using namespace ccad;

void test_parse_barcode() {
  const std::string json_str = R"({
    "id": "barcode_1",
    "layer_id": "F_Cu",
    "text": "CCad_Native",
    "kind": "QRCode",
    "error_correction": "Medium",
    "position": {"x_nm": 100000000, "y_nm": 200000000},
    "rotation_degrees": 45.0,
    "size": {"width_nm": 10000000, "height_nm": 10000000},
    "margin": {"width_nm": 1000000, "height_nm": 1000000},
    "locked": true
  })";

  // create a dummy board wrapper to test serialization
  const std::string board_json = R"({
    "board": {
      "design_rules": {},
      "barcodes": [)" + json_str + R"(]
    }
  })";
  std::cout << board_json << std::endl;

  Project project = loadProjectJson(board_json);
  Board board = project.boards[0];
  assert(board.barcodes.size() == 1);

  const BoardBarcode& bc = board.barcodes[0];
  assert(bc.id == "barcode_1");
  assert(bc.layer_id == "F_Cu");
  assert(bc.text == "CCad_Native");
  assert(bc.kind == BarcodeType::QRCode);
  assert(bc.error_correction == BarcodeEcc::Medium);
  assert(bc.position.x.nanometers == 100000000);
  assert(bc.position.y.nanometers == 200000000);
  assert(bc.rotation_degrees == 45.0);
  assert(bc.size.width.nanometers == 10000000);
  assert(bc.size.height.nanometers == 10000000);
  assert(bc.margin.width.nanometers == 1000000);
  assert(bc.margin.height.nanometers == 1000000);
  assert(bc.locked == true);
  
  std::cout << "test_parse_barcode passed\n";
}

int main() {
  test_parse_barcode();
  std::cout << "All barcode tests passed\n";
  return 0;
}
