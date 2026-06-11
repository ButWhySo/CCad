#pragma once

#include "ccad_core/footprint.hpp"
#include "ccad_core/model.hpp"

#include <map>
#include <string>

namespace ccad {

struct AutoPlacementPlan {
  bool placeable = false;
  Point origin;
  double score = 0.0;
  std::string reason;
};

AutoPlacementPlan planFootprintAutoPlacement(
    const Board& board, const Footprint& footprint,
    const std::map<std::string, std::string>& footprint_pad_nets, const std::string& layer_id,
    Length grid_step);

}  // namespace ccad
