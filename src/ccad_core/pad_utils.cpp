#include "ccad_core/pad_utils.hpp"
#include "ccad_core/model.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace ccad {

static int getCopperLayerIndex(const Board& board, const std::string& layer_id) {
  int copper_idx = 0;
  for (const Layer& layer : board.layers) {
    if (layer.kind == "copper") {
      if (layer.id == layer_id) {
        return copper_idx;
      }
      copper_idx++;
    }
  }
  return -1;
}

static int getCopperLayerCount(const Board& board) {
  int count = 0;
  for (const Layer& layer : board.layers) {
    if (layer.kind == "copper") count++;
  }
  return count;
}

static int64_t getLayerDistance(const Board& board, const std::string& layer1, const std::string& layer2) {
  int idx1 = getCopperLayerIndex(board, layer1);
  int idx2 = getCopperLayerIndex(board, layer2);
  int count = getCopperLayerCount(board);
  if (idx1 < 0 || idx2 < 0 || count <= 1) return 0;
  
  double ratio = std::abs(idx1 - idx2) / static_cast<double>(count - 1);
  return static_cast<int64_t>(ratio * board.design_rules.board_thickness.nanometers);
}

int64_t getPostMachiningKnockout(const Board& board, const Pad& pad, const std::string& layer_id) {
  if (getCopperLayerIndex(board, layer_id) < 0) return 0;

  // Check front post-machining
  const auto& frontPM = pad.padstack.front_post_machining;
  if (frontPM.mode.has_value() && frontPM.mode.value() != "not_post_machined" && frontPM.mode.value() != "unknown" && frontPM.size.nanometers > 0) {
    int64_t pmDepth = frontPM.depth.nanometers;

    if (pmDepth <= 0 && frontPM.mode.value() == "countersink" && frontPM.angle_degrees > 0) {
      double halfAngleRad = (frontPM.angle_degrees / 2.0) * M_PI / 180.0;
      pmDepth = static_cast<int64_t>(std::round((frontPM.size.nanometers / 2.0) / std::tan(halfAngleRad)));
    }

    if (pmDepth > 0) {
      int64_t layerDist = getLayerDistance(board, "F.Cu", layer_id);
      if (layerDist < pmDepth) {
        if (frontPM.mode.value() == "countersink" && frontPM.angle_degrees > 0) {
          double halfAngleRad = (frontPM.angle_degrees / 2.0) * M_PI / 180.0;
          int64_t diameterAtLayer = frontPM.size.nanometers - static_cast<int64_t>(std::round(2.0 * layerDist * std::tan(halfAngleRad)));
          return std::max<int64_t>(0, diameterAtLayer);
        } else {
          return frontPM.size.nanometers;
        }
      }
    }
  }

  // Check back post-machining
  const auto& backPM = pad.padstack.back_post_machining;
  if (backPM.mode.has_value() && backPM.mode.value() != "not_post_machined" && backPM.mode.value() != "unknown" && backPM.size.nanometers > 0) {
    int64_t pmDepth = backPM.depth.nanometers;

    if (pmDepth <= 0 && backPM.mode.value() == "countersink" && backPM.angle_degrees > 0) {
      double halfAngleRad = (backPM.angle_degrees / 2.0) * M_PI / 180.0;
      pmDepth = static_cast<int64_t>(std::round((backPM.size.nanometers / 2.0) / std::tan(halfAngleRad)));
    }

    if (pmDepth > 0) {
      int64_t layerDist = getLayerDistance(board, "B.Cu", layer_id);
      if (layerDist < pmDepth) {
        if (backPM.mode.value() == "countersink" && backPM.angle_degrees > 0) {
          double halfAngleRad = (backPM.angle_degrees / 2.0) * M_PI / 180.0;
          int64_t diameterAtLayer = backPM.size.nanometers - static_cast<int64_t>(std::round(2.0 * layerDist * std::tan(halfAngleRad)));
          return std::max<int64_t>(0, diameterAtLayer);
        } else {
          return backPM.size.nanometers;
        }
      }
    }
  }

  return 0;
}

bool isBackdrilledOrPostMachined(const Board& board, const Pad& pad, const std::string& layer_id) {
  int layerOrdinal = getCopperLayerIndex(board, layer_id);
  if (layerOrdinal < 0) return false;

  // Check secondary drill
  if (pad.padstack.secondary_drill.has_value()) {
    const auto& secDrill = pad.padstack.secondary_drill.value();
    if (secDrill.size.width.nanometers > 0 && !secDrill.start_layer.empty() && !secDrill.end_layer.empty()) {
      int startOrdinal = getCopperLayerIndex(board, secDrill.start_layer);
      int endOrdinal = getCopperLayerIndex(board, secDrill.end_layer);
      if (startOrdinal >= 0 && endOrdinal >= 0) {
        if (startOrdinal > endOrdinal) std::swap(startOrdinal, endOrdinal);
        if (layerOrdinal >= startOrdinal && layerOrdinal <= endOrdinal) return true;
      }
    }
  }

  // Check tertiary drill
  if (pad.padstack.tertiary_drill.has_value()) {
    const auto& terDrill = pad.padstack.tertiary_drill.value();
    if (terDrill.size.width.nanometers > 0 && !terDrill.start_layer.empty() && !terDrill.end_layer.empty()) {
      int startOrdinal = getCopperLayerIndex(board, terDrill.start_layer);
      int endOrdinal = getCopperLayerIndex(board, terDrill.end_layer);
      if (startOrdinal >= 0 && endOrdinal >= 0) {
        if (startOrdinal > endOrdinal) std::swap(startOrdinal, endOrdinal);
        if (layerOrdinal >= startOrdinal && layerOrdinal <= endOrdinal) return true;
      }
    }
  }

  // Check post-machining
  if (getPostMachiningKnockout(board, pad, layer_id) > 0) {
    return true;
  }

  return false;
}

}  // namespace ccad
