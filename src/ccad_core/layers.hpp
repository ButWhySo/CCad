#pragma once

#include "ccad_core/model.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ccad {

std::vector<Layer> standardKiCadPcbLayers();
const Layer* findStandardKiCadPcbLayer(const std::string& id);
std::size_t appendMissingStandardKiCadPcbLayers(Board& board);

}  // namespace ccad
