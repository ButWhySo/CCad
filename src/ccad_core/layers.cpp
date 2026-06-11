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
      Layer{.id = "Dwgs.User", .name = "User drawings", .kind = "user", .visible = false});
  layers.push_back(
      Layer{.id = "Cmts.User", .name = "User comments", .kind = "user", .visible = false});
  layers.push_back(
      Layer{.id = "Eco1.User", .name = "Engineering change order 1", .kind = "user", .visible = false});
  layers.push_back(
      Layer{.id = "Eco2.User", .name = "Engineering change order 2", .kind = "user", .visible = false});
  layers.push_back(
      Layer{.id = "Edge.Cuts", .name = "Board outline", .kind = "board_edge", .visible = true});
  layers.push_back(Layer{.id = "Margin", .name = "Board margin", .kind = "margin", .visible = false});
  layers.push_back(
      Layer{.id = "B.CrtYd", .name = "Back courtyard", .kind = "courtyard", .visible = false});
  layers.push_back(
      Layer{.id = "F.CrtYd", .name = "Front courtyard", .kind = "courtyard", .visible = false});
  layers.push_back(
      Layer{.id = "B.Fab", .name = "Back fabrication", .kind = "fabrication", .visible = false});
  layers.push_back(Layer{
      .id = "F.Fab", .name = "Front fabrication", .kind = "fabrication", .visible = false});
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

bool hasSuffix(const std::string& value, const std::string& suffix) {
  return value.size() >= suffix.size() &&
         value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool layerMatchesKiCadSelector(const Layer& layer, const std::string& selector) {
  if (!selector.starts_with("*.")) {
    return layer.id == selector;
  }

  const std::string suffix = selector.substr(1);
  if (hasSuffix(layer.id, suffix)) {
    return true;
  }

  if (selector == "*.Cu") return layer.kind == "copper";
  if (selector == "*.Adhes") return layer.kind == "adhesive";
  if (selector == "*.Paste") return layer.kind == "paste";
  if (selector == "*.SilkS") return layer.kind == "silkscreen";
  if (selector == "*.Mask") return layer.kind == "mask";
  if (selector == "*.CrtYd") return layer.kind == "courtyard";
  if (selector == "*.Fab") return layer.kind == "fabrication";
  return false;
}

void appendUniqueLayerId(std::vector<std::string>& output, const std::string& layer_id) {
  if (std::find(output.begin(), output.end(), layer_id) == output.end()) {
    output.push_back(layer_id);
  }
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

std::optional<std::size_t> standardKiCadPcbLayerNumber(const std::string& id) {
  const std::vector<Layer>& layers = standardKiCadPcbLayerStorage();
  const auto it = std::find_if(layers.begin(), layers.end(),
                               [&id](const Layer& layer) { return layer.id == id; });
  if (it == layers.end()) {
    return std::nullopt;
  }
  return static_cast<std::size_t>(std::distance(layers.begin(), it));
}

std::vector<std::string> expandKiCadLayerSet(const std::vector<std::string>& layer_selectors,
                                             const Board& board) {
  std::vector<std::string> expanded;
  for (const std::string& selector : layer_selectors) {
    bool matched = false;
    for (const Layer& layer : board.layers) {
      if (layerMatchesKiCadSelector(layer, selector)) {
        appendUniqueLayerId(expanded, layer.id);
        matched = true;
      }
    }
    if (!matched) {
      appendUniqueLayerId(expanded, selector);
    }
  }
  return expanded;
}

std::vector<std::size_t> standardKiCadPcbLayerNumbersForSet(
    const std::vector<std::string>& layer_ids) {
  std::vector<std::size_t> layer_numbers;
  for (const std::string& layer_id : layer_ids) {
    const std::optional<std::size_t> layer_number = standardKiCadPcbLayerNumber(layer_id);
    if (layer_number.has_value() &&
        std::find(layer_numbers.begin(), layer_numbers.end(), *layer_number) ==
            layer_numbers.end()) {
      layer_numbers.push_back(*layer_number);
    }
  }
  return layer_numbers;
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
