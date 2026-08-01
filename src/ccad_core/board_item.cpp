#include "ccad_core/board_item.hpp"

#include "ccad_core/layers.hpp"

#include <algorithm>
#include <sstream>

namespace ccad {
namespace {

bool startsWith(const std::string& value, const std::string& prefix) {
  return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool isCopperLayerId(const Board& board, const std::string& layer_id) {
  for (const Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return layer.kind == "copper";
    }
  }
  const Layer* standard_layer = findStandardKiCadPcbLayer(layer_id);
  if (standard_layer != nullptr) {
    return standard_layer->kind == "copper";
  }
  return layer_id.size() >= 3 && layer_id.compare(layer_id.size() - 3, 3, ".Cu") == 0;
}

std::vector<std::string> copperLayerIds(const Board& board) {
  std::vector<std::string> layer_ids;
  for (const Layer& layer : board.layers) {
    if (layer.kind == "copper") {
      layer_ids.push_back(layer.id);
    }
  }
  return layer_ids;
}

bool containsAll(const std::vector<std::string>& candidate,
                 const std::vector<std::string>& required) {
  if (candidate.size() != required.size()) {
    return false;
  }
  return std::all_of(required.begin(), required.end(), [&candidate](const std::string& value) {
    return std::find(candidate.begin(), candidate.end(), value) != candidate.end();
  });
}

std::string describeLayerMask(const Board& board, const std::vector<std::string>& layer_ids) {
  if (layer_ids.empty()) {
    return "no layers";
  }
  if (containsAll(layer_ids, copperLayerIds(board))) {
    return "all copper layers";
  }
  std::ostringstream out;
  for (std::size_t i = 0; i < layer_ids.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << layer_ids.at(i);
  }
  return out.str();
}

bool isSideSpecific(const std::vector<std::string>& layer_ids) {
  return std::any_of(layer_ids.begin(), layer_ids.end(), [](const std::string& layer_id) {
    return startsWith(layer_id, "F.") || startsWith(layer_id, "B.");
  });
}

std::vector<std::string> viewLayersFor(const std::vector<std::string>& layer_ids,
                                       const bool locked) {
  std::vector<std::string> view_layers = layer_ids;
  if (locked) {
    view_layers.push_back("LAYER_LOCKED_ITEM_SHADOW");
  }
  return view_layers;
}

}  // namespace

BoardItemMetadata boardItemMetadata(const Board& board, const std::vector<std::string>& layer_ids,
                                    const bool groupable, const bool has_hole,
                                    const bool has_drilled_hole, const bool locked,
                                    const bool knockout) {
  BoardItemMetadata metadata;
  metadata.kicad_groupable = groupable;
  metadata.primary_layer_id = layer_ids.empty() ? "" : layer_ids.front();
  metadata.layer_ids = layer_ids;
  metadata.layer_mask_description = describeLayerMask(board, layer_ids);
  metadata.side_specific = isSideSpecific(layer_ids);
  metadata.is_on_copper_layer =
      std::any_of(layer_ids.begin(), layer_ids.end(), [&board](const std::string& layer_id) {
        return isCopperLayerId(board, layer_id);
      });
  metadata.has_hole = has_hole;
  metadata.has_drilled_hole = has_drilled_hole;
  metadata.locked = locked;
  metadata.knockout = knockout;
  metadata.view_layer_ids = viewLayersFor(layer_ids, locked);
  return metadata;
}

BoardItemMetadata boardItemMetadata(const Board& board, const Pad& pad) {
  const std::vector<std::string> resolved_layers = expandKiCadLayerSet(pad.padstack.layer_set, board);
  const bool drilled = pad.padstack.drill.size.width.nanometers > 0;
  return boardItemMetadata(board, resolved_layers, true, drilled, drilled, pad.locked);
}

BoardItemMetadata boardItemMetadata(const Board& board, const Via& via) {
  return boardItemMetadata(board, copperLayerIds(board), true, via.drill.nanometers > 0,
                           via.drill.nanometers > 0, via.locked);
}

BoardItemMetadata boardItemMetadata(const Board& board, const TrackSegment& track) {
  return boardItemMetadata(board,
                           std::vector<std::string>{track.layer_id},
                           true,
                           false,
                           false,
                           track.locked);
}

BoardItemMetadata boardItemMetadata(const Board& board, const TrackArc& arc) {
  return boardItemMetadata(board,
                           std::vector<std::string>{arc.layer_id},
                           true,
                           false,
                           false,
                           arc.locked);
}

BoardItemMetadata boardItemMetadata(const Board& board, const BoardGraphic& graphic) {
  return boardItemMetadata(board,
                           std::vector<std::string>{graphic.layer_id},
                           true,
                           false,
                           false,
                           graphic.locked);
}

BoardItemMetadata boardItemMetadata(const Board& board, const BoardText& text) {
  return boardItemMetadata(board,
                           std::vector<std::string>{text.layer_id},
                           true,
                           false,
                           false,
                           text.locked);
}

BoardItemMetadata boardItemMetadata(const Board& board, const BoardZone& zone) {
  return boardItemMetadata(board, zone.layer_ids, true, false, false, zone.locked);
}

BoardItemMetadata boardItemMetadata(const Board& board, const BoardTeardrop& teardrop) {
  return boardItemMetadata(board,
                           std::vector<std::string>{teardrop.layer_id},
                           true,
                           false,
                           false,
                           teardrop.locked);
}

}  // namespace ccad
