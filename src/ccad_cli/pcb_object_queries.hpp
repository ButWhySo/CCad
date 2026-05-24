#pragma once

#include "ccad_core/model.hpp"

#include <string>

namespace ccad_cli {

std::string pcbLayerObjectJson(const ccad::Layer& layer);
std::string pcbPadObjectJson(const ccad::Pad& pad);
std::string pcbViaObjectJson(const ccad::Via& via);
std::string pcbTrackObjectJson(const ccad::TrackSegment& track);
std::string pcbRegionObjectJson(const std::string& type, const std::string& id,
                                const std::string& kind, const ccad::Rect& area);
std::string listPcbObjectsJson(const ccad::Board& board, const std::string& type_filter);
std::string listPcbNetsJson(const ccad::Board& board);
void requireKnownPcbObjectType(const std::string& type);

}  // namespace ccad_cli
