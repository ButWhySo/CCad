#include "ccad_core/pad_utils.hpp"
#include <iostream>
#include <cassert>

using namespace ccad;

void testPadUtils() {
  Board board;
  
  Layer l1; l1.id = "F.Cu"; l1.kind = "copper";
  Layer l2; l2.id = "In1.Cu"; l2.kind = "copper";
  Layer l3; l3.id = "B.Cu"; l3.kind = "copper";
  board.layers = {l1, l2, l3};
  board.design_rules.board_thickness = millimeters(1.6); // 1600000 nm

  Pad pad;
  pad.id = "pad1";
  
  // Test countersink from top
  pad.padstack.front_post_machining.mode = "countersink";
  pad.padstack.front_post_machining.size = millimeters(2.0); // 2000000 nm
  pad.padstack.front_post_machining.angle_degrees = 90.0;
  
  int64_t knockoutF = getPostMachiningKnockout(board, pad, "F.Cu");
  assert(knockoutF == 2000000);
  
  // At In1.Cu, distance is 0.8mm = 800000 nm
  // diameterAtLayer = 2000000 - 2 * 800000 * tan(45) = 2000000 - 1600000 = 400000
  int64_t knockoutIn1 = getPostMachiningKnockout(board, pad, "In1.Cu");
  std::cout << "knockoutIn1: " << knockoutIn1 << "\n";
  assert(knockoutIn1 == 400000);
  
  int64_t knockoutB = getPostMachiningKnockout(board, pad, "B.Cu");
  assert(knockoutB == 0);
  
  // Test backdrill
  pad.padstack.secondary_drill = PadstackDrillProps();
  pad.padstack.secondary_drill->size = Size{millimeters(1.0), millimeters(1.0)};
  pad.padstack.secondary_drill->start_layer = "B.Cu";
  pad.padstack.secondary_drill->end_layer = "In1.Cu";
  
  assert(isBackdrilledOrPostMachined(board, pad, "B.Cu"));
  assert(isBackdrilledOrPostMachined(board, pad, "In1.Cu"));
  assert(isBackdrilledOrPostMachined(board, pad, "F.Cu")); // Due to countersink from front!
  
  std::cout << "testPadUtils passed.\n";
}

int main() {
  testPadUtils();
  return 0;
}
