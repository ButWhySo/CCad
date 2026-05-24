#pragma once

#include "ccad_core/erc.hpp"
#include "ccad_core/model.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ccad {

struct ProjectReview {
  std::string project_id;
  std::string project_name;
  std::size_t component_count = 0;
  std::size_t net_count = 0;
  std::size_t constraint_count = 0;
  bool has_board = false;
  std::int64_t board_width_nm = 0;
  std::int64_t board_height_nm = 0;
  std::size_t layer_count = 0;
  std::size_t pad_count = 0;
  std::size_t via_count = 0;
  std::size_t track_count = 0;
  std::size_t keepout_count = 0;
  std::vector<Diagnostic> diagnostics;
  std::size_t error_count = 0;
  std::size_t warning_count = 0;
  std::string status;
};

ProjectReview buildReview(const Project& project);

}  // namespace ccad

