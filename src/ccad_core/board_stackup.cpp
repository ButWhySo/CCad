#include "ccad_core/board_stackup.hpp"

#include "ccad_core/layers.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace ccad {
namespace {

constexpr std::int64_t kCopperDefaultThicknessNm = 35000;
constexpr std::int64_t kMaskDefaultThicknessNm = 10000;

bool isCopperLayer(const Layer& layer) {
  return layer.kind == "copper";
}

bool isCopperLayerId(const std::string& layer_id) {
  return layer_id == "F.Cu" || layer_id == "B.Cu" ||
         (layer_id.size() > 5 && layer_id.starts_with("In") && layer_id.ends_with(".Cu"));
}

bool isExternalCopperLayer(const std::string& layer_id) {
  return layer_id == "F.Cu" || layer_id == "B.Cu";
}

int innerLayerNumber(const std::string& layer_id) {
  if (!(layer_id.size() > 5 && layer_id.starts_with("In") && layer_id.ends_with(".Cu"))) {
    return 1000;
  }
  int value = 0;
  for (std::size_t i = 2; i + 3 < layer_id.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(layer_id.at(i)))) {
      return 1000;
    }
    value = value * 10 + (layer_id.at(i) - '0');
  }
  return value;
}

int copperSortKey(const Layer& layer, std::size_t original_index) {
  if (layer.id == "F.Cu") return 0;
  if (layer.id == "B.Cu") return 10000;
  if (layer.id.starts_with("In") && layer.id.ends_with(".Cu")) {
    return 100 + innerLayerNumber(layer.id);
  }
  if (const std::optional<std::size_t> number = standardKiCadPcbLayerNumber(layer.id)) {
    return static_cast<int>(*number);
  }
  return 20000 + static_cast<int>(original_index);
}

std::vector<const Layer*> orderedCopperLayers(const Board& board) {
  std::vector<std::pair<int, const Layer*>> keyed;
  for (std::size_t i = 0; i < board.layers.size(); ++i) {
    const Layer& layer = board.layers.at(i);
    if (isCopperLayer(layer)) {
      keyed.push_back({copperSortKey(layer, i), &layer});
    }
  }
  std::sort(keyed.begin(), keyed.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

  std::vector<const Layer*> layers;
  layers.reserve(keyed.size());
  for (const auto& entry : keyed) {
    layers.push_back(entry.second);
  }
  return layers;
}

const Layer* findLayer(const Board& board, const std::string& layer_id) {
  const auto it = std::find_if(board.layers.begin(), board.layers.end(),
                               [&layer_id](const Layer& layer) { return layer.id == layer_id; });
  return it == board.layers.end() ? nullptr : &*it;
}

BoardStackupItem technicalLayerItem(BoardStackupItemType type,
                                    const Layer& layer,
                                    const std::string& type_name) {
  BoardStackupItem item;
  item.type = type;
  item.layer_id = layer.id;
  item.layer_name = layer.name;
  item.type_name = type_name;
  if (type == BoardStackupItemType::solder_mask) {
    item.thickness = defaultSolderMaskThickness();
    item.epsilon_r = 3.3;
  }
  return item;
}

BoardStackupItem copperItem(const Layer& layer) {
  BoardStackupItem item;
  item.type = BoardStackupItemType::copper;
  item.layer_id = layer.id;
  item.layer_name = layer.name;
  item.type_name = "copper";
  item.thickness = defaultCopperThickness();
  item.material = "copper";
  return item;
}

BoardStackupItem dielectricItem(int dielectric_index, Length thickness) {
  BoardStackupItem item;
  item.type = BoardStackupItemType::dielectric;
  item.layer_name = "Dielectric " + std::to_string(dielectric_index);
  item.type_name = (dielectric_index % 2 == 1) ? "core" : "prepreg";
  item.dielectric_layer_id = dielectric_index;
  item.thickness = thickness;
  item.material = "FR4";
  item.epsilon_r = 4.5;
  item.loss_tangent = 0.02;
  item.spec_frequency_hz = 1000000000.0;
  item.dielectric_model = "constant";
  return item;
}

bool contributesToThickness(BoardStackupItemType type) {
  return type == BoardStackupItemType::copper || type == BoardStackupItemType::dielectric ||
         type == BoardStackupItemType::solder_mask;
}

int layerItemIndex(const BoardStackup& stackup, const std::string& layer_id) {
  for (std::size_t i = 0; i < stackup.items.size(); ++i) {
    if (stackup.items.at(i).layer_id == layer_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace

Length defaultCopperThickness() {
  return nanometers(kCopperDefaultThicknessNm);
}

Length defaultSolderMaskThickness() {
  return nanometers(kMaskDefaultThicknessNm);
}

BoardStackup buildDefaultBoardStackup(const Board& board) {
  BoardStackup stackup;
  const std::vector<const Layer*> copper_layers = orderedCopperLayers(board);
  const int copper_count = static_cast<int>(copper_layers.size());
  const int active_copper_count = std::max(1, copper_count);

  int mask_count = 0;
  if (findLayer(board, "F.Mask") != nullptr) ++mask_count;
  if (findLayer(board, "B.Mask") != nullptr) ++mask_count;

  std::int64_t dielectric_thickness_nm =
      board.design_rules.board_thickness.nanometers -
      (defaultCopperThickness().nanometers * active_copper_count) -
      (defaultSolderMaskThickness().nanometers * mask_count);
  dielectric_thickness_nm /= std::max(1, active_copper_count - 1);

  if (const Layer* layer = findLayer(board, "F.SilkS")) {
    stackup.items.push_back(
        technicalLayerItem(BoardStackupItemType::silkscreen, *layer, "Top Silk Screen"));
  }
  if (const Layer* layer = findLayer(board, "F.Paste")) {
    stackup.items.push_back(
        technicalLayerItem(BoardStackupItemType::solder_paste, *layer, "Top Solder Paste"));
  }
  if (const Layer* layer = findLayer(board, "F.Mask")) {
    stackup.items.push_back(
        technicalLayerItem(BoardStackupItemType::solder_mask, *layer, "Top Solder Mask"));
  }

  int dielectric_index = 1;
  for (std::size_t i = 0; i < copper_layers.size(); ++i) {
    stackup.items.push_back(copperItem(*copper_layers.at(i)));
    if (i + 1 < copper_layers.size()) {
      stackup.items.push_back(dielectricItem(dielectric_index, nanometers(dielectric_thickness_nm)));
      ++dielectric_index;
    }
  }

  if (const Layer* layer = findLayer(board, "B.Mask")) {
    stackup.items.push_back(
        technicalLayerItem(BoardStackupItemType::solder_mask, *layer, "Bottom Solder Mask"));
  }
  if (const Layer* layer = findLayer(board, "B.Paste")) {
    stackup.items.push_back(
        technicalLayerItem(BoardStackupItemType::solder_paste, *layer, "Bottom Solder Paste"));
  }
  if (const Layer* layer = findLayer(board, "B.SilkS")) {
    stackup.items.push_back(
        technicalLayerItem(BoardStackupItemType::silkscreen, *layer, "Bottom Silk Screen"));
  }

  return stackup;
}

Length buildBoardThicknessFromStackup(const BoardStackup& stackup) {
  std::int64_t total = 0;
  for (const BoardStackupItem& item : stackup.items) {
    if (item.enabled && contributesToThickness(item.type)) {
      total += item.thickness.nanometers;
    }
  }
  return nanometers(total);
}

Length boardStackupLayerDistance(const BoardStackup& stackup,
                                 const std::string& first_layer_id,
                                 const std::string& second_layer_id) {
  if (first_layer_id == second_layer_id) {
    return nanometers(0);
  }
  if (!isCopperLayerId(first_layer_id) || !isCopperLayerId(second_layer_id)) {
    throw std::runtime_error("layer distance requires copper layer ids");
  }

  int first_index = layerItemIndex(stackup, first_layer_id);
  int second_index = layerItemIndex(stackup, second_layer_id);
  if (first_index < 0) throw std::runtime_error("unknown stackup layer: " + first_layer_id);
  if (second_index < 0) throw std::runtime_error("unknown stackup layer: " + second_layer_id);
  if (second_index < first_index) {
    std::swap(first_index, second_index);
  }

  std::int64_t total = 0;
  bool start = false;
  bool half = false;
  for (int i = first_index; i <= second_index; ++i) {
    const BoardStackupItem& item = stackup.items.at(static_cast<std::size_t>(i));
    const bool copper = item.type == BoardStackupItemType::copper;

    if (!start && copper && i == first_index) {
      start = true;
      if (!isExternalCopperLayer(item.layer_id)) {
        half = true;
      }
    } else if (!start) {
      continue;
    }

    if (start && copper && i == second_index && !isExternalCopperLayer(item.layer_id)) {
      half = true;
    }

    if (copper || item.type == BoardStackupItemType::dielectric) {
      total += half ? (item.thickness.nanometers / 2) : item.thickness.nanometers;
    }

    half = false;
  }

  return nanometers(total);
}

std::string boardStackupItemTypeName(BoardStackupItemType type) {
  switch (type) {
    case BoardStackupItemType::copper:
      return "copper";
    case BoardStackupItemType::dielectric:
      return "dielectric";
    case BoardStackupItemType::solder_paste:
      return "solderpaste";
    case BoardStackupItemType::solder_mask:
      return "soldermask";
    case BoardStackupItemType::silkscreen:
      return "silkscreen";
    case BoardStackupItemType::undefined:
      return "undefined";
  }
  return "undefined";
}

}  // namespace ccad
