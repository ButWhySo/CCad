#include "ccad_core/board_stackup.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

ccad::Board makeThreeCopperBoard() {
  ccad::Board board;
  board.design_rules.board_thickness = ccad::millimeters(1.6);
  board.layers = {
      {.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen", .visible = true},
      {.id = "F.Paste", .name = "Front solder paste", .kind = "paste", .visible = true},
      {.id = "F.Mask", .name = "Front solder mask", .kind = "mask", .visible = true},
      {.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
      {.id = "In1.Cu", .name = "Inner 1 copper", .kind = "copper", .visible = true},
      {.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true},
      {.id = "B.Mask", .name = "Back solder mask", .kind = "mask", .visible = true},
      {.id = "B.Paste", .name = "Back solder paste", .kind = "paste", .visible = true},
      {.id = "B.SilkS", .name = "Back silkscreen", .kind = "silkscreen", .visible = true},
  };
  return board;
}

void test_default_stackup_matches_kicad_item_order() {
  const ccad::Board board = makeThreeCopperBoard();
  const ccad::BoardStackup stackup = ccad::buildDefaultBoardStackup(board);

  require(stackup.kicad_class == "BOARD_STACKUP", "stackup reports KiCad class");
  require(stackup.parity_scope == "default_stackup_first_slice",
          "stackup declares first-slice parity scope");
  require(stackup.items.size() == 11, "stackup includes silk, paste, mask, copper, dielectric rows");

  require(stackup.items.at(0).type == ccad::BoardStackupItemType::silkscreen,
          "first stackup item is top silkscreen");
  require(stackup.items.at(0).layer_id == "F.SilkS", "top silkscreen layer id is preserved");
  require(stackup.items.at(2).type == ccad::BoardStackupItemType::solder_mask,
          "front mask is before front copper");
  require(stackup.items.at(3).type == ccad::BoardStackupItemType::copper,
          "front copper follows front mask");
  require(stackup.items.at(3).layer_id == "F.Cu", "front copper layer id is preserved");
  require(stackup.items.at(4).type == ccad::BoardStackupItemType::dielectric,
          "first dielectric follows front copper");
  require(stackup.items.at(4).dielectric_layer_id == 1, "first dielectric id is one-based");
  require(stackup.items.at(4).type_name == "core", "first dielectric defaults to core");
  require(stackup.items.at(4).material == "FR4", "first dielectric defaults to FR4");
  require(stackup.items.at(4).epsilon_r == 4.5, "FR4 epsilon defaults to KiCad value");
  require(stackup.items.at(4).loss_tangent == 0.02, "FR4 loss tangent defaults to KiCad value");
  require(stackup.items.at(5).layer_id == "In1.Cu", "inner copper is retained");
  require(stackup.items.at(6).type_name == "prepreg", "second dielectric alternates to prepreg");
  require(stackup.items.at(8).type == ccad::BoardStackupItemType::solder_mask,
          "bottom mask follows bottom copper");
  require(stackup.items.at(10).layer_id == "B.SilkS", "bottom silkscreen is last");
}

void test_default_stackup_derives_thickness_and_layer_distance() {
  const ccad::Board board = makeThreeCopperBoard();
  const ccad::BoardStackup stackup = ccad::buildDefaultBoardStackup(board);

  require(stackup.items.at(3).thickness.nanometers == 35000,
          "copper default thickness matches KiCad 0.035 mm");
  require(stackup.items.at(2).thickness.nanometers == 10000,
          "solder mask default thickness matches KiCad 0.01 mm");
  require(stackup.items.at(4).thickness.nanometers == 737500,
          "dielectric thickness distributes remaining board thickness");
  require(ccad::buildBoardThicknessFromStackup(stackup).nanometers == 1600000,
          "stackup thickness sums to board design-rule thickness");
  require(ccad::boardStackupLayerDistance(stackup, "F.Cu", "B.Cu").nanometers == 1580000,
          "front-to-back copper distance follows KiCad stackup algorithm");
  require(ccad::boardStackupLayerDistance(stackup, "F.Cu", "In1.Cu").nanometers == 790000,
          "external-to-internal copper distance uses half internal copper");
}

}  // namespace

int main() {
  try {
    test_default_stackup_matches_kicad_item_order();
    test_default_stackup_derives_thickness_and_layer_distance();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
