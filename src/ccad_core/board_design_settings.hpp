#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <vector>

namespace ccad {

struct DesignRuleValidationError {
  std::string field;
  std::string code;
  std::string message;
};

std::vector<DesignRuleValidationError> validateDesignRules(const DesignRules& rules);

}  // namespace ccad
