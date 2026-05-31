#pragma once

#include "ccad_core/model.hpp"
#include "ccad_core/footprint.hpp"
#include <string>

namespace ccad {

void placeFootprint(Project& project, const Footprint& footprint, const std::string& component_id,
                    const Point& origin, double rotation_deg, const std::string& layer_id);

}  // namespace ccad
