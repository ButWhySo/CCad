#pragma once

#include "ccad_core/footprint.hpp"
#include <string>

namespace ccad {

std::string exportKiCadFootprint(const Footprint& footprint);

}  // namespace ccad
