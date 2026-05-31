#pragma once

#include "model.hpp"
#include <string>

namespace ccad {

/**
 * @brief Export the board to Specctra DSN format.
 * 
 * @param project The CCad project containing the board.
 * @return A string containing the DSN contents.
 * @throws std::runtime_error if the board is missing or invalid.
 */
std::string exportSpecctraDsn(const Project& project);

}  // namespace ccad
