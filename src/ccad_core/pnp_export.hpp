#pragma once

#include "ccad_core/model.hpp"
#include <string>

namespace ccad {

/**
 * @brief Exports the project's Pick and Place component data as CSV.
 * Computes component centroids from placed pad geometries.
 * @param project The CCad project to export.
 * @return A string containing the CSV formatted PnP data.
 */
std::string exportToPnpCsv(const Project& project);

} // namespace ccad
