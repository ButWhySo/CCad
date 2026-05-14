#pragma once

#include "ccad_core/erc.hpp"
#include "ccad_core/model.hpp"

#include <vector>

namespace ccad {

std::vector<Diagnostic> runDrc(const Project& project);

}  // namespace ccad
