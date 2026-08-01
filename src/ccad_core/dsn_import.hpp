#pragma once

#include "ccad_core/model.hpp"
#include <string_view>
#include <vector>

namespace ccad {

struct SesRouting {
  std::vector<TrackSegment> tracks;
  std::vector<Via> vias;
};

SesRouting importSpecctraSes(std::string_view source);

} // namespace ccad
