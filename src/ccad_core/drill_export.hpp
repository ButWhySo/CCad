#pragma once

#include "ccad_core/model.hpp"
#include <string>

namespace ccad {

/**
 * @brief Exports the project's drilled holes (vias and through-hole pads)
 * to an Excellon NC Drill file.
 * @param project The CCad project to export.
 * @return A string containing the Excellon formatted drill data.
 */
std::string exportToDrillExcellon(const Project& project);

} // namespace ccad
