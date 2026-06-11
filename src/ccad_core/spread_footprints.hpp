#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <vector>

namespace ccad {

struct SpreadFootprintRequest {
  std::vector<std::string> component_ids;
  Point target;
  Length component_gap;
  Length group_gap;
};

struct SpreadFootprintPlacement {
  std::string component_id;
  Rect previous_bounds;
  Rect new_bounds;
};

std::vector<SpreadFootprintPlacement> spreadFootprintComponents(
    Board& board, const SpreadFootprintRequest& request);

}  // namespace ccad
