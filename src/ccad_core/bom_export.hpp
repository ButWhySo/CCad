#pragma once

#include "ccad_core/model.hpp"
#include <string>

namespace ccad {

/**
 * @brief Exports the project's components as a CSV Bill of Materials.
 * @param project The CCad project to export.
 * @return A string containing the CSV formatted BOM.
 */
std::string exportToBomCsv(const Project& project);

} // namespace ccad
