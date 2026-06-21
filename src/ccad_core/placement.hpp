#pragma once

#include "ccad_core/model.hpp"
#include "ccad_core/footprint.hpp"
#include "ccad_core/symbol.hpp"

namespace ccad {

void placeFootprint(Project& project, const Footprint& footprint, const std::string& component_id,
                    const Point& origin, double rotation_deg, const std::string& layer_id,
                    std::optional<std::string> value = std::nullopt,
                    bool exclude_from_bom = false);

void placeComponent(Project& project, const Symbol& symbol, const std::string& component_id,
                    const Point& origin, double rotation_deg);

void moveFootprint(Project& project, const std::string& component_id, const Point& delta);

}  // namespace ccad
