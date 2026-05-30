#pragma once

#include "ccad_core/model.hpp"

#include <string>

namespace ccad {

std::string exportToKiCadPcb(const Project& project);

}  // namespace ccad
