#pragma once

#include "ccad_core/model.hpp"

#include <cstdint>

namespace ccad {

struct CanvasScene {
  bool has_board = false;
  std::int64_t board_width_nm = 0;
  std::int64_t board_height_nm = 0;
  double view_width_units = 0.0;
  double view_height_units = 0.0;
};

CanvasScene buildCanvasScene(const Project& project);

}  // namespace ccad

