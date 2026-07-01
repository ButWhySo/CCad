#pragma once

#include "ccad_core/footprint.hpp"
#include <vector>
#include <string>

namespace ccad {

struct LosslessnessDiagnostic {
  std::string severity; // "error" or "warning"
  std::string field;
  std::string message;
};

std::vector<LosslessnessDiagnostic> verifyFootprintLosslessness(
    const Footprint& original,
    const Footprint& candidate);

} // namespace ccad
