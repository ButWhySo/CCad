#pragma once

#include "ccad_core/erc.hpp"
#include "ccad_core/model.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ccad {

struct ProjectReview {
  std::string project_id;
  std::string project_name;
  std::size_t component_count = 0;
  std::size_t net_count = 0;
  std::size_t constraint_count = 0;
  std::vector<Diagnostic> diagnostics;
  std::size_t error_count = 0;
  std::size_t warning_count = 0;
  std::string status;
};

ProjectReview buildReview(const Project& project);

}  // namespace ccad

