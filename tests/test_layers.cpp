#include "ccad_core/layers.hpp"
#include "test_support.hpp"

#include <string>
#include <vector>

int main() {
  const std::vector<ccad::Layer> layers = ccad::standardKiCadPcbLayers();
  require(layers.size() == 59, "KiCad canonical PCB layer registry has 59 named layers");

  require(layers.front().id == "F.Cu", "front copper is first layer");
  require(layers.back().id == "User.9", "User.9 is final layer");
  require(layers.at(40).id == "Dwgs.User", "Dwgs.User uses KiCad canonical index 40");
  require(layers.at(44).id == "Edge.Cuts", "Edge.Cuts uses KiCad canonical index 44");
  require(layers.at(45).id == "Margin", "Margin uses KiCad canonical index 45");
  require(layers.at(46).id == "B.CrtYd", "B.CrtYd uses KiCad canonical index 46");
  require(layers.at(47).id == "F.CrtYd", "F.CrtYd uses KiCad canonical index 47");
  require(layers.at(58).id == "User.9", "User.9 uses KiCad canonical index 58");
  require(ccad::standardKiCadPcbLayerNumber("F.Cu").has_value(),
          "canonical layer number exists for F.Cu");
  require(*ccad::standardKiCadPcbLayerNumber("F.Cu") == 0,
          "F.Cu canonical layer number is 0");
  require(*ccad::standardKiCadPcbLayerNumber("Edge.Cuts") == 44,
          "Edge.Cuts canonical layer number is 44");
  require(*ccad::standardKiCadPcbLayerNumber("User.9") == 58,
          "User.9 canonical layer number is 58");
  require(!ccad::standardKiCadPcbLayerNumber("Custom.Mechanical").has_value(),
          "custom layers do not claim KiCad canonical numbers");
  require(ccad::findStandardKiCadPcbLayer("In30.Cu") != nullptr, "registry includes In30.Cu");
  require(ccad::findStandardKiCadPcbLayer("Edge.Cuts") != nullptr,
          "registry includes Edge.Cuts");
  require(ccad::findStandardKiCadPcbLayer("F.Mask") != nullptr, "registry includes F.Mask");

  const ccad::Layer* front_copper = ccad::findStandardKiCadPcbLayer("F.Cu");
  const ccad::Layer* inner_copper = ccad::findStandardKiCadPcbLayer("In1.Cu");
  const ccad::Layer* edge_cuts = ccad::findStandardKiCadPcbLayer("Edge.Cuts");
  const ccad::Layer* front_paste = ccad::findStandardKiCadPcbLayer("F.Paste");
  const ccad::Layer* user_layer = ccad::findStandardKiCadPcbLayer("User.9");
  require(front_copper != nullptr && front_copper->kind == "copper",
          "F.Cu is modeled as copper");
  require(inner_copper != nullptr && inner_copper->kind == "copper",
          "inner copper is modeled as copper");
  require(edge_cuts != nullptr && edge_cuts->kind == "board_edge",
          "Edge.Cuts is modeled as board edge geometry");
  require(front_paste != nullptr && front_paste->kind == "paste",
          "F.Paste is modeled as solder paste");
  require(user_layer != nullptr && user_layer->kind == "user",
          "User.9 is modeled as user data");

  ccad::Board board;
  board.layers = {
      ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
      ccad::Layer{.id = "Custom.Mechanical",
                  .name = "Custom mechanical",
                  .kind = "user",
                  .visible = true},
  };
  const std::size_t added = ccad::appendMissingStandardKiCadPcbLayers(board);
  require(added == 58, "append adds missing standard layers while preserving existing F.Cu");
  require(board.layers.size() == 60, "append preserves custom layers");
  require(board.layers.at(0).id == "F.Cu", "append preserves existing layer order");
  require(board.layers.at(1).id == "Custom.Mechanical", "append preserves custom layer order");
  require(board.layers.at(2).id == "In1.Cu", "append starts missing canonical layers after custom");
  require(ccad::appendMissingStandardKiCadPcbLayers(board) == 0, "append is idempotent");
}
