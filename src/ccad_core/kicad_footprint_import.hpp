#pragma once

#include "ccad_core/footprint.hpp"

#include <string>
#include <string_view>

namespace ccad {

Footprint importKiCadFootprint(std::string_view source);
std::string dumpFootprintJson(const Footprint& footprint);
Footprint loadFootprintJson(std::string_view source);

}  // namespace ccad
