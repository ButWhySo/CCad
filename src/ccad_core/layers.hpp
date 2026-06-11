#pragma once

#include "ccad_core/model.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace ccad {

std::vector<Layer> standardKiCadPcbLayers();
const Layer* findStandardKiCadPcbLayer(const std::string& id);
std::optional<std::size_t> standardKiCadPcbLayerNumber(const std::string& id);
std::vector<std::string> expandKiCadLayerSet(const std::vector<std::string>& layer_selectors,
                                             const Board& board);
std::vector<std::size_t> standardKiCadPcbLayerNumbersForSet(
    const std::vector<std::string>& layer_ids);
std::size_t appendMissingStandardKiCadPcbLayers(Board& board);

}  // namespace ccad
