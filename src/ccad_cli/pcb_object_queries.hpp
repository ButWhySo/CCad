#pragma once

#include "ccad_core/model.hpp"
#include "ccad_core/net_chain_bridging.hpp"

#include <string>

namespace ccad_cli {

std::string pcbLayerObjectJson(const ccad::Layer& layer);
std::string pcbPadObjectJson(const ccad::Board& board, const ccad::Pad& pad);
std::string pcbViaObjectJson(const ccad::Board& board, const ccad::Via& via);
std::string pcbTrackObjectJson(const ccad::Board& board, const ccad::TrackSegment& track);
std::string pcbBoardGraphicObjectJson(const ccad::Board& board, const ccad::BoardGraphic& graphic);
std::string pcbBoardTextObjectJson(const ccad::Board& board, const ccad::BoardText& text);
std::string pcbBoardZoneObjectJson(const ccad::Board& board, const ccad::BoardZone& zone);
std::string pcbRegionObjectJson(const std::string& type, const std::string& id,
                                const std::string& kind, const ccad::Rect& area);
std::string listPcbObjectsJson(const ccad::Board& board, const std::string& type_filter);
std::string listPcbNetsJson(const ccad::Board& board);
std::string listPcbObjectsByNetJson(const ccad::Board& board, const std::string& net_id,
                                    const std::string& type_filter);
std::string listPcbConnectedObjectsJson(const ccad::Board& board, const std::string& source_id,
                                        const std::string& type_filter);
std::string listPcbEnabledLayersJson(const ccad::Board& board);
std::string listPcbVisibleLayersJson(const ccad::Board& board);
std::string getPcbLayerNameJson(const ccad::Board& board, const std::string& layer_id);
std::string getPcbBoardStackupJson(const ccad::Board& board);
std::string getPcbDesignRulesJson(const ccad::Board& board);
std::string getPcbOutlineJson(const ccad::Board& board);
std::string listRouteRequestsJson(const ccad::Board& board);
std::string routeStatusJson(const ccad::Board& board);
std::string exportRouteJobJson(const ccad::Board& board, const std::string& request_id_filter);
std::string netChainBridgingReportJson(const ccad::NetChainBridgingReport& report);
void requireKnownPcbObjectType(const std::string& type);
void requireKnownPcbConnectableObjectType(const std::string& type);

}  // namespace ccad_cli
