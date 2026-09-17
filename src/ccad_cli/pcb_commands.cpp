#include "ccad_cli/pcb_commands.hpp"

#include "ccad_cli/common.hpp"
#include "ccad_cli/pcb_object_queries.hpp"
#include "ccad_core/autoplacer.hpp"
#include "ccad_core/board_collector.hpp"
#include "ccad_core/board_item_container.hpp"
#include "ccad_core/board_loader.hpp"
#include "ccad_core/board_outline_polygon.hpp"
#include "ccad_core/board_statistics.hpp"
#include "ccad_core/board_text_var_adapter.hpp"
#include "ccad_core/bom_export.hpp"
#include "ccad_core/cleanup_item.hpp"
#include "ccad_core/graphics_cleaner.hpp"
#include "ccad_core/cross_probing.hpp"
#include "ccad_core/net_chain_bridging.hpp"
#include "ccad_core/net_info.hpp"
#include "ccad_core/diff.hpp"
#include "ccad_core/drill_export.hpp"
#include "ccad_core/dsn_export.hpp"
#include "ccad_core/dsn_import.hpp"
#include "ccad_core/kicad_pcb_export.hpp"
#include "ccad_core/item_geometry.hpp"
#include "ccad_core/json.hpp"
#include "ccad_core/layers.hpp"
#include "ccad_core/placement.hpp"
#include "ccad_core/pnp_export.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/teardrop_generator.hpp"
#include "ccad_core/zone_fill.hpp"
#include "ccad_core/filesystem_u8.hpp"
#include "ccad_core/spread_footprints.hpp"
#include <fstream>

#include <algorithm>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad_cli {
namespace {

ccad::PadShape parsePadShapeStr(const std::string& shape) {
  if (shape == "circle") return ccad::PadShape::Circle;
  if (shape == "rect" || shape == "rectangle") return ccad::PadShape::Rectangle;
  if (shape == "oval") return ccad::PadShape::Oval;
  if (shape == "trapezoid") return ccad::PadShape::Trapezoid;
  if (shape == "roundrect") return ccad::PadShape::RoundRect;
  if (shape == "chamfered_rect") return ccad::PadShape::ChamferedRect;
  if (shape == "custom") return ccad::PadShape::Custom;
  return ccad::PadShape::Circle;
}

std::string netIdForPin(const ccad::Project& project, const std::string& component_id,
                        const std::string& pin_name) {
  const ccad::Schematic* schematic = ccad::primarySchematic(project);
  if (schematic == nullptr) {
    return "";
  }
  for (const ccad::Net& net : schematic->nets) {
    for (const ccad::NetMember& member : net.members) {
      if (member.component_id == component_id && member.pin_name == pin_name) {
        return net.id;
      }
    }
  }
  return "";
}

bool parseVisibleOption(const std::map<std::string, std::string>& options) {
  if (!options.contains("--visible")) {
    return true;
  }
  const std::string value = requireOption(options, "--visible");
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error("--visible must be true or false");
}

bool parseRequiredVisibleOption(const std::map<std::string, std::string>& options) {
  requireOption(options, "--visible");
  return parseVisibleOption(options);
}

bool parseCompleteOption(const std::map<std::string, std::string>& options) {
  if (!options.contains("--complete")) {
    return true;
  }
  const std::string value = requireOption(options, "--complete");
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error("--complete must be true or false");
}

ccad::BoardContainerRemoveMode parseRemoveModeOption(
    const std::map<std::string, std::string>& options) {
  if (!options.contains("--mode")) {
    return ccad::BoardContainerRemoveMode::normal;
  }
  const std::string value = requireOption(options, "--mode");
  if (value == "normal") {
    return ccad::BoardContainerRemoveMode::normal;
  }
  if (value == "bulk") {
    return ccad::BoardContainerRemoveMode::bulk;
  }
  throw std::runtime_error("--mode must be normal or bulk");
}

std::vector<ccad::Point> parsePolylinePointsMm(const std::string& value) {
  std::vector<ccad::Point> points;
  std::stringstream point_stream(value);
  std::string point_text;
  while (std::getline(point_stream, point_text, ';')) {
    if (point_text.empty()) {
      throw std::runtime_error("--points-mm contains an empty point");
    }
    const std::size_t comma = point_text.find(',');
    if (comma == std::string::npos || point_text.find(',', comma + 1) != std::string::npos) {
      throw std::runtime_error("--points-mm points must use x,y pairs");
    }
    const std::string x_text = point_text.substr(0, comma);
    const std::string y_text = point_text.substr(comma + 1);
    if (x_text.empty() || y_text.empty()) {
      throw std::runtime_error("--points-mm points must include x and y");
    }
    points.push_back(ccad::Point{
        .x = ccad::millimeters(std::stod(x_text)),
        .y = ccad::millimeters(std::stod(y_text)),
    });
  }
  if (points.size() < 2) {
    throw std::runtime_error("--points-mm must contain at least two points");
  }
  return points;
}

void requireLayerUnused(const ccad::Board& board, const std::string& id) {
  for (const ccad::Pad& pad : board.pads) {
    for (const std::string& l : pad.padstack.layer_set) {
      if (l == id) {
        throw std::runtime_error("layer is referenced by pad: " + pad.id);
      }
    }
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    if (track.layer_id == id) {
      throw std::runtime_error("layer is referenced by track: " + track.id);
    }
  }
  for (const ccad::BoardGraphic& graphic : board.graphics) {
    if (graphic.layer_id == id) {
      throw std::runtime_error("layer is referenced by board graphic: " + graphic.id);
    }
  }
  for (const ccad::BoardText& text : board.texts) {
    if (text.layer_id == id) {
      throw std::runtime_error("layer is referenced by board text: " + text.id);
    }
  }
  for (const ccad::BoardZone& zone : board.zones) {
    for (const std::string& layer_id : zone.layer_ids) {
      if (layer_id == id) {
        throw std::runtime_error("layer is referenced by zone: " + zone.id);
      }
    }
  }
}

std::vector<std::string> splitLayers(const std::string& value) {
  std::vector<std::string> layers;
  std::stringstream ss(value);
  std::string layer;
  while (std::getline(ss, layer, ',')) {
    layers.push_back(layer);
  }
  return layers;
}

std::vector<std::string> splitCommaList(const std::string& value, const std::string& option_name) {
  std::vector<std::string> items;
  std::stringstream ss(value);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (item.empty()) {
      throw std::runtime_error(option_name + " must not contain empty items");
    }
    items.push_back(item);
  }
  return items;
}

ccad::Length requireMillimeters(const std::map<std::string, std::string>& options,
                                const std::string& key) {
  return ccad::millimeters(requireDoubleOption(options, key));
}

void setOptionalMillimeters(const std::map<std::string, std::string>& options,
                            const std::string& key, ccad::Length& target) {
  if (options.contains(key)) {
    target = requireMillimeters(options, key);
  }
}

bool requireBoolOption(const std::map<std::string, std::string>& options,
                       const std::string& key) {
  const std::string value = requireOption(options, key);
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error(key + " must be true or false");
}

void setOptionalBool(const std::map<std::string, std::string>& options, const std::string& key,
                     bool& target) {
  if (options.contains(key)) {
    target = requireBoolOption(options, key);
  }
}

std::optional<double> optionalRatio(const std::map<std::string, std::string>& options,
                                    const std::string& key) {
  if (!options.contains(key)) {
    return std::nullopt;
  }
  const double ratio = requireDoubleOption(options, key);
  if (ratio < 0.0 || ratio > 0.5) {
    throw std::runtime_error(key + " must be between 0.0 and 0.5");
  }
  return ratio;
}

bool projectHasNet(const ccad::Project& project, const std::string& net_id) {
  if (const ccad::Schematic* schematic = ccad::primarySchematic(project)) {
    for (const ccad::Net& net : schematic->nets) {
      if (net.id == net_id) {
        return true;
      }
    }
  }
  if (!project.boards.empty()) {
    for (const ccad::Pad& pad : project.boards[0].pads) {
      if (pad.net_id == net_id) {
        return true;
      }
    }
    for (const ccad::Via& via : project.boards[0].vias) {
      if (via.net_id == net_id) {
        return true;
      }
    }
    for (const ccad::TrackSegment& track : project.boards[0].tracks) {
      if (track.net_id == net_id) {
        return true;
      }
    }
    for (const ccad::BoardZone& zone : project.boards[0].zones) {
      if (zone.net_id == net_id) {
        return true;
      }
    }
  }
  return false;
}

std::map<std::string, std::string> footprintPadNetMap(const ccad::Project& project,
                                                      const ccad::Footprint& footprint,
                                                      const std::string& component_id) {
  std::map<std::string, std::string> pad_nets;
  for (const ccad::FootprintPad& pad : footprint.pads) {
    pad_nets[pad.number] = netIdForPin(project, component_id, pad.number);
  }
  return pad_nets;
}

std::string autoplaceResultJson(const std::string& component_id, const std::string& layer_id,
                                const ccad::AutoPlacementPlan& plan) {
  std::ostringstream out;
  out << "{\n"
      << "  \"autoplace\": {\n"
      << "    \"component_id\": \"" << ccad::escapeJson(component_id) << "\",\n"
      << "    \"layer_id\": \"" << ccad::escapeJson(layer_id) << "\",\n"
      << "    \"origin_x_nm\": " << plan.origin.x.nanometers << ",\n"
      << "    \"origin_y_nm\": " << plan.origin.y.nanometers << ",\n"
      << "    \"score\": " << plan.score << ",\n"
      << "    \"reason\": \"" << ccad::escapeJson(plan.reason) << "\"\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string spreadFootprintsResultJson(
    const std::vector<ccad::SpreadFootprintPlacement>& placements) {
  std::ostringstream out;
  out << "{\n"
      << "  \"spread_footprints\": {\n"
      << "    \"moved\": " << placements.size() << ",\n"
      << "    \"placements\": [\n";
  for (std::size_t index = 0; index < placements.size(); ++index) {
    const ccad::SpreadFootprintPlacement& placement = placements.at(index);
    out << "      {\n"
        << "        \"component_id\": \"" << ccad::escapeJson(placement.component_id)
        << "\",\n"
        << "        \"previous_x_nm\": " << placement.previous_bounds.origin.x.nanometers
        << ",\n"
        << "        \"previous_y_nm\": " << placement.previous_bounds.origin.y.nanometers
        << ",\n"
        << "        \"new_x_nm\": " << placement.new_bounds.origin.x.nanometers << ",\n"
        << "        \"new_y_nm\": " << placement.new_bounds.origin.y.nanometers << "\n"
        << "      }" << (index + 1 == placements.size() ? "" : ",") << '\n';
  }
  out << "    ]\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

void appendStringArray(std::ostringstream& out, const std::vector<std::string>& values,
                       const std::string& indent) {
  out << "[\n";
  for (std::size_t index = 0; index < values.size(); ++index) {
    out << indent << "  \"" << ccad::escapeJson(values.at(index)) << "\""
        << (index + 1 == values.size() ? "" : ",") << '\n';
  }
  out << indent << "]";
}

std::string cleanupActionsJson() {
  const std::vector<ccad::CleanupActionInfo>& actions = ccad::cleanupActionCatalog();
  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_class\": \"CLEANUP_ITEM\",\n"
      << "  \"provider_class\": \"VECTOR_CLEANUP_ITEMS_PROVIDER\",\n"
      << "  \"provider_semantics\": \"vector_indexed_rows\",\n"
      << "  \"parity_scope\": \"cleanup_action_catalog_first_slice\",\n"
      << "  \"actions\": [\n";
  for (std::size_t index = 0; index < actions.size(); ++index) {
    const ccad::CleanupActionInfo& action = actions.at(index);
    out << "    {\n"
        << "      \"index\": " << index << ",\n"
        << "      \"kicad_offset\": " << action.kicad_offset << ",\n"
        << "      \"id\": \"" << ccad::escapeJson(action.id) << "\",\n"
        << "      \"domain\": \"" << ccad::escapeJson(action.domain) << "\",\n"
        << "      \"title\": \"" << ccad::escapeJson(action.title) << "\"\n"
        << "    }" << (index + 1 == actions.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string collectBoardItemsJson(const ccad::BoardCollectorReport& report,
                                  const ccad::BoardCollectorGuide& guide) {
  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_collector\": \"" << ccad::escapeJson(report.kicad_collector) << "\",\n"
      << "  \"parity_scope\": \"" << ccad::escapeJson(report.parity_scope) << "\",\n"
      << "  \"scan_set\": \"" << ccad::escapeJson(report.scan_set) << "\",\n"
      << "  \"collector_guide\": {\n"
      << "    \"preferred_layer_id\": \"" << ccad::escapeJson(guide.preferred_layer_id)
      << "\",\n"
      << "    \"visible_layer_ids\": ";
  appendStringArray(out, guide.visible_layer_ids, "    ");
  out << ",\n"
      << "    \"include_secondary\": " << (guide.include_secondary ? "true" : "false")
      << ",\n"
      << "    \"ignore_locked_items\": "
      << (guide.ignore_locked_items ? "true" : "false") << ",\n"
      << "    \"ignore_tracks\": " << (guide.ignore_tracks ? "true" : "false") << ",\n"
      << "    \"ignore_zone_fills\": " << (guide.ignore_zone_fills ? "true" : "false")
      << ",\n"
      << "    \"ignore_no_nets\": " << (guide.ignore_no_nets ? "true" : "false") << "\n"
      << "  },\n"
      << "  \"kicad_scan_types\": ";
  appendStringArray(out, report.kicad_scan_types, "  ");
  out << ",\n"
      << "  \"unsupported_kicad_types\": ";
  appendStringArray(out, report.unsupported_kicad_types, "  ");
  out << ",\n"
      << "  \"summary\": {\n"
      << "    \"total\": " << report.candidates.size() << ",\n"
      << "    \"primary_count\": " << report.primary_count << ",\n"
      << "    \"secondary_count\": " << report.secondary_count << "\n"
      << "  },\n"
      << "  \"items\": [\n";
  for (std::size_t index = 0; index < report.candidates.size(); ++index) {
    const ccad::BoardCollectorCandidate& candidate = report.candidates.at(index);
    out << "    {\n"
        << "      \"type\": \"" << ccad::escapeJson(candidate.type) << "\",\n"
        << "      \"id\": \"" << ccad::escapeJson(candidate.id) << "\",\n"
        << "      \"kicad_type\": \"" << ccad::escapeJson(candidate.kicad_type) << "\",\n"
        << "      \"collection_bucket\": \"" << ccad::escapeJson(candidate.collection_bucket)
        << "\",\n"
        << "      \"net_id\": \"" << ccad::escapeJson(candidate.net_id) << "\",\n"
        << "      \"primary_layer_id\": \"" << ccad::escapeJson(candidate.primary_layer_id)
        << "\",\n"
        << "      \"layer_ids\": ";
    appendStringArray(out, candidate.layer_ids, "      ");
    out << ",\n"
        << "      \"current_layer_match\": "
        << (candidate.current_layer_match ? "true" : "false") << ",\n"
        << "      \"visible_layer_match\": "
        << (candidate.visible_layer_match ? "true" : "false") << ",\n"
        << "      \"locked\": " << (candidate.locked ? "true" : "false") << "\n"
        << "    }" << (index + 1 == report.candidates.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string boardLoadStateJson(const std::string& source_file,
                               const ccad::BoardLoadState& state) {
  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_class\": \"" << ccad::escapeJson(state.kicad_class) << "\",\n"
      << "  \"source_format\": \"" << ccad::escapeJson(state.source_format) << "\",\n"
      << "  \"source_file\": \"" << ccad::escapeJson(source_file) << "\",\n"
      << "  \"loaded\": " << (state.loaded ? "true" : "false") << ",\n"
      << "  \"initialize_after_load\": "
      << (state.initialize_after_load ? "true" : "false") << ",\n"
      << "  \"board_attached\": " << (state.board_attached ? "true" : "false") << ",\n"
      << "  \"design_rules_ready\": " << (state.design_rules_ready ? "true" : "false")
      << ",\n"
      << "  \"drc_ready\": " << (state.drc_ready ? "true" : "false") << ",\n"
      << "  \"persistent_drc_engine\": "
      << (state.persistent_drc_engine ? "true" : "false") << ",\n"
      << "  \"drc_engine_model\": \"" << ccad::escapeJson(state.drc_engine_model)
      << "\",\n"
      << "  \"connectivity_ready\": "
      << (state.connectivity_ready ? "true" : "false") << ",\n"
      << "  \"netlist_ready\": " << (state.netlist_ready ? "true" : "false") << ",\n"
      << "  \"netclass_sync_ready\": "
      << (state.netclass_sync_ready ? "true" : "false") << ",\n"
      << "  \"component_class_sync_ready\": "
      << (state.component_class_sync_ready ? "true" : "false") << ",\n"
      << "  \"drawing_sheet_loaded\": "
      << (state.drawing_sheet_loaded ? "true" : "false") << ",\n"
      << "  \"user_units_ready\": " << (state.user_units_ready ? "true" : "false")
      << ",\n"
      << "  \"board_count\": " << state.board_count << ",\n"
      << "  \"schematic_count\": " << state.schematic_count << ",\n"
      << "  \"active_board_index\": " << state.active_board_index << ",\n"
      << "  \"layer_count\": " << state.layer_count << ",\n"
      << "  \"copper_layer_count\": " << state.copper_layer_count << ",\n"
      << "  \"visible_layer_count\": " << state.visible_layer_count << ",\n"
      << "  \"hidden_layer_count\": " << state.hidden_layer_count << ",\n"
      << "  \"pad_count\": " << state.pad_count << ",\n"
      << "  \"via_count\": " << state.via_count << ",\n"
      << "  \"track_count\": " << state.track_count << ",\n"
      << "  \"graphic_count\": " << state.graphic_count << ",\n"
      << "  \"text_count\": " << state.text_count << ",\n"
      << "  \"zone_count\": " << state.zone_count << ",\n"
      << "  \"keepout_count\": " << state.keepout_count << ",\n"
      << "  \"placement_region_count\": " << state.placement_region_count << ",\n"
      << "  \"route_request_count\": " << state.route_request_count << ",\n"
      << "  \"physical_object_count\": " << state.physical_object_count << ",\n"
      << "  \"board_net_count\": " << state.board_net_count << ",\n"
      << "  \"pads_with_nets\": " << state.pads_with_nets << ",\n"
      << "  \"vias_with_nets\": " << state.vias_with_nets << ",\n"
      << "  \"tracks_with_nets\": " << state.tracks_with_nets << ",\n"
      << "  \"zones_with_nets\": " << state.zones_with_nets << ",\n"
      << "  \"pending_kicad_loader_steps\": ";
  appendStringArray(out, state.pending_kicad_loader_steps, "  ");
  out << "\n}\n";
  return out.str();
}

std::string drillShapeLabel(const ccad::DrillShape shape) {
  switch (shape) {
    case ccad::DrillShape::Circle:
      return "Round";
    case ccad::DrillShape::Oval:
      return "Slot";
    case ccad::DrillShape::Undefined:
      return "Unknown";
  }
  return "Unknown";
}

std::string drillSourceLabel(const ccad::DrillLineSource source) {
  switch (source) {
    case ccad::DrillLineSource::pad:
      return "Pad";
    case ccad::DrillLineSource::via:
      return "Via";
  }
  return "Unknown";
}

void appendNullableLayer(std::ostringstream& out, const std::string& layer_id) {
  if (layer_id.empty()) {
    out << "null";
  } else {
    out << "\"" << ccad::escapeJson(layer_id) << "\"";
  }
}

std::string drillStatisticsJson(const ccad::Board& board) {
  std::vector<ccad::DrillLineItem> drills = ccad::collectDrillLineItems(board);
  std::sort(drills.begin(), drills.end(),
            ccad::DrillLineItemCompare(ccad::DrillLineColumn::count, false));

  int total_drill_count = 0;
  for (const ccad::DrillLineItem& drill : drills) {
    total_drill_count += drill.quantity;
  }

  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_reference\": \"board_statistics\",\n"
      << "  \"parity_scope\": \"drill_line_items_first_slice\",\n"
      << "  \"summary\": {\n"
      << "    \"unique_drill_rows\": " << drills.size() << ",\n"
      << "    \"total_drill_count\": " << total_drill_count << "\n"
      << "  },\n"
      << "  \"drill_holes\": [\n";
  for (std::size_t index = 0; index < drills.size(); ++index) {
    const ccad::DrillLineItem& drill = drills.at(index);
    out << "    {\n"
        << "      \"count\": " << drill.quantity << ",\n"
        << "      \"shape\": \"" << drillShapeLabel(drill.shape) << "\",\n"
        << "      \"x_size_nm\": " << drill.x_size.nanometers << ",\n"
        << "      \"y_size_nm\": " << drill.y_size.nanometers << ",\n"
        << "      \"plated\": " << (drill.plated ? "true" : "false") << ",\n"
        << "      \"source\": \"" << drillSourceLabel(drill.source) << "\",\n"
        << "      \"start_layer\": ";
    appendNullableLayer(out, drill.start_layer_id);
    out << ",\n"
        << "      \"stop_layer\": ";
    appendNullableLayer(out, drill.stop_layer_id);
    out << "\n"
        << "    }" << (index + 1 == drills.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

void appendOptionalLengthNm(std::ostringstream& out, const std::optional<ccad::Length>& length) {
  if (length.has_value()) {
    out << length->nanometers;
  } else {
    out << "null";
  }
}

void appendDrillRows(std::ostringstream& out,
                     const std::vector<ccad::DrillLineItem>& drills,
                     const std::string& indent) {
  out << "[\n";
  for (std::size_t index = 0; index < drills.size(); ++index) {
    const ccad::DrillLineItem& drill = drills.at(index);
    out << indent << "  {\n"
        << indent << "    \"count\": " << drill.quantity << ",\n"
        << indent << "    \"shape\": \"" << drillShapeLabel(drill.shape) << "\",\n"
        << indent << "    \"x_size_nm\": " << drill.x_size.nanometers << ",\n"
        << indent << "    \"y_size_nm\": " << drill.y_size.nanometers << ",\n"
        << indent << "    \"plated\": " << (drill.plated ? "true" : "false") << ",\n"
        << indent << "    \"source\": \"" << drillSourceLabel(drill.source) << "\",\n"
        << indent << "    \"start_layer\": ";
    appendNullableLayer(out, drill.start_layer_id);
    out << ",\n" << indent << "    \"stop_layer\": ";
    appendNullableLayer(out, drill.stop_layer_id);
    out << "\n" << indent << "  }" << (index + 1 == drills.size() ? "" : ",") << '\n';
  }
  out << indent << "]";
}

std::string boardStatisticsReportJson(const ccad::Project& project, const ccad::Board& board) {
  const ccad::BoardStatisticsReport report =
      ccad::buildBoardStatisticsReport(board, project.name, project.name);
  int total_drill_count = 0;
  for (const ccad::DrillLineItem& drill : report.drill_holes) {
    total_drill_count += drill.quantity;
  }

  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_reference\": \"" << ccad::escapeJson(report.kicad_reference) << "\",\n"
      << "  \"parity_scope\": \"" << ccad::escapeJson(report.parity_scope) << "\",\n"
      << "  \"metadata\": {\n"
      << "    \"project\": \"" << ccad::escapeJson(report.project_name) << "\",\n"
      << "    \"board_name\": \"" << ccad::escapeJson(report.board_name) << "\"\n"
      << "  },\n"
      << "  \"board\": {\n"
      << "    \"has_outline\": " << (report.has_outline ? "true" : "false") << ",\n"
      << "    \"width_nm\": " << report.board_width.nanometers << ",\n"
      << "    \"height_nm\": " << report.board_height.nanometers << ",\n"
      << "    \"board_area_square_mm\": " << report.board_area_square_mm << ",\n"
      << "    \"front_copper_area_square_mm\": " << report.front_copper_area_square_mm << ",\n"
      << "    \"back_copper_area_square_mm\": " << report.back_copper_area_square_mm << ",\n"
      << "    \"front_footprint_area_square_mm\": "
      << report.front_footprint_area_square_mm << ",\n"
      << "    \"back_footprint_area_square_mm\": " << report.back_footprint_area_square_mm
      << ",\n"
      << "    \"front_footprint_density_percent\": "
      << report.front_footprint_density_percent << ",\n"
      << "    \"back_footprint_density_percent\": "
      << report.back_footprint_density_percent << ",\n"
      << "    \"min_track_width_nm\": ";
  appendOptionalLengthNm(out, report.min_track_width);
  out << ",\n"
      << "    \"min_drill_diameter_nm\": ";
  appendOptionalLengthNm(out, report.min_drill_diameter);
  out << ",\n"
      << "    \"board_thickness_nm\": " << report.board_thickness.nanometers << "\n"
      << "  },\n"
      << "  \"object_counts\": {\n"
      << "    \"pad_count\": " << report.object_counts.pad_count << ",\n"
      << "    \"through_hole_pad_count\": " << report.object_counts.through_hole_pad_count
      << ",\n"
      << "    \"smd_pad_count\": " << report.object_counts.smd_pad_count << ",\n"
      << "    \"connector_pad_count\": " << report.object_counts.connector_pad_count
      << ",\n"
      << "    \"npth_pad_count\": " << report.object_counts.npth_pad_count << ",\n"
      << "    \"via_count\": " << report.object_counts.via_count << ",\n"
      << "    \"track_count\": " << report.object_counts.track_count << ",\n"
      << "    \"zone_count\": " << report.object_counts.zone_count << ",\n"
      << "    \"graphic_count\": " << report.object_counts.graphic_count << ",\n"
      << "    \"text_count\": " << report.object_counts.text_count << ",\n"
      << "    \"keepout_count\": " << report.object_counts.keepout_count << ",\n"
      << "    \"placement_region_count\": " << report.object_counts.placement_region_count
      << "\n"
      << "  },\n"
      << "  \"drill_summary\": {\n"
      << "    \"unique_drill_rows\": " << report.drill_holes.size() << ",\n"
      << "    \"total_drill_count\": " << total_drill_count << "\n"
      << "  },\n"
      << "  \"drill_holes\": ";
  appendDrillRows(out, report.drill_holes, "  ");
  out << "\n}\n";
  return out.str();
}

void appendPointRows(std::ostringstream& out, const std::vector<ccad::Point>& points,
                     const std::string& indent) {
  out << "[\n";
  for (std::size_t index = 0; index < points.size(); ++index) {
    const ccad::Point& point = points.at(index);
    out << indent << "  {\"x_nm\": " << point.x.nanometers << ", \"y_nm\": "
        << point.y.nanometers << "}" << (index + 1 == points.size() ? "" : ",") << '\n';
  }
  out << indent << "]";
}

std::string boardOutlinePolygonJson(const ccad::BoardOutlinePolygonReport& report) {
  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_source\": \"" << ccad::escapeJson(report.kicad_source) << "\",\n"
      << "  \"kicad_function\": \"" << ccad::escapeJson(report.kicad_function) << "\",\n"
      << "  \"parity_scope\": \"" << ccad::escapeJson(report.parity_scope) << "\",\n"
      << "  \"edge_cut_segment_count\": " << report.edge_cut_segment_count << ",\n"
      << "  \"outline_count\": " << report.outline_count << ",\n"
      << "  \"hole_count\": " << report.hole_count << ",\n"
      << "  \"closed\": " << (report.closed ? "true" : "false") << ",\n"
      << "  \"valid\": " << (report.valid ? "true" : "false") << ",\n"
      << "  \"used_inferred_outline\": "
      << (report.used_inferred_outline ? "true" : "false") << ",\n"
      << "  \"allow_disjoint\": " << (report.allow_disjoint ? "true" : "false") << ",\n"
      << "  \"allow_use_arcs_in_polygons\": "
      << (report.allow_use_arcs_in_polygons ? "true" : "false") << ",\n"
      << "  \"bounding_box\": {\n"
      << "    \"x_nm\": " << report.bounding_box.origin.x.nanometers << ",\n"
      << "    \"y_nm\": " << report.bounding_box.origin.y.nanometers << ",\n"
      << "    \"width_nm\": " << report.bounding_box.size.width.nanometers << ",\n"
      << "    \"height_nm\": " << report.bounding_box.size.height.nanometers << "\n"
      << "  },\n"
      << "  \"points\": ";
  appendPointRows(out, report.points, "  ");
  out << ",\n"
      << "  \"source_graphic_ids\": ";
  appendStringArray(out, report.source_graphic_ids, "  ");
  out << ",\n"
      << "  \"diagnostics\": ";
  appendStringArray(out, report.diagnostics, "  ");
  out << ",\n"
      << "  \"pending_kicad_features\": ";
  appendStringArray(out, report.pending_kicad_features, "  ");
  out << "\n}\n";
  return out.str();
}

void appendCrossProbeTargets(std::ostringstream& out,
                             const std::vector<ccad::CrossProbeTarget>& targets,
                             const std::string& indent) {
  out << "[\n";
  for (std::size_t index = 0; index < targets.size(); ++index) {
    const ccad::CrossProbeTarget& target = targets.at(index);
    out << indent << "  {\n"
        << indent << "    \"type\": \"" << ccad::escapeJson(target.type) << "\",\n"
        << indent << "    \"id\": \"" << ccad::escapeJson(target.id) << "\",\n"
        << indent << "    \"component_id\": \"" << ccad::escapeJson(target.component_id)
        << "\",\n"
        << indent << "    \"pin_name\": \"" << ccad::escapeJson(target.pin_name) << "\",\n"
        << indent << "    \"net_id\": \"" << ccad::escapeJson(target.net_id) << "\",\n"
        << indent << "    \"source\": \"" << ccad::escapeJson(target.source) << "\",\n"
        << indent << "    \"selection_index\": " << target.selection_index << ",\n"
        << indent << "    \"focus\": " << (target.focus ? "true" : "false") << "\n"
        << indent << "  }" << (index + 1 == targets.size() ? "" : ",") << '\n';
  }
  out << indent << "]";
}

std::string crossProbeReportJson(const ccad::CrossProbeReport& report) {
  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_source\": \"" << ccad::escapeJson(report.kicad_source) << "\",\n"
      << "  \"kicad_class\": \"" << ccad::escapeJson(report.kicad_class) << "\",\n"
      << "  \"kicad_function\": \"" << ccad::escapeJson(report.kicad_function) << "\",\n"
      << "  \"parity_scope\": \"" << ccad::escapeJson(report.parity_scope) << "\",\n"
      << "  \"packet\": \"" << ccad::escapeJson(report.packet) << "\",\n"
      << "  \"packet_kind\": \"" << ccad::escapeJson(report.packet_kind) << "\",\n"
      << "  \"clear_highlight\": " << (report.clear_highlight ? "true" : "false") << ",\n"
      << "  \"select_connections\": " << (report.select_connections ? "true" : "false")
      << ",\n"
      << "  \"part_reference\": \"" << ccad::escapeJson(report.part_reference) << "\",\n"
      << "  \"pad_number\": \"" << ccad::escapeJson(report.pad_number) << "\",\n"
      << "  \"requested_nets\": ";
  appendStringArray(out, report.requested_nets, "  ");
  out << ",\n"
      << "  \"targets\": ";
  appendCrossProbeTargets(out, report.targets, "  ");
  out << ",\n"
      << "  \"diagnostics\": ";
  appendStringArray(out, report.diagnostics, "  ");
  out << ",\n"
      << "  \"pending_kicad_features\": ";
  appendStringArray(out, report.pending_kicad_features, "  ");
  out << "\n}\n";
  return out.str();
}

void appendTextVariableReferences(std::ostringstream& out,
                                  const std::vector<ccad::TextVariableReference>& refs,
                                  const std::string& indent) {
  out << indent << "\"references\": [\n";
  for (std::size_t i = 0; i < refs.size(); ++i) {
    const ccad::TextVariableReference& ref = refs.at(i);
    out << indent << "  {\"name\": \"" << ccad::escapeJson(ref.name)
        << "\", \"resolved\": " << (ref.resolved ? "true" : "false") << "}"
        << (i + 1 == refs.size() ? "" : ",") << '\n';
  }
  out << indent << "]";
}

std::string textVariableExpansionJson(const ccad::Project& project,
                                      const ccad::Board& board,
                                      const std::optional<std::string>& explicit_text) {
  std::ostringstream out;
  out << "{\n"
      << "  \"kicad_handler\": \"ExpandTextVariables\",\n"
      << "  \"kicad_source\": \"BOARD::ResolveTextVar\",\n"
      << "  \"parity_scope\": \"project_text_variable_expansion_first_slice\",\n";

  out << "  \"variables\": {\n";
  std::size_t variable_index = 0;
  for (const auto& [key, value] : project.text_variables) {
    out << "    \"" << ccad::escapeJson(key) << "\": \""
        << ccad::escapeJson(value) << "\""
        << (++variable_index == project.text_variables.size() ? "" : ",") << '\n';
  }
  out << "  },\n";

  out << "  \"texts\": [\n";
  if (explicit_text.has_value()) {
    std::vector<ccad::TextVariableReference> refs;
    const std::string expanded =
        ccad::expandTextVariables(*explicit_text, project.text_variables, &refs);
    out << "    {\n"
        << "      \"source_text\": \"" << ccad::escapeJson(*explicit_text) << "\",\n"
        << "      \"expanded_text\": \"" << ccad::escapeJson(expanded) << "\",\n";
    appendTextVariableReferences(out, refs, "      ");
    out << "\n"
        << "    }\n";
  } else {
    const std::vector<ccad::ExpandedBoardText> texts = ccad::expandBoardTexts(project, board);
    for (std::size_t i = 0; i < texts.size(); ++i) {
      const ccad::ExpandedBoardText& text = texts.at(i);
      out << "    {\n"
          << "      \"id\": \"" << ccad::escapeJson(text.id) << "\",\n"
          << "      \"layer_id\": \"" << ccad::escapeJson(text.layer_id) << "\",\n"
          << "      \"source_text\": \"" << ccad::escapeJson(text.source_text) << "\",\n"
          << "      \"expanded_text\": \"" << ccad::escapeJson(text.expanded_text) << "\",\n";
      appendTextVariableReferences(out, text.references, "      ");
      out << "\n"
          << "    }" << (i + 1 == texts.size() ? "" : ",") << '\n';
    }
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string removeObjectResultJson(const ccad::BoardContainerRemoveResult& result) {
  std::ostringstream out;
  out << "{\"command\":\"pcb remove-object\""
      << ",\"kicad_container_class\":\"BOARD_ITEM_CONTAINER\""
      << ",\"kicad_method\":\"Delete\""
      << ",\"remove_mode\":\""
      << ccad::escapeJson(ccad::boardContainerRemoveModeName(result.mode)) << "\""
      << ",\"removed\":" << (result.removed ? "true" : "false")
      << ",\"id\":\"" << ccad::escapeJson(result.id) << "\""
      << ",\"kind\":\"" << ccad::escapeJson(ccad::boardContainerItemKindName(result.kind))
      << "\""
      << ",\"index\":" << result.index
      << ",\"kicad_delete_semantics\":"
      << (result.kicad_delete_semantics ? "true" : "false") << "}\n";
  return out.str();
}

int requireNonNegativeIntOption(const std::map<std::string, std::string>& options,
                                const std::string& key) {
  const std::string value = requireOption(options, key);
  try {
    std::size_t parsed = 0;
    const int number = std::stoi(value, &parsed);
    if (parsed != value.size() || number < 0) {
      throw std::runtime_error(key + " must be a non-negative integer");
    }
    return number;
  } catch (const std::exception&) {
    throw std::runtime_error(key + " must be a non-negative integer");
  }
}

std::string requireZonePadConnection(const std::map<std::string, std::string>& options) {
  const std::string value = requireOption(options, "--pad-connection");
  if (value != "thermal" && value != "solid" && value != "none" && value != "pth_thermal") {
    throw std::runtime_error("--pad-connection must be thermal, solid, none, or pth_thermal");
  }
  return value;
}

void requireBoardObjectsInsideOutline(const ccad::Board& board) {
  for (const ccad::Pad& pad : board.pads) {
    ccad::Size pad_size = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
    requireRotatedRectInsideBoard(board, pad.position, pad_size, pad.rotation_degrees,
                                  "pad " + pad.id);
  }
  for (const ccad::Via& via : board.vias) {
    requirePointWithMarginInsideBoard(board, via.position,
                                      ccad::nanometers(via.diameter.nanometers / 2),
                                      "via " + via.id);
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    const ccad::Length half_width = ccad::nanometers(track.width.nanometers / 2);
    requirePointWithMarginInsideBoard(board, track.start, half_width, "track " + track.id);
    requirePointWithMarginInsideBoard(board, track.end, half_width, "track " + track.id);
  }
  for (const ccad::BoardGraphic& graphic : board.graphics) {
    const ccad::Length half_width = ccad::nanometers(graphic.width.nanometers / 2);
    requirePointWithMarginInsideBoard(board, graphic.start, half_width,
                                      "board graphic " + graphic.id);
    requirePointWithMarginInsideBoard(board, graphic.end, half_width,
                                      "board graphic " + graphic.id);
  }
  for (const ccad::BoardText& text : board.texts) {
    requireInsideBoard(board, text.position, "board text " + text.id);
  }
  for (const ccad::BoardZone& zone : board.zones) {
    for (const ccad::Point& point : zone.outline) {
      requireInsideBoard(board, point, "zone " + zone.id);
    }
  }
  for (const ccad::Keepout& keepout : board.keepouts) {
    requireRectInsideBoard(board, keepout.area, "keepout " + keepout.id);
  }
  for (const ccad::PlacementRegion& region : board.placement_regions) {
    requireRectInsideBoard(board, region.area, "placement region " + region.id);
  }
}

void requireUniqueRouteRequestId(const ccad::Board& board, const std::string& id) {
  for (const ccad::RouteRequest& route_request : board.route_requests) {
    if (route_request.id == id) {
      throw std::runtime_error("duplicate route request id: " + id);
    }
  }
}

void requireBoardObjectId(const ccad::Board& board, const std::string& id) {
  for (const ccad::Pad& pad : board.pads) {
    if (pad.id == id) {
      return;
    }
  }
  for (const ccad::Via& via : board.vias) {
    if (via.id == id) {
      return;
    }
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    if (track.id == id) {
      return;
    }
  }
  throw std::runtime_error("unknown board object id: " + id);
}

}  // namespace

int pcbCommand(const std::vector<std::string>& args) {
  if (args.empty()) {
    std::cerr << "pcb requires a subcommand\n";
    return 2;
  }

  try {
    const std::string& subcommand = args.at(0);
    if (subcommand == "cleanup-actions") {
      std::cout << cleanupActionsJson();
      return 0;
    }

    if (subcommand == "clean-graphics") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--dry-run", "--merge-rects", "--delete-redundant", "--merge-pads"});
      const std::string file = requireOption(options, "--file");
      bool dry_run = options.contains("--dry-run") && options.at("--dry-run") == "true";
      bool merge_rects = !options.contains("--merge-rects") || options.at("--merge-rects") != "false";
      bool delete_redundant = !options.contains("--delete-redundant") || options.at("--delete-redundant") != "false";
      bool merge_pads = !options.contains("--merge-pads") || options.at("--merge-pads") != "false";

      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = const_cast<ccad::Board&>(requireBoard(project));
      
      ccad::GraphicsCleaner cleaner(board);
      auto actions = cleaner.cleanupBoard(dry_run, merge_rects, delete_redundant, merge_pads);
      
      if (!dry_run) {
        if (!writeProjectFile(file, project)) {
          std::cerr << "failed to write project file: " << file << '\n';
          return 2;
        }
      }
      
      std::cout << "[\n";
      for (std::size_t i = 0; i < actions.size(); ++i) {
        const auto& a = actions[i];
        std::cout << "  {\n";
        std::cout << "    \"code\": \"" << ccad::cleanupActionTitle(a.code) << "\",\n";
        std::cout << "    \"item_ids\": [";
        for (std::size_t j = 0; j < a.item_ids.size(); ++j) {
          std::cout << "\"" << a.item_ids[j] << "\"";
          if (j + 1 < a.item_ids.size()) std::cout << ", ";
        }
        std::cout << "]\n";
        std::cout << "  }" << (i + 1 < actions.size() ? "," : "") << "\n";
      }
      std::cout << "]\n";
      return 0;
    }

    if (subcommand == "collect-items") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1,
                       {"--file", "--scan-set", "--preferred-layer", "--visible-layers",
                        "--include-secondary", "--ignore-locked", "--ignore-tracks",
                        "--ignore-zone-fills", "--ignore-no-nets"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      const ccad::Board& board = requireBoard(project);
      const ccad::BoardCollectorScanSet scan_set =
          ccad::parseBoardCollectorScanSet(requireOption(options, "--scan-set"));
      ccad::BoardCollectorGuide guide;
      if (options.contains("--preferred-layer")) {
        guide.preferred_layer_id = requireOption(options, "--preferred-layer");
      }
      if (options.contains("--visible-layers")) {
        guide.visible_layer_ids =
            splitCommaList(requireOption(options, "--visible-layers"), "--visible-layers");
      }
      if (options.contains("--include-secondary")) {
        guide.include_secondary = requireBoolOption(options, "--include-secondary");
      }
      if (options.contains("--ignore-locked")) {
        guide.ignore_locked_items = requireBoolOption(options, "--ignore-locked");
      }
      if (options.contains("--ignore-tracks")) {
        guide.ignore_tracks = requireBoolOption(options, "--ignore-tracks");
      }
      if (options.contains("--ignore-zone-fills")) {
        guide.ignore_zone_fills = requireBoolOption(options, "--ignore-zone-fills");
      }
      if (options.contains("--ignore-no-nets")) {
        guide.ignore_no_nets = requireBoolOption(options, "--ignore-no-nets");
      }
      std::cout << collectBoardItemsJson(ccad::collectBoardItems(board, scan_set, guide), guide);
      return 0;
    }

    if (subcommand == "add-layer") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--name", "--kind", "--visible"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string name = requireOption(options, "--name");
      const std::string kind = requireOption(options, "--kind");
      requireUniqueLayerId(board, id);
      board.layers.push_back(ccad::Layer{
          .id = id,
          .name = name,
          .kind = kind,
          .visible = parseVisibleOption(options),
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-standard-layers") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      ccad::appendMissingStandardKiCadPcbLayers(board);
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-layer") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--name", "--kind", "--visible"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string name = requireOption(options, "--name");
      const std::string kind = requireOption(options, "--kind");
      const bool visible = parseRequiredVisibleOption(options);
      if (kind != "copper") {
        requireLayerUnused(board, id);
      }
      bool updated = false;
      for (ccad::Layer& layer : board.layers) {
        if (layer.id == id) {
          layer.name = name;
          layer.kind = kind;
          layer.visible = visible;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown layer: " + id);
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "load-state") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--initialize"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      ccad::BoardLoadOptions load_options;
      if (options.contains("--initialize")) {
        load_options.initialize_after_load = requireBoolOption(options, "--initialize");
      }
      const ccad::BoardLoadState state = ccad::summarizeLoadedBoard(project, load_options);
      std::cout << boardLoadStateJson(file, state);
      return 0;
    }

    if (subcommand == "drill-statistics") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      const ccad::Board& board = requireBoard(project);
      std::cout << drillStatisticsJson(board);
      return 0;
    }

    if (subcommand == "board-statistics") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      const ccad::Board& board = requireBoard(project);
      std::cout << boardStatisticsReportJson(project, board);
      return 0;
    }

    if (subcommand == "expand-text-variables") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--text"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      const ccad::Board& board = requireBoard(project);
      std::optional<std::string> text;
      if (options.contains("--text")) {
        text = requireOption(options, "--text");
      }
      std::cout << textVariableExpansionJson(project, board, text);
      return 0;
    }

    if (subcommand == "get-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      const ccad::Board& board = project.boards[0];
      const std::string id = requireOption(options, "--id");
      for (const ccad::Layer& layer : board.layers) {
        if (layer.id == id) {
          std::cout << pcbLayerObjectJson(layer);
          return 0;
        }
      }
      for (const ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          std::cout << pcbPadObjectJson(board, pad);
          return 0;
        }
      }
      for (const ccad::Via& via : board.vias) {
        if (via.id == id) {
          std::cout << pcbViaObjectJson(board, via);
          return 0;
        }
      }
      for (const ccad::TrackSegment& track : board.tracks) {
        if (track.id == id) {
          std::cout << pcbTrackObjectJson(board, track);
          return 0;
        }
      }
      for (const ccad::BoardGraphic& graphic : board.graphics) {
        if (graphic.id == id) {
          std::cout << pcbBoardGraphicObjectJson(board, graphic);
          return 0;
        }
      }
      for (const ccad::BoardText& text : board.texts) {
        if (text.id == id) {
          std::cout << pcbBoardTextObjectJson(board, text);
          return 0;
        }
      }
      for (const ccad::BoardBarcode& barcode : board.barcodes) {
        if (barcode.id == id) {
          std::cout << boardBarcodeReportJson(barcode);
          return 0;
        }
      }
      for (const ccad::BoardDimension& dim : board.dimensions) {
        if (dim.id == id) {
          std::cout << boardDimensionReportJson(dim);
          return 0;
        }
      }
      for (const ccad::BoardGroup& group : board.groups) {
        if (group.id == id) {
          std::cout << boardGroupReportJson(group);
          return 0;
        }
      }
      for (const ccad::BoardTarget& target : board.targets) {
        if (target.id == id) {
          std::cout << boardTargetReportJson(target);
          return 0;
        }
      }
      for (const ccad::BoardZone& zone : board.zones) {
        if (zone.id == id) {
          std::cout << pcbBoardZoneObjectJson(board, zone);
          return 0;
        }
      }
      for (const ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          std::cout << pcbRegionObjectJson("keepout", keepout.id, keepout.kind, keepout.area);
          return 0;
        }
      }
      for (const ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          std::cout << pcbRegionObjectJson("placement_region", region.id, region.kind,
                                           region.area);
          return 0;
        }
      }
      throw std::runtime_error("unknown board object: " + id);
    }

    if (subcommand == "list-objects") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--type"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      const std::string type_filter =
          options.contains("--type") ? requireOption(options, "--type") : "";
      requireKnownPcbObjectType(type_filter);
      std::cout << listPcbObjectsJson(project.boards[0], type_filter);
      return 0;
    }

    if (subcommand == "list-enabled-layers") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << listPcbEnabledLayersJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "list-visible-layers") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << listPcbVisibleLayersJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "get-layer-name") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id"});
      const std::string file = requireOption(options, "--file");
      const std::string layer_id = requireOption(options, "--id");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << getPcbLayerNameJson(project.boards[0], layer_id);
      return 0;
    }

    if (subcommand == "get-board-stackup") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << getPcbBoardStackupJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "list-nets") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << listPcbNetsJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "list-by-net") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--net", "--type"});
      const std::string file = requireOption(options, "--file");
      const std::string net_id = requireOption(options, "--net");
      const std::string type_filter = options.contains("--type") ? requireOption(options, "--type") : "";
      requireKnownPcbConnectableObjectType(type_filter);
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << listPcbObjectsByNetJson(project.boards[0], net_id, type_filter);
      return 0;
    }

    if (subcommand == "list-connected") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--type"});
      const std::string file = requireOption(options, "--file");
      const std::string object_id = requireOption(options, "--id");
      const std::string type_filter = options.contains("--type") ? requireOption(options, "--type") : "";
      requireKnownPcbConnectableObjectType(type_filter);
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << listPcbConnectedObjectsJson(project.boards[0], object_id, type_filter);
      return 0;
    }

    if (subcommand == "list-route-requests") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << listRouteRequestsJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "route-status") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << routeStatusJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "get-rules") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << getPcbDesignRulesJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "get-outline") {
      const std::map<std::string, std::string> options = parseOptions(args, 1, {"--file"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      std::cout << getPcbOutlineJson(project.boards[0]);
      return 0;
    }

    if (subcommand == "outline-polygon") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--infer"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      ccad::BoardOutlinePolygonOptions outline_options;
      if (options.contains("--infer")) {
        outline_options.infer_outline_if_necessary = requireBoolOption(options, "--infer");
      }
      std::cout << boardOutlinePolygonJson(
          ccad::buildBoardOutlinePolygonReport(project.boards[0], outline_options));
      return 0;
    }

    if (subcommand == "cross-probe") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--packet"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      std::cout << crossProbeReportJson(
          ccad::resolveCrossProbePacket(project, requireOption(options, "--packet")));
      return 0;
    }

    if (subcommand == "calculate-net-bridges") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--net"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      std::cout << netChainBridgingReportJson(
          ccad::calculateNetChainBridges(project, requireOption(options, "--net")));
      return 0;
    }

    if (subcommand == "get-net-info") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--net"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      std::cout << netInfoReportJson(
          ccad::getNetInfo(project, requireOption(options, "--net")));
      return 0;
    }

    if (subcommand == "get-pad-machining") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--pad", "--layer"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      std::cout << padMachiningReportJson(project,
          requireOption(options, "--pad"), requireOption(options, "--layer"));
      return 0;
    }

    if (subcommand == "export-route-job") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--request-id"});
      const std::string file = requireOption(options, "--file");
      const ccad::Project project = loadProjectFile(file);
      if (!!project.boards.empty()) {
        throw std::runtime_error("project has no board");
      }
      const std::string request_id_filter =
          options.contains("--request-id") ? requireOption(options, "--request-id") : "";
      std::cout << exportRouteJobJson(project.boards[0], request_id_filter);
      return 0;
    }

    if (subcommand == "remove-layer") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireLayerUnused(board, id);
      bool removed = false;
      for (auto it = board.layers.begin(); it != board.layers.end(); ++it) {
        if (it->id == id) {
          board.layers.erase(it);
          removed = true;
          break;
        }
      }
      if (!removed) {
        throw std::runtime_error("unknown layer: " + id);
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-layer-visibility") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--visible"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      bool updated = false;
      for (ccad::Layer& layer : board.layers) {
        if (layer.id == id) {
          layer.visible = parseRequiredVisibleOption(options);
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown layer: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-rules") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--copper-clearance-mm", "--min-track-width-mm",
                                 "--max-track-width-mm",
                                 "--min-via-annular-ring-mm", "--min-connection-mm",
                                 "--min-via-diameter-mm", "--max-via-diameter-mm", "--min-through-hole-drill-mm",
                                 "--min-microvia-diameter-mm", "--min-microvia-drill-mm",
                                 "--min-hole-to-hole-mm", "--hole-clearance-mm",
                                 "--copper-edge-clearance-mm", "--silk-clearance-mm", "--min-text-height-mm",
                                 "--min-track-angle-degrees", "--max-track-angle-degrees",
                                 "--min-track-segment-length-mm", "--max-track-segment-length-mm",
                                 "--min-groove-width-mm", "--solder-mask-expansion-mm",
                                 "--solder-mask-min-width-mm",
                                 "--solder-mask-to-copper-clearance-mm",
                                 "--solder-paste-margin-mm", "--solder-paste-margin-ratio",
                                 "--board-thickness-mm", "--use-height-for-length-calcs",
                                 "--tent-vias-front", "--tent-vias-back", "--cover-vias-front",
                                 "--cover-vias-back", "--plug-vias-front", "--plug-vias-back",
                                 "--cap-vias", "--fill-vias"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      ccad::DesignRules rules = board.design_rules;
      rules.copper_clearance = requirePositiveMillimeters(options, "--copper-clearance-mm");
      rules.min_track_width = requirePositiveMillimeters(options, "--min-track-width-mm");
      setOptionalMillimeters(options, "--max-track-width-mm", rules.max_track_width);
      rules.min_via_annular_ring =
          requirePositiveMillimeters(options, "--min-via-annular-ring-mm");
      setOptionalMillimeters(options, "--min-connection-mm", rules.min_connection);
      setOptionalMillimeters(options, "--min-via-diameter-mm", rules.min_via_diameter);
      setOptionalMillimeters(options, "--max-via-diameter-mm", rules.max_via_diameter);
      setOptionalMillimeters(options, "--min-through-hole-drill-mm",
                             rules.min_through_hole_drill);
      setOptionalMillimeters(options, "--min-microvia-diameter-mm",
                             rules.min_microvia_diameter);
      setOptionalMillimeters(options, "--min-microvia-drill-mm", rules.min_microvia_drill);
      setOptionalMillimeters(options, "--min-hole-to-hole-mm", rules.min_hole_to_hole);
      setOptionalMillimeters(options, "--hole-clearance-mm", rules.hole_clearance);
      setOptionalMillimeters(options, "--copper-edge-clearance-mm", rules.copper_edge_clearance);
      setOptionalMillimeters(options, "--silk-clearance-mm", rules.silk_clearance);
      setOptionalMillimeters(options, "--min-text-height-mm", rules.min_text_height);
      if (options.contains("--min-track-angle-degrees"))
        rules.min_track_angle_degrees = requireDoubleOption(options, "--min-track-angle-degrees");
      if (options.contains("--max-track-angle-degrees"))
        rules.max_track_angle_degrees = requireDoubleOption(options, "--max-track-angle-degrees");
      setOptionalMillimeters(options, "--min-track-segment-length-mm", rules.min_track_segment_length);
      setOptionalMillimeters(options, "--max-track-segment-length-mm", rules.max_track_segment_length);
      setOptionalMillimeters(options, "--min-groove-width-mm", rules.min_groove_width);
      setOptionalMillimeters(options, "--solder-mask-expansion-mm",
                             rules.solder_mask_expansion);
      setOptionalMillimeters(options, "--solder-mask-min-width-mm",
                             rules.solder_mask_min_width);
      setOptionalMillimeters(options, "--solder-mask-to-copper-clearance-mm",
                             rules.solder_mask_to_copper_clearance);
      setOptionalMillimeters(options, "--solder-paste-margin-mm", rules.solder_paste_margin);
      if (options.contains("--solder-paste-margin-ratio")) {
        rules.solder_paste_margin_ratio = requireDoubleOption(options, "--solder-paste-margin-ratio");
      }
      setOptionalMillimeters(options, "--board-thickness-mm", rules.board_thickness);
      setOptionalBool(options, "--use-height-for-length-calcs",
                      rules.use_height_for_length_calcs);
      setOptionalBool(options, "--tent-vias-front", rules.tent_vias_front);
      setOptionalBool(options, "--tent-vias-back", rules.tent_vias_back);
      setOptionalBool(options, "--cover-vias-front", rules.cover_vias_front);
      setOptionalBool(options, "--cover-vias-back", rules.cover_vias_back);
      setOptionalBool(options, "--plug-vias-front", rules.plug_vias_front);
      setOptionalBool(options, "--plug-vias-back", rules.plug_vias_back);
      setOptionalBool(options, "--cap-vias", rules.cap_vias);
      setOptionalBool(options, "--fill-vias", rules.fill_vias);
      board.design_rules = rules;
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-outline") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--x-mm", "--y-mm", "--width-mm", "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const ccad::Rect previous_outline = board.outline;
      board.outline = ccad::Rect{
          .origin = ccad::Point{.x = requireMillimeters(options, "--x-mm"),
                                .y = requireMillimeters(options, "--y-mm")},
          .size = ccad::Size{.width = requirePositiveMillimeters(options, "--width-mm"),
                             .height = requirePositiveMillimeters(options, "--height-mm")},
      };
      try {
        requireBoardObjectsInsideOutline(board);
      } catch (...) {
        board.outline = previous_outline;
        throw;
      }
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }


    if (subcommand == "add-pad") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--component", "--pin", "--net", "--layers",
                                 "--type", "--shape", "--drill-mm", "--roundrect-rratio",
                                 "--chamfer-ratio",
                                 "--x-mm", "--y-mm", "--width-mm", "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::vector<std::string> layers = splitLayers(requireOption(options, "--layers"));
      requireUniquePadId(board, id);
      requireUniquePhysicalObjectId(board, id);
      for (const std::string& l : layers) {
        if (!l.starts_with("*.")) requireCopperLayer(board, l);
      }
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "pad position");
      const ccad::Size size{.width = requirePositiveMillimeters(options, "--width-mm"),
                            .height = requirePositiveMillimeters(options, "--height-mm")};
      requireCenteredRectInsideBoard(board, position, size, "pad");
      std::optional<ccad::Length> drill;
      if (options.contains("--drill-mm")) {
        drill = ccad::millimeters(requireDoubleOption(options, "--drill-mm"));
      }
      const std::optional<double> roundrect_rratio = optionalRatio(options, "--roundrect-rratio");
      const std::optional<double> chamfer_ratio = optionalRatio(options, "--chamfer-ratio");
      board.pads.push_back(ccad::Pad{
          .id = id,
          .component_id = requireOption(options, "--component"),
          .pin_name = requireOption(options, "--pin"),
          .net_id = requireOption(options, "--net"),
          .type = options.contains("--type") ? requireOption(options, "--type") : "smd",
          .position = position,
          .padstack = ccad::Padstack{
              .mode = ccad::PadstackMode::Normal,
              .layer_set = layers,
              .copper_props = {{"top", ccad::PadstackCopperLayerProps{
                  .shape = ccad::PadstackShapeProps{
                      .shape = options.contains("--shape") ? parsePadShapeStr(options.at("--shape")) : ccad::PadShape::Rectangle,
                      .anchor_shape = ccad::PadShape::Rectangle,
                      .size = size,
                      .offset = ccad::Point{ccad::nanometers(0), ccad::nanometers(0)},
                      .roundrect_rratio = roundrect_rratio.value_or(0.0),
                      .chamfer_ratio = chamfer_ratio.value_or(0.0),
                      .chamfer_positions = 0,
                      .trapezoid_delta_size = ccad::Size{ccad::nanometers(0), ccad::nanometers(0)}
                  },
                  .clearance = std::nullopt,
                  .zone_connection = std::nullopt,
                  .thermal_gap = std::nullopt,
                  .thermal_spoke_width = std::nullopt,
                  .thermal_spoke_angle_degrees = std::nullopt
              }}},
              .drill = ccad::PadstackDrillProps{
                  .size = ccad::Size{.width = drill.value_or(ccad::Length{}), .height = drill.value_or(ccad::Length{})},
                  .shape = ccad::DrillShape::Circle,
                  .start_layer = "",
                  .end_layer = "",
                  .is_capped = std::nullopt,
                  .is_filled = std::nullopt
              },
              .secondary_drill = std::nullopt,
              .tertiary_drill = std::nullopt,
              .front_post_machining = ccad::PadstackPostMachiningProps{std::nullopt, ccad::nanometers(0), ccad::nanometers(0), 0.0},
              .back_post_machining = ccad::PadstackPostMachiningProps{std::nullopt, ccad::nanometers(0), ccad::nanometers(0), 0.0},
              .front_outer_layers = ccad::PadstackOuterLayerProps{std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt},
              .back_outer_layers = ccad::PadstackOuterLayerProps{std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt},
              .unconnected_layer_mode = "keep_all"
          }
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-pad") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--component", "--pin", "--net", "--layers",
                                 "--type", "--shape", "--rotation-deg", "--roundrect-rratio",
                                 "--chamfer-ratio"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::vector<std::string> layers = splitLayers(requireOption(options, "--layers"));
      const double rotation_degrees = requireDoubleOption(options, "--rotation-deg");
      for (const std::string& l : layers) {
        if (!l.starts_with("*.")) requireCopperLayer(board, l);
      }
      bool updated = false;
      for (ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          ccad::Size pad_size = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
          requireRotatedRectInsideBoard(board, pad.position, pad_size, rotation_degrees, "pad");
          pad.component_id = requireOption(options, "--component");
          pad.pin_name = requireOption(options, "--pin");
          pad.net_id = requireOption(options, "--net");
          pad.padstack.layer_set = layers;
          if (options.contains("--type")) pad.type = requireOption(options, "--type");
          if (options.contains("--shape")) pad.padstack.copper_props["top"].shape.shape = parsePadShapeStr(options.at("--shape"));
          if (options.contains("--roundrect-rratio")) {
            pad.padstack.copper_props["top"].shape.roundrect_rratio = *optionalRatio(options, "--roundrect-rratio");
          }
          if (options.contains("--chamfer-ratio")) {
            pad.padstack.copper_props["top"].shape.chamfer_ratio = *optionalRatio(options, "--chamfer-ratio");
          }
          pad.rotation_degrees = rotation_degrees;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown pad: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-via") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--net", "--x-mm", "--y-mm", "--diameter-mm",
                                 "--drill-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniqueViaId(board, id);
      requireUniquePhysicalObjectId(board, id);
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "via position");
      const ccad::Length diameter = requirePositiveMillimeters(options, "--diameter-mm");
      const ccad::Length drill = requirePositiveMillimeters(options, "--drill-mm");
      if (drill.nanometers > diameter.nanometers) {
        throw std::runtime_error("via drill must be less than or equal to diameter");
      }
      requirePointWithMarginInsideBoard(board, position,
                                        ccad::nanometers(diameter.nanometers / 2), "via");
      board.vias.push_back(ccad::Via{
          .id = id,
          .net_id = requireOption(options, "--net"),
          .position = position,
          .diameter = diameter,
          .drill = drill,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-via") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--diameter-mm", "--drill-mm", "--net"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const ccad::Length diameter = requirePositiveMillimeters(options, "--diameter-mm");
      const ccad::Length drill = requirePositiveMillimeters(options, "--drill-mm");
      if (drill.nanometers > diameter.nanometers) {
        throw std::runtime_error("via drill must be less than or equal to diameter");
      }
      bool updated = false;
      for (ccad::Via& via : board.vias) {
        if (via.id == id) {
          requirePointWithMarginInsideBoard(board, via.position,
                                            ccad::nanometers(diameter.nanometers / 2), "via");
          if (options.contains("--net")) {
            via.net_id = requireOption(options, "--net");
          }
          via.diameter = diameter;
          via.drill = drill;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown via: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-track") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--net", "--layer", "--start-x-mm",
                                 "--start-y-mm", "--end-x-mm", "--end-y-mm", "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      requireUniqueTrackId(board, id);
      requireUniquePhysicalObjectId(board, id);
      requireCopperLayer(board, layer_id);
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      requireInsideBoard(board, start, "track start");
      requireInsideBoard(board, end, "track end");
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "track start");
      requirePointWithMarginInsideBoard(board, end, half_width, "track end");
      board.tracks.push_back(ccad::TrackSegment{
          .id = id,
          .net_id = requireOption(options, "--net"),
          .layer_id = layer_id,
          .start = start,
          .end = end,
          .width = width,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-track-arc") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--net", "--layer", "--start-x-mm",
                                 "--start-y-mm", "--mid-x-mm", "--mid-y-mm", "--end-x-mm", "--end-y-mm", "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      requireUniqueTrackId(board, id);
      requireUniquePhysicalObjectId(board, id);
      requireCopperLayer(board, layer_id);
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point mid{
          .x = requirePositiveMillimeters(options, "--mid-x-mm"),
          .y = requirePositiveMillimeters(options, "--mid-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      requireInsideBoard(board, start, "arc start");
      requireInsideBoard(board, mid, "arc mid");
      requireInsideBoard(board, end, "arc end");
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "arc start");
      requirePointWithMarginInsideBoard(board, mid, half_width, "arc mid");
      requirePointWithMarginInsideBoard(board, end, half_width, "arc end");
      board.track_arcs.push_back(ccad::TrackArc{
          .id = id,
          .net_id = requireOption(options, "--net"),
          .layer_id = layer_id,
          .start = start,
          .mid = mid,
          .end = end,
          .width = width,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-graphic-line") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--start-x-mm", "--start-y-mm",
                                 "--end-x-mm", "--end-y-mm", "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      requireUniquePhysicalObjectId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      if (start.x.nanometers == end.x.nanometers && start.y.nanometers == end.y.nanometers) {
        throw std::runtime_error("graphic line start and end must be different");
      }
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "graphic line start");
      requirePointWithMarginInsideBoard(board, end, half_width, "graphic line end");
      board.graphics.push_back(ccad::BoardGraphic{
          .id = id,
          .kind = "line",
          .layer_id = layer_id,
          .start = start,
          .end = end,
          .width = width,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-graphic-arc") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--start-x-mm", "--start-y-mm",
                                 "--mid-x-mm", "--mid-y-mm", "--end-x-mm", "--end-y-mm", "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      requireUniquePhysicalObjectId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point mid{
          .x = requirePositiveMillimeters(options, "--mid-x-mm"),
          .y = requirePositiveMillimeters(options, "--mid-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      requireInsideBoard(board, start, "arc start");
      requireInsideBoard(board, mid, "arc mid");
      requireInsideBoard(board, end, "arc end");
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "arc start");
      requirePointWithMarginInsideBoard(board, mid, half_width, "arc mid");
      requirePointWithMarginInsideBoard(board, end, half_width, "arc end");
      board.graphics.push_back(ccad::BoardGraphic{
          .id = id,
          .kind = "arc",
          .layer_id = layer_id,
          .start = start,
          .end = end,
          .mid = mid,
          .width = width,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-target") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--shape", "--x-mm", "--y-mm", "--size-mm", "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      requireUniquePhysicalObjectId(board, id);
      requireLayer(board, layer_id);
      
      const std::string shape_str = requireOption(options, "--shape");
      ccad::TargetShape shape = ccad::TargetShape::Plus;
      if (shape_str == "Plus") {
        shape = ccad::TargetShape::Plus;
      } else if (shape_str == "X") {
        shape = ccad::TargetShape::X;
      } else {
        throw std::runtime_error("invalid shape: " + shape_str);
      }

      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "target position");

      board.targets.push_back(ccad::BoardTarget{
          .id = id,
          .layer_id = layer_id,
          .shape = shape,
          .position_x = position.x,
          .position_y = position.y,
          .size = requirePositiveMillimeters(options, "--size-mm"),
          .line_width = requirePositiveMillimeters(options, "--width-mm"),
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-zone") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--name", "--net", "--layers",
                                 "--x-mm", "--y-mm", "--width-mm", "--height-mm",
                                 "--priority", "--clearance-mm",
                                 "--min-thickness-mm", "--pad-connection"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniquePhysicalObjectId(board, id);
      const std::vector<std::string> layers = splitLayers(requireOption(options, "--layers"));
      for (const std::string& l : layers) {
        if (!l.starts_with("*.")) requireLayer(board, l);
      }
      
      const double x = requireDoubleOption(options, "--x-mm");
      const double y = requireDoubleOption(options, "--y-mm");
      const double w = requireDoubleOption(options, "--width-mm");
      const double h = requireDoubleOption(options, "--height-mm");
      
      const std::vector<ccad::Point> points = {
        {ccad::millimeters(x), ccad::millimeters(y)},
        {ccad::millimeters(x + w), ccad::millimeters(y)},
        {ccad::millimeters(x + w), ccad::millimeters(y + h)},
        {ccad::millimeters(x), ccad::millimeters(y + h)}
      };
      for (const ccad::Point& point : points) {
        requireInsideBoard(board, point, "zone outline");
      }
      board.zones.push_back(ccad::BoardZone{
          .id = id,
          .name = options.contains("--name") ? options.at("--name") : "",
          .net_id = options.contains("--net") ? options.at("--net") : "",
          .layer_ids = layers,
          .outline = points,
          .priority = options.contains("--priority") ? requireNonNegativeIntOption(options, "--priority") : 0,
          .clearance = ccad::millimeters(optionDoubleOrDefault(options, "--clearance-mm", 0.508)),
          .min_thickness = ccad::millimeters(optionDoubleOrDefault(options, "--min-thickness-mm", 0.254)),
          .fill_enabled = true,
          .pad_connection = options.contains("--pad-connection") ? requireZonePadConnection(options) : "thermal"
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "refill-zones") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--zone-id", "--apply"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      const ccad::Board& board = requireBoard(project);
      const std::string requested_id = options.contains("--zone-id") ? options.at("--zone-id") : "";
      const bool apply = options.contains("--apply") && requireBoolOption(options, "--apply");
      std::ostringstream out;
      out << "{\n  \"zones\": [\n";
      bool first = true;
      for (const ccad::BoardZone& zone : board.zones) {
        if (!requested_id.empty() && zone.id != requested_id) continue;
        const ccad::ZoneFillResult fill = ccad::calculateZoneFill(
            zone, board.pads, ccad::millimeters(0.5), ccad::millimeters(0.5));
        if (!first) out << ",\n";
        first = false;
        out << "    {\n      \"id\": \"" << ccad::escapeJson(zone.id)
            << "\",\n      \"filled\": " << (fill.filled ? "true" : "false")
            << ",\n      \"contour_count\": " << fill.contours.size()
            << ",\n      \"area_square_nanometers\": " << fill.area_square_nanometers
            << ",\n      \"thermal_spoke_count\": " << fill.thermal_spokes.size()
            << ",\n      \"applied\": " << (apply && fill.filled ? "true" : "false")
            << ",\n      \"diagnostics\": [";
        for (std::size_t i = 0; i < fill.diagnostics.size(); ++i) {
          if (i) out << ", ";
          out << "\"" << ccad::escapeJson(fill.diagnostics.at(i)) << "\"";
        }
        out << "]\n    }";
        if (apply && fill.filled) {
          for (ccad::BoardZone& mutable_zone : project.boards.front().zones) {
            if (mutable_zone.id == zone.id) mutable_zone.filled_contours = fill.contours;
            if (mutable_zone.id == zone.id) {
              mutable_zone.filled_thermal_spokes.clear();
              for (const ccad::ZoneThermalSpoke& spoke : fill.thermal_spokes)
                mutable_zone.filled_thermal_spokes.push_back(
                    {spoke.start, spoke.end, spoke.width});
            }
          }
        }
      }
      if (!requested_id.empty() && first) throw std::runtime_error("unknown zone: " + requested_id);
      out << "\n  ]\n}\n";
      if (apply) {
        context.markDirty();
        if (!writeProjectFile(file, project))
          throw std::runtime_error("failed to write refilled project");
      }
      std::cout << out.str();
      return 0;
    }

    if (subcommand == "add-text") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--text", "--x-mm", "--y-mm",
                                 "--size-x-mm", "--size-y-mm", "--rotation-deg", "--mirrored", "--stroke-width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      const std::string text = requireOption(options, "--text");
      if (text.empty()) {
        throw std::runtime_error("--text must not be empty");
      }
      requireUniquePhysicalObjectId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "text position");
      const ccad::Size size{.width = requirePositiveMillimeters(options, "--size-x-mm"),
                            .height = requirePositiveMillimeters(options, "--size-y-mm")};
      board.texts.push_back(ccad::BoardText{
          .id = id,
          .layer_id = layer_id,
          .text = text,
          .position = position,
          .rotation_degrees = requireDoubleOption(options, "--rotation-deg"),
          .size = size,
      });
      setOptionalBool(options, "--mirrored", board.texts.back().mirrored);
      setOptionalMillimeters(options, "--stroke-width-mm", board.texts.back().stroke_width);
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-barcode") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--text", "--kind", "--error-correction", "--x-mm", "--y-mm",
                                 "--size-x-mm", "--size-y-mm", "--margin-x-mm", "--margin-y-mm", "--rotation-deg"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      const std::string text = requireOption(options, "--text");
      if (text.empty()) {
        throw std::runtime_error("--text must not be empty");
      }
      requireUniquePhysicalObjectId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      requireInsideBoard(board, position, "barcode position");
      const ccad::Size size{.width = requirePositiveMillimeters(options, "--size-x-mm"),
                            .height = requirePositiveMillimeters(options, "--size-y-mm")};
      const ccad::Size margin{.width = options.contains("--margin-x-mm") ? requirePositiveMillimeters(options, "--margin-x-mm") : ccad::Length{0},
                              .height = options.contains("--margin-y-mm") ? requirePositiveMillimeters(options, "--margin-y-mm") : ccad::Length{0}};
      
      ccad::BarcodeType kind = ccad::parseBarcodeType(requireOption(options, "--kind"));
      ccad::BarcodeEcc error_correction = ccad::parseBarcodeEcc(options.contains("--error-correction") ? requireOption(options, "--error-correction") : "Low");

      board.barcodes.push_back(ccad::BoardBarcode{
          .id = id,
          .layer_id = layer_id,
          .text = text,
          .kind = kind,
          .error_correction = error_correction,
          .position = position,
          .rotation_degrees = optionDoubleOrDefault(options, "--rotation-deg", 0.0),
          .size = size,
          .margin = margin,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-dimension") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--kind", "--text", "--start-x-mm", "--start-y-mm",
                                 "--end-x-mm", "--end-y-mm", "--text-x-mm", "--text-y-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string layer_id = requireOption(options, "--layer");
      const std::string kind = requireOption(options, "--kind");
      const std::string text = requireOption(options, "--text");
      requireUniquePhysicalObjectId(board, id);
      requireLayer(board, layer_id);
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      const ccad::Point text_position{
          .x = requirePositiveMillimeters(options, "--text-x-mm"),
          .y = requirePositiveMillimeters(options, "--text-y-mm"),
      };
      requireInsideBoard(board, start, "dimension start");
      requireInsideBoard(board, end, "dimension end");
      requireInsideBoard(board, text_position, "dimension text position");

      board.dimensions.push_back(ccad::BoardDimension{
          .id = id,
          .layer_id = layer_id,
          .kind = kind,
          .text = text,
          .start = start,
          .end = end,
          .text_position = text_position,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-group") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--name", "--members"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniquePhysicalObjectId(board, id);
      
      const std::string members_str = requireOption(options, "--members");
      std::vector<std::string> members;
      if (!members_str.empty()) {
        size_t pos = 0;
        size_t next;
        while ((next = members_str.find(',', pos)) != std::string::npos) {
          members.push_back(members_str.substr(pos, next - pos));
          pos = next + 1;
        }
        members.push_back(members_str.substr(pos));
      }

      for (const std::string& member_id : members) {
        requireBoardObjectId(board, member_id);
      }

      board.groups.push_back(ccad::BoardGroup{
          .id = id,
          .name = options.contains("--name") ? options.at("--name") : "",
          .members = members,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-route-request") {
      const std::map<std::string, std::string> options = parseOptions(
          args, 1,
          {"--file", "--id", "--net", "--from", "--to", "--preferred-layer", "--policy",
           "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string from_object_id = requireOption(options, "--from");
      const std::string to_object_id = requireOption(options, "--to");
      const std::string preferred_layer_id = requireOption(options, "--preferred-layer");
      requireUniqueRouteRequestId(board, id);
      requireBoardObjectId(board, from_object_id);
      requireBoardObjectId(board, to_object_id);
      requireCopperLayer(board, preferred_layer_id);
      board.route_requests.push_back(ccad::RouteRequest{
          .id = id,
          .net_id = requireOption(options, "--net"),
          .from_object_id = from_object_id,
          .to_object_id = to_object_id,
          .preferred_layer_id = preferred_layer_id,
          .policy = requireOption(options, "--policy"),
          .width = requirePositiveMillimeters(options, "--width-mm"),
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-route-request") {
      const std::map<std::string, std::string> options = parseOptions(
          args, 1,
          {"--file", "--id", "--net", "--from", "--to", "--preferred-layer", "--policy",
           "--width-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string from_object_id = requireOption(options, "--from");
      const std::string to_object_id = requireOption(options, "--to");
      const std::string preferred_layer_id = requireOption(options, "--preferred-layer");
      requireBoardObjectId(board, from_object_id);
      requireBoardObjectId(board, to_object_id);
      requireCopperLayer(board, preferred_layer_id);
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      bool updated = false;
      for (ccad::RouteRequest& request : board.route_requests) {
        if (request.id == id) {
          request.net_id = requireOption(options, "--net");
          request.from_object_id = from_object_id;
          request.to_object_id = to_object_id;
          request.preferred_layer_id = preferred_layer_id;
          request.policy = requireOption(options, "--policy");
          request.width = width;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown route request: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "remove-route-request") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const auto old_size = board.route_requests.size();
      std::erase_if(board.route_requests,
                    [&id](const ccad::RouteRequest& request) { return request.id == id; });
      if (board.route_requests.size() == old_size) {
        throw std::runtime_error("unknown route request: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "apply-route-segment") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--request-id", "--track-id", "--layer",
                                 "--start-x-mm", "--start-y-mm", "--end-x-mm",
                                 "--end-y-mm", "--complete"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string request_id = requireOption(options, "--request-id");
      const std::string track_id = requireOption(options, "--track-id");
      const bool complete_request = parseCompleteOption(options);
      requireUniqueTrackId(board, track_id);
      requireUniquePhysicalObjectId(board, track_id);
      auto request_it = std::find_if(
          board.route_requests.begin(), board.route_requests.end(),
          [&request_id](const ccad::RouteRequest& request) { return request.id == request_id; });
      if (request_it == board.route_requests.end()) {
        throw std::runtime_error("unknown route request: " + request_id);
      }
      const std::string layer_id =
          options.contains("--layer") ? requireOption(options, "--layer")
                                      : request_it->preferred_layer_id;
      requireCopperLayer(board, layer_id);
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      const ccad::Length width = request_it->width;
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "route segment start");
      requirePointWithMarginInsideBoard(board, end, half_width, "route segment end");
      board.tracks.push_back(ccad::TrackSegment{
          .id = track_id,
          .net_id = request_it->net_id,
          .layer_id = layer_id,
          .start = start,
          .end = end,
          .width = width,
          .source_route_request_id = request_id,
      });
      if (complete_request) {
        board.route_requests.erase(request_it);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "apply-route-polyline") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--request-id", "--track-prefix", "--layer",
                                 "--points-mm", "--complete"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string request_id = requireOption(options, "--request-id");
      const std::string track_prefix = requireOption(options, "--track-prefix");
      if (track_prefix.empty()) {
        throw std::runtime_error("--track-prefix must not be empty");
      }
      const bool complete_request = parseCompleteOption(options);
      auto request_it = std::find_if(
          board.route_requests.begin(), board.route_requests.end(),
          [&request_id](const ccad::RouteRequest& request) { return request.id == request_id; });
      if (request_it == board.route_requests.end()) {
        throw std::runtime_error("unknown route request: " + request_id);
      }
      const std::string layer_id =
          options.contains("--layer") ? requireOption(options, "--layer")
                                      : request_it->preferred_layer_id;
      requireCopperLayer(board, layer_id);
      const std::vector<ccad::Point> points =
          parsePolylinePointsMm(requireOption(options, "--points-mm"));
      const ccad::Length width = request_it->width;
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        const std::string track_id = track_prefix + "." + std::to_string(index + 1);
        requireUniqueTrackId(board, track_id);
        requireUniquePhysicalObjectId(board, track_id);
        if (points.at(index).x.nanometers == points.at(index + 1).x.nanometers &&
            points.at(index).y.nanometers == points.at(index + 1).y.nanometers) {
          throw std::runtime_error("route polyline contains a zero-length segment");
        }
        requirePointWithMarginInsideBoard(board, points.at(index), half_width,
                                          "route polyline point");
        requirePointWithMarginInsideBoard(board, points.at(index + 1), half_width,
                                          "route polyline point");
      }
      for (std::size_t index = 0; index + 1 < points.size(); ++index) {
        board.tracks.push_back(ccad::TrackSegment{
            .id = track_prefix + "." + std::to_string(index + 1),
            .net_id = request_it->net_id,
            .layer_id = layer_id,
            .start = points.at(index),
            .end = points.at(index + 1),
            .width = width,
            .source_route_request_id = request_id,
        });
      }
      if (complete_request) {
        board.route_requests.erase(request_it);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-track") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--start-x-mm", "--start-y-mm",
                                 "--end-x-mm", "--end-y-mm", "--width-mm", "--net",
                                 "--layer"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      if (options.contains("--layer")) {
        requireCopperLayer(board, requireOption(options, "--layer"));
      }
      const ccad::Point start{
          .x = requirePositiveMillimeters(options, "--start-x-mm"),
          .y = requirePositiveMillimeters(options, "--start-y-mm"),
      };
      const ccad::Point end{
          .x = requirePositiveMillimeters(options, "--end-x-mm"),
          .y = requirePositiveMillimeters(options, "--end-y-mm"),
      };
      const ccad::Length width = requirePositiveMillimeters(options, "--width-mm");
      const ccad::Length half_width = ccad::nanometers(width.nanometers / 2);
      requirePointWithMarginInsideBoard(board, start, half_width, "track start");
      requirePointWithMarginInsideBoard(board, end, half_width, "track end");
      bool updated = false;
      for (ccad::TrackSegment& track : board.tracks) {
        if (track.id == id) {
          if (options.contains("--net")) {
            track.net_id = requireOption(options, "--net");
          }
          if (options.contains("--layer")) {
            track.layer_id = requireOption(options, "--layer");
          }
          track.start = start;
          track.end = end;
          track.width = width;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown track: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-keepout") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--kind", "--x-mm", "--y-mm", "--width-mm",
                                 "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniqueKeepoutId(board, id);
      requireUniquePhysicalObjectId(board, id);
      const ccad::Rect area{
          .origin = ccad::Point{.x = requirePositiveMillimeters(options, "--x-mm"),
                                .y = requirePositiveMillimeters(options, "--y-mm")},
          .size = ccad::Size{.width = requirePositiveMillimeters(options, "--width-mm"),
                             .height = requirePositiveMillimeters(options, "--height-mm")},
      };
      requireRectInsideBoard(board, area, "keepout area");
      board.keepouts.push_back(ccad::Keepout{
          .id = id,
          .kind = requireOption(options, "--kind"),
          .area = area,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "update-teardrops") {
      const std::map<std::string, std::string> options = parseOptions(args, 1,
          {"--file", "--enable-pads", "--enable-vias", "--curved", "--smd"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const auto flag = [&](const std::string& key, bool fallback) {
        const auto it = options.find(key);
        if (it == options.end()) return fallback;
        if (it->second == "true") return true;
        if (it->second == "false") return false;
        throw std::runtime_error(key + " must be true or false");
      };
      // Parse all settings before changing the in-memory project.
      const bool pads = flag("--enable-pads", false);
      const bool vias = flag("--enable-vias", false);
      ccad::TeardropGenerator::TeardropSettings settings;
      settings.curvedEdges = flag("--curved", false);
      settings.targetSMDPads = flag("--smd", false);
      if (options.contains("--enable-pads"))
        for (auto& pad : board.pads) pad.teardrops_enabled = pads;
      if (options.contains("--enable-vias"))
        for (auto& via : board.vias) via.teardrops_enabled = vias;
      ccad::TeardropGenerator generator(&board);
      generator.setSettings(settings);
      generator.generateTeardrops();
      context.markDirty();

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      std::cout << "{\"teardrops\":" << board.teardrops.size()
                << ",\"clearance_verified\":false,\"fabrication_export_supported\":false}\n";
      return 0;
    }

    if (subcommand == "add-zone") {
      const std::map<std::string, std::string> options = parseOptions(
          args, 1,
          {"--file", "--id", "--name", "--net", "--layers", "--x-mm", "--y-mm",
           "--width-mm", "--height-mm", "--priority", "--clearance-mm", "--min-thickness-mm",
           "--pad-connection"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniquePhysicalObjectId(board, id);
      const std::vector<std::string> layer_ids = splitLayers(requireOption(options, "--layers"));
      if (layer_ids.empty()) {
        throw std::runtime_error("--layers must include at least one copper layer");
      }
      for (const std::string& layer_id : layer_ids) {
        if (layer_id.empty()) {
          throw std::runtime_error("--layers must not include an empty layer");
        }
        requireCopperLayer(board, layer_id);
      }
      std::string net_id;
      if (options.contains("--net")) {
        net_id = requireOption(options, "--net");
        if (!projectHasNet(project, net_id)) {
          throw std::runtime_error("unknown net: " + net_id);
        }
      }
      const ccad::Point origin{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };
      const ccad::Size size{.width = requirePositiveMillimeters(options, "--width-mm"),
                            .height = requirePositiveMillimeters(options, "--height-mm")};
      const ccad::Rect area{.origin = origin, .size = size};
      requireRectInsideBoard(board, area, "zone area");
      board.zones.push_back(ccad::BoardZone{
          .id = id,
          .name = options.contains("--name") ? requireOption(options, "--name") : "",
          .net_id = net_id,
          .layer_ids = layer_ids,
          .outline = {origin,
                      ccad::Point{.x = ccad::nanometers(origin.x.nanometers +
                                                        size.width.nanometers),
                                  .y = origin.y},
                      ccad::Point{.x = ccad::nanometers(origin.x.nanometers +
                                                        size.width.nanometers),
                                  .y = ccad::nanometers(origin.y.nanometers +
                                                        size.height.nanometers)},
                      ccad::Point{.x = origin.x,
                                  .y = ccad::nanometers(origin.y.nanometers +
                                                        size.height.nanometers)}},
          .filled_contours = {},
          .priority = requireNonNegativeIntOption(options, "--priority"),
          .clearance = requirePositiveMillimeters(options, "--clearance-mm"),
          .min_thickness = requirePositiveMillimeters(options, "--min-thickness-mm"),
          .fill_enabled = true,
          .pad_connection = requireZonePadConnection(options),
      });
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-placement-region") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--kind", "--x-mm", "--y-mm", "--width-mm",
                                 "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      requireUniquePlacementRegionId(board, id);
      requireUniquePhysicalObjectId(board, id);
      const ccad::Rect area{
          .origin = ccad::Point{.x = requirePositiveMillimeters(options, "--x-mm"),
                                .y = requirePositiveMillimeters(options, "--y-mm")},
          .size = ccad::Size{.width = requirePositiveMillimeters(options, "--width-mm"),
                             .height = requirePositiveMillimeters(options, "--height-mm")},
      };
      requireRectInsideBoard(board, area, "placement region area");
      board.placement_regions.push_back(ccad::PlacementRegion{
          .id = id,
          .kind = requireOption(options, "--kind"),
          .area = area,
      });
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "set-region-kind") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--kind"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const std::string kind = requireOption(options, "--kind");
      bool updated = false;
      for (ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          keepout.kind = kind;
          updated = true;
          break;
        }
      }
      for (ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          region.kind = kind;
          updated = true;
          break;
        }
      }
      if (!updated) {
        throw std::runtime_error("unknown region: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "remove-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--mode"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const ccad::BoardContainerRemoveResult result =
          ccad::removeBoardContainerItem(board, id, parseRemoveModeOption(options));
      if (!result.removed) {
        throw std::runtime_error("unknown physical object: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      std::cout << removeObjectResultJson(result);
      return 0;
    }

    if (subcommand == "move-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--x-mm", "--y-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const ccad::Point position{
          .x = requirePositiveMillimeters(options, "--x-mm"),
          .y = requirePositiveMillimeters(options, "--y-mm"),
      };

      bool moved = false;
      for (ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          ccad::Size pad_size = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
          requireRotatedRectInsideBoard(board, position, pad_size, pad.rotation_degrees, "pad");
          pad.position = position;
          moved = true;
          break;
        }
      }
      for (ccad::Via& via : board.vias) {
        if (via.id == id) {
          requirePointWithMarginInsideBoard(board, position,
                                            ccad::nanometers(via.diameter.nanometers / 2),
                                            "via");
          via.position = position;
          moved = true;
          break;
        }
      }
      for (ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          const ccad::Rect moved_area{.origin = position, .size = keepout.area.size};
          requireRectInsideBoard(board, moved_area, "keepout area");
          keepout.area.origin = position;
          moved = true;
          break;
        }
      }
      for (ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          const ccad::Rect moved_area{.origin = position, .size = region.area.size};
          requireRectInsideBoard(board, moved_area, "placement region area");
          region.area.origin = position;
          moved = true;
          break;
        }
      }
      if (!moved) {
        throw std::runtime_error("unknown movable physical object: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "resize-object") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--width-mm", "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::HeadlessBoardContext context;
      context.loadFile(file);
      ccad::Project& project = context.project();
      ccad::Board& board = requireBoard(project);
      const std::string id = requireOption(options, "--id");
      const ccad::Size size{.width = requirePositiveMillimeters(options, "--width-mm"),
                            .height = requirePositiveMillimeters(options, "--height-mm")};

      bool resized = false;
      for (ccad::Pad& pad : board.pads) {
        if (pad.id == id) {
          requireRotatedRectInsideBoard(board, pad.position, size, pad.rotation_degrees, "pad");
          if (!pad.padstack.copper_props.empty()) {
            pad.padstack.copper_props.begin()->second.shape.size = size;
          } else {
            pad.padstack.copper_props["top"].shape.size = size;
          }
          resized = true;
          break;
        }
      }
      for (ccad::Keepout& keepout : board.keepouts) {
        if (keepout.id == id) {
          const ccad::Rect resized_area{.origin = keepout.area.origin, .size = size};
          requireRectInsideBoard(board, resized_area, "keepout area");
          keepout.area.size = size;
          resized = true;
          break;
        }
      }
      for (ccad::PlacementRegion& region : board.placement_regions) {
        if (region.id == id) {
          const ccad::Rect resized_area{.origin = region.area.origin, .size = size};
          requireRectInsideBoard(board, resized_area, "placement region area");
          region.area.size = size;
          resized = true;
          break;
        }
      }
      if (!resized) {
        throw std::runtime_error("unknown resizable physical object: " + id);
      }
      context.markDirty();
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "export-kicad") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--output"});
      const std::string file = requireOption(options, "--file");
      const std::string output = requireOption(options, "--output");
      ccad::Project project = loadProjectFile(file);
      const std::string exported = ccad::exportToKiCadPcb(project);
      std::ofstream out(ccad::u8ToPath(output));
      if (!out) {
        throw std::runtime_error("failed to open output file: " + output);
      }
      out << exported;
      return 0;
    }

    if (subcommand == "export-dsn") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--output"});
      const std::string file = requireOption(options, "--file");
      const std::string output = requireOption(options, "--output");
      ccad::Project project = loadProjectFile(file);
      const std::string exported = ccad::exportSpecctraDsn(project);
      std::ofstream out(ccad::u8ToPath(output));
      if (!out) {
        throw std::runtime_error("failed to open output file: " + output);
      }
      out << exported;
      return 0;
    }

    if (subcommand == "import-ses") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--input"});
      const std::string file = requireOption(options, "--file");
      const std::string input = requireOption(options, "--input");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      
      std::ifstream in(ccad::u8ToPath(input));
      if (!in) {
        throw std::runtime_error("failed to open input file: " + input);
      }
      std::stringstream buffer;
      buffer << in.rdbuf();
      
      ccad::SesRouting routing = ccad::importSpecctraSes(buffer.str());
      
      for (auto& track : routing.tracks) {
        // give unique IDs relative to existing tracks
        track.id += "_" + std::to_string(board.tracks.size());
        board.tracks.push_back(track);
      }
      for (auto& via : routing.vias) {
        via.id += "_" + std::to_string(board.vias.size());
        board.vias.push_back(via);
      }
      
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "export-board-bom") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--output"});
      const std::string file = requireOption(options, "--file");
      const std::string output = requireOption(options, "--output");
      const ccad::Project project = loadProjectFile(file);
      const std::string exported = ccad::exportBoardToBomCsv(project);
      std::ofstream out(ccad::u8ToPath(output));
      if (!out) {
        throw std::runtime_error("failed to open output file: " + output);
      }
      out << exported;
      return 0;
    }

    if (subcommand == "export-pnp") {
      auto opts = parseOptions(args, 1, {"--file", "--output"});
      auto file = requireOption(opts, "--file");
      auto out = requireOption(opts, "--output");
      ccad::Project project = loadProjectFile(file);
      std::string pnp = ccad::exportToPnpCsv(project);
      std::ofstream out_file(ccad::u8ToPath(out));
      if (!out_file) {
        throw std::runtime_error("failed to open output file: " + out);
      }
      out_file << pnp;
      return 0;
    }

    if (subcommand == "export-drill") {
      auto opts = parseOptions(args, 1, {"--file", "--output"});
      auto file = requireOption(opts, "--file");
      auto out = requireOption(opts, "--output");
      ccad::Project project = loadProjectFile(file);
      std::string drill = ccad::exportToDrillExcellon(project);
      std::ofstream out_file(ccad::u8ToPath(out));
      if (!out_file) {
        throw std::runtime_error("failed to open output file: " + out);
      }
      out_file << drill;
      return 0;
    }

    if (subcommand == "autoplace-footprint") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--footprint", "--component", "--layer", "--grid-mm",
                                 "--rotation-deg"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const ccad::Footprint footprint = loadFootprintFile(requireOption(options, "--footprint"));
      const std::string component_id = requireOption(options, "--component");
      const std::string layer_id = requireOption(options, "--layer");
      requireCopperLayer(board, layer_id);
      const ccad::Length grid_step = options.contains("--grid-mm")
                                         ? requirePositiveMillimeters(options, "--grid-mm")
                                         : ccad::millimeters(1.0);
      const double placement_rotation = optionDoubleOrDefault(options, "--rotation-deg", 0.0);

      const ccad::AutoPlacementPlan plan = ccad::planFootprintAutoPlacement(
          board, footprint, footprintPadNetMap(project, footprint, component_id), layer_id,
          grid_step);
      if (!plan.placeable) {
        throw std::runtime_error("autoplace failed: " + plan.reason);
      }

      ccad::placeFootprint(project, footprint, component_id, plan.origin, placement_rotation,
                           layer_id);

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      std::cout << autoplaceResultJson(component_id, layer_id, plan);
      return 0;
    }

    if (subcommand == "spread-footprints") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--symbols", "--target-x-mm", "--target-y-mm",
                                 "--component-gap-mm", "--group-gap-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      const std::vector<std::string> component_ids =
          options.contains("--symbols")
              ? splitCommaList(requireOption(options, "--symbols"), "--symbols")
              : std::vector<std::string>{};
      const ccad::SpreadFootprintRequest request{
          .component_ids = component_ids,
          .target = {.x = requirePositiveMillimeters(options, "--target-x-mm"),
                     .y = requirePositiveMillimeters(options, "--target-y-mm")},
          .component_gap = options.contains("--component-gap-mm")
                               ? requirePositiveMillimeters(options, "--component-gap-mm")
                               : ccad::millimeters(1.0),
          .group_gap = options.contains("--group-gap-mm")
                           ? requirePositiveMillimeters(options, "--group-gap-mm")
                           : ccad::millimeters(1.5),
      };

      const std::vector<ccad::SpreadFootprintPlacement> placements =
          ccad::spreadFootprintComponents(board, request);
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      std::cout << spreadFootprintsResultJson(placements);
      return 0;
    }

    if (subcommand == "add-reference-image") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--data", "--x-mm", "--y-mm", "--scale", "--opacity"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);
      
      ccad::BoardReferenceImage obj;
      obj.id = requireOption(options, "--id");
      obj.layer = requireOption(options, "--layer");
      obj.data = requireOption(options, "--data");
      obj.x_mm = requireDoubleOption(options, "--x-mm");
      obj.y_mm = requireDoubleOption(options, "--y-mm");
      obj.scale = requireDoubleOption(options, "--scale");
      obj.opacity = requireDoubleOption(options, "--opacity");
      board.reference_images.push_back(obj);
      
      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "add-table") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--id", "--layer", "--x-mm", "--y-mm", "--rows", "--cols", "--width-mm", "--height-mm"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);

      ccad::BoardTable obj;
      obj.id = requireOption(options, "--id");
      obj.layer = requireOption(options, "--layer");
      obj.x_mm = requireDoubleOption(options, "--x-mm");
      obj.y_mm = requireDoubleOption(options, "--y-mm");
      obj.rows = (int)requireDoubleOption(options, "--rows");
      obj.cols = (int)requireDoubleOption(options, "--cols");
      obj.width_mm = requireDoubleOption(options, "--width-mm");
      obj.height_mm = requireDoubleOption(options, "--height-mm");
      board.tables.push_back(obj);

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    if (subcommand == "item-geometry") {
      const std::map<std::string, std::string> options = parseOptions(
          args, 1, {"--file", "--id", "--hit-test-x-mm", "--hit-test-y-mm", "--hit-test-accuracy-nm"});
      const std::string file = requireOption(options, "--file");
      const std::string id = requireOption(options, "--id");
      ccad::Project project = loadProjectFile(file);
      ccad::Board& board = requireBoard(project);

      const ccad::Pad* pad = nullptr;
      const ccad::Via* via = nullptr;
      const ccad::TrackSegment* track = nullptr;
      const ccad::TrackArc* arc = nullptr;
      const ccad::BoardZone* zone = nullptr;

      for (const auto& p : board.pads) if (p.id == id) pad = &p;
      for (const auto& v : board.vias) if (v.id == id) via = &v;
      for (const auto& t : board.tracks) if (t.id == id) track = &t;
      for (const auto& a : board.track_arcs) if (a.id == id) arc = &a;
      for (const auto& z : board.zones) if (z.id == id) zone = &z;

      ccad::BoundingBox bb;
      double length = 0.0;
      int64_t annular_ring = 0;
      bool has_hit_test = false;
      bool hit_test_result = false;

      std::optional<ccad::Point> hit_test_point;
      if (options.contains("--hit-test-x-mm") && options.contains("--hit-test-y-mm")) {
        hit_test_point = ccad::Point{
            .x = ccad::millimeters(std::stod(options.at("--hit-test-x-mm"))),
            .y = ccad::millimeters(std::stod(options.at("--hit-test-y-mm")))};
      }
      int64_t accuracy = options.contains("--hit-test-accuracy-nm") ? std::stoll(options.at("--hit-test-accuracy-nm")) : 0;

      std::string item_type = "unknown";

      if (pad) {
        item_type = "pad";
        bb = ccad::itemBoundingBox(*pad);
        annular_ring = ccad::padAnnularRing(*pad);
        if (hit_test_point) {
          has_hit_test = true;
          hit_test_result = ccad::itemHitTest(*pad, *hit_test_point);
        }
      } else if (via) {
        item_type = "via";
        bb = ccad::itemBoundingBox(*via);
        annular_ring = ccad::viaAnnularRing(*via);
        if (hit_test_point) {
          has_hit_test = true;
          hit_test_result = ccad::itemHitTest(*via, *hit_test_point);
        }
      } else if (track) {
        item_type = "track";
        bb = ccad::itemBoundingBox(*track);
        length = ccad::itemLength(*track);
        if (hit_test_point) {
          has_hit_test = true;
          hit_test_result = ccad::itemHitTest(*track, *hit_test_point, accuracy);
        }
      } else if (arc) {
        item_type = "track_arc";
        bb = ccad::itemBoundingBox(*arc);
        length = ccad::itemLength(*arc);
        if (hit_test_point) {
          has_hit_test = true;
          hit_test_result = ccad::itemHitTest(*arc, *hit_test_point, accuracy);
        }
      } else if (zone) {
        item_type = "zone";
        bb = ccad::itemBoundingBox(*zone);
        if (hit_test_point) {
          has_hit_test = true;
          hit_test_result = ccad::itemHitTest(*zone, *hit_test_point);
        }
      } else {
        std::cerr << "item not found or unsupported for geometry: " << id << '\n';
        return 2;
      }

      std::cout << "{\n"
                << "  \"id\": \"" << ccad::escapeJson(id) << "\",\n"
                << "  \"type\": \"" << ccad::escapeJson(item_type) << "\",\n"
                << "  \"bounding_box\": {\n"
                << "    \"valid\": " << (bb.valid ? "true" : "false") << ",\n"
                << "    \"min_x_nm\": " << bb.min.x.nanometers << ",\n"
                << "    \"min_y_nm\": " << bb.min.y.nanometers << ",\n"
                << "    \"max_x_nm\": " << bb.max.x.nanometers << ",\n"
                << "    \"max_y_nm\": " << bb.max.y.nanometers << "\n"
                << "  },\n"
                << "  \"length_mm\": " << length << ",\n"
                << "  \"annular_ring_nm\": " << annular_ring << ",\n"
                << "  \"hit_test\": {\n"
                << "    \"executed\": " << (has_hit_test ? "true" : "false") << ",\n"
                << "    \"hit\": " << (hit_test_result ? "true" : "false") << "\n"
                << "  }\n"
                << "}\n";
      return 0;
    }

    if (subcommand == "place-footprint") {
      const std::map<std::string, std::string> options =
          parseOptions(args, 1, {"--file", "--footprint", "--component", "--at-x-mm", "--at-y-mm",
                                 "--layer", "--rotation-deg", "--value", "--exclude-from-bom"});
      const std::string file = requireOption(options, "--file");
      ccad::Project project = loadProjectFile(file);
      const ccad::Footprint footprint = loadFootprintFile(requireOption(options, "--footprint"));
      const std::string component_id = requireOption(options, "--component");
      const std::string layer_id = requireOption(options, "--layer");
      const ccad::Point origin{
          .x = requirePositiveMillimeters(options, "--at-x-mm"),
          .y = requirePositiveMillimeters(options, "--at-y-mm"),
      };
      const double placement_rotation = optionDoubleOrDefault(options, "--rotation-deg", 0.0);
      const std::optional<std::string> value =
          options.contains("--value") ? std::optional<std::string>(requireOption(options, "--value"))
                                      : std::nullopt;
      const bool exclude_from_bom = options.contains("--exclude-from-bom")
                                        ? requireBoolOption(options, "--exclude-from-bom")
                                        : false;

      ccad::placeFootprint(project, footprint, component_id, origin, placement_rotation, layer_id,
                           value, exclude_from_bom);

      if (!writeProjectFile(file, project)) {
        std::cerr << "failed to write project file: " << file << '\n';
        return 2;
      }
      return 0;
    }

    std::cerr << "unknown pcb subcommand: " << subcommand << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "failed to mutate pcb project: " << error.what() << '\n';
    return 2;
  }
}

}  // namespace ccad_cli
