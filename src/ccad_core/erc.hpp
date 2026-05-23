#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <vector>

namespace ccad {

struct Diagnostic {
  std::string severity;
  std::string code;
  std::string message;
  std::string object_id;
};

std::vector<Diagnostic> runErc(const Project& project);

}  // namespace ccad

