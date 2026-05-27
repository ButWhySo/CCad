#include "ccad_core/layers.hpp"

#include <algorithm>

namespace ccad {
namespace {

std::vector<Layer> buildStandardKiCadPcbLayers() {
  std::vector<Layer> layers;
  layers.reserve(59);

  layers.push_back(Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true});
  for (int index = 1; index <= 30; ++index) {
    layers.push_back(Layer{.id = "In" + std::to_string(index) + ".Cu",
                           .name = "Inner " + std::to_string(index) + " copper",
                           .kind = "copper",
                           .visible = false});
  }
  layers.push_back(Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true});

  layers.push_back(
      Layer{.id = "B.Adhes", .name = "Back adhesive", .kind = "adhesive", .visible = false});
  layers.push_back(
      Layer{.id = "F.Adhes", .name = "Front adhesive", .kind = "adhesive", .visible = false});
  layers.push_back(
      Layer{.id = "B.Paste", .name = "Back solder paste", .kind = "paste", .visible = true});
  layers.push_back(
      Layer{.id = "F.Paste", .name = "Front solder paste", .kind = "paste", .visible = true});
  layers.push_back(
      Layer{.id = "B.SilkS", .name = "Back silkscreen", .kind = "silkscreen", .visible = true});
  layers.push_back(
      Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen", .visible = true});
  layers.push_back(
      Layer{.id = "B.Mask", .name = "Back solder mask", .kind = "mask", .visible = true});
  layers.push_back(
      Layer{.id = "F.Mask", .name = "Front solder mask", .kind = "mask", .visible = true});
  layers.push_back(
      Layer{.id = "F.CrtYd", .name = "Front courtyard", .kind = "courtyard", .visible = false});
  layers.push_back(
      Layer{.id = "B.CrtYd", .name = "Back courtyard", .kind = "courtyard", .visible = false});
  layers.push_back(Layer{
      .id = "F.Fab", .name = "Front fabrication", .kind = "fabrication", .visible = false});
  layers.push_back(
      Layer{.id = "B.Fab", .name = "Back fabrication", .kind = "fabrication", .visible = false});

  layers.push_back(
      Layer{.id = "Edge.Cuts", .name = "Board outline", .kind = "board_edge", .visible = true});
  layers.push_back(Layer{.id = "Margin", .name = "Board margin", .kind = "margin", .visible = false});

  layers.push_back(
      Layer{.id = "Dwgs.User", .name = "User drawings", .kind = "user", .visible = false});
  layers.push_back(
      Layer{.id = "Cmts.User", .name = "User comments", .kind = "user", .visible = false});
  layers.push_back(
      Layer{.id = "Eco1.User", .name = "Engineering change order 1", .kind = "user", .visible = false});
  layers.push_back(
      Layer{.id = "Eco2.User", .name = "Engineering change order 2", .kind = "user", .visible = false});
  for (int index = 1; index <= 9; ++index) {
    layers.push_back(Layer{.id = "User." + std::to_string(index),
                           .name = "User " + std::to_string(index),
                           .kind = "user",
                           .visible = false});
  }
  return layers;
}

const std::vector<Layer>& standardKiCadPcbLayerStorage() {
  static const std::vector<Layer> layers = buildStandardKiCadPcbLayers();
  return layers;
}

bool hasLayerId(const Board& board, const std::string& id) {
  return std::any_of(board.layers.begin(), board.layers.end(),
                     [&id](const Layer& layer) { return layer.id == id; });
}

}  // namespace

std::vector<Layer> standardKiCadPcbLayers() {
  return standardKiCadPcbLayerStorage();
}

const Layer* findStandardKiCadPcbLayer(const std::string& id) {
  const std::vector<Layer>& layers = standardKiCadPcbLayerStorage();
  const auto it = std::find_if(layers.begin(), layers.end(),
                               [&id](const Layer& layer) { return layer.id == id; });
  return it == layers.end() ? nullptr : &*it;
}

std::size_t appendMissingStandardKiCadPcbLayers(Board& board) {
  std::size_t added = 0;
  for (const Layer& standard_layer : standardKiCadPcbLayerStorage()) {
    if (!hasLayerId(board, standard_layer.id)) {
      board.layers.push_back(standard_layer);
      ++added;
    }
  }
  return added;
}

}  // namespace ccad
