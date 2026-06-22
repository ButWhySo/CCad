#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <cstdint>

namespace ccad {

// Computes the diameter of removed copper at a given layer due to counterbore or countersink
// mapped from KiCad's PAD::GetPostMachiningKnockout
int64_t getPostMachiningKnockout(const Board& board, const Pad& pad, const std::string& layer_id);

// Checks if the layer falls within the secondary or tertiary drill range, or has post-machining knockout
// mapped from KiCad's PAD::IsBackdrilledOrPostMachined
bool isBackdrilledOrPostMachined(const Board& board, const Pad& pad, const std::string& layer_id);

}  // namespace ccad
