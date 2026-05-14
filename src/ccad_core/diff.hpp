#pragma once

#include "ccad_core/model.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ccad {

struct DiffEntry {
  std::string change;
  std::string object_type;
  std::string object_id;
  std::string message;
};

struct ProjectDiff {
  std::vector<DiffEntry> entries;
  std::size_t added_count = 0;
  std::size_t removed_count = 0;
  std::size_t changed_count = 0;
};

ProjectDiff diffProjects(const Project& before, const Project& after);

}  // namespace ccad

