#include "ccad_cli/pcb_object_queries.hpp"

#include "ccad_core/json.hpp"
#include "ccad_core/layers.hpp"

#include <map>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad_cli {
namespace {

struct PhysicalNetSummary {
  std::string id;
  int pad_count = 0;
  int via_count = 0;
  int track_count = 0;
};

struct ConnectableSummary {
  int pad_count = 0;
  int via_count = 0;
  int track_count = 0;
  int zone_count = 0;
};

struct LayerQuerySummary {
  int copper_count = 0;
  int non_copper_count = 0;
  int visible_count = 0;
  int hidden_count = 0;
};

struct ConnectableSource {
  std::string type;
  std::string id;
  std::string net_id;
};

void writePointJson(std::ostream& out, const ccad::Point& point, const int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << pad << "\"x_nm\": " << point.x.nanometers << ",\n";
  out << pad << "\"y_nm\": " << point.y.nanometers;
}

void writeSizeJson(std::ostream& out, const ccad::Size& size, const int indent) {
  const std::string pad(static_cast<std::size_t>(indent), ' ');
  out << pad << "\"width_nm\": " << size.width.nanometers << ",\n";
  out << pad << "\"height_nm\": " << size.height.nanometers;
}

bool includeObjectType(const std::string& filter, const std::string& type) {
  return filter.empty() || filter == type;
}

bool includeConnectableType(const std::string& filter, const std::string& type) {
  return filter.empty() || filter == type;
}

std::string firstLayerId(const std::vector<std::string>& layers) {
  return layers.empty() ? "" : layers.front();
}

void appendLayerIdsJson(std::ostream& out, const std::vector<std::string>& layer_ids) {
  out << "[";
  for (std::size_t i = 0; i < layer_ids.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << "\"" << ccad::escapeJson(layer_ids.at(i)) << "\"";
  }
  out << "]";
}

void appendLayerNumbersJson(std::ostream& out, const std::vector<std::size_t>& layer_numbers) {
  out << "[";
  for (std::size_t i = 0; i < layer_numbers.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }
    out << layer_numbers.at(i);
  }
  out << "]";
}

std::vector<std::string> resolvedPadLayers(const ccad::Board& board, const ccad::Pad& pad) {
  return ccad::expandKiCadLayerSet(pad.layers, board);
}

std::string padNetRowJson(const ccad::Board& board, const ccad::Pad& pad) {
  const std::vector<std::string> resolved_layers = resolvedPadLayers(board, pad);
  const std::vector<std::size_t> kicad_layer_numbers =
      ccad::standardKiCadPcbLayerNumbersForSet(resolved_layers);
  std::ostringstream row;
  row << "    {\"type\": \"pad\", \"id\": \"" << ccad::escapeJson(pad.id)
      << "\", \"component_id\": \"" << ccad::escapeJson(pad.component_id)
      << "\", \"pin_name\": \"" << ccad::escapeJson(pad.pin_name)
      << "\", \"net_id\": \"" << ccad::escapeJson(pad.net_id)
      << "\", \"pad_type\": \"" << ccad::escapeJson(pad.type)
      << "\", \"shape\": \"" << ccad::escapeJson(pad.shape)
      << "\", \"layer_id\": \"" << ccad::escapeJson(firstLayerId(pad.layers))
      << "\", \"resolved_layers\": ";
  appendLayerIdsJson(row, resolved_layers);
  row << ", \"kicad_layer_numbers\": ";
  appendLayerNumbersJson(row, kicad_layer_numbers);
  if (pad.drill.has_value()) {
    row << ", \"drill_nm\": " << pad.drill->nanometers;
  }
  row << "}";
  return row.str();
}

std::string viaNetRowJson(const ccad::Via& via) {
  std::ostringstream row;
  row << "    {\"type\": \"via\", \"id\": \"" << ccad::escapeJson(via.id)
      << "\", \"net_id\": \"" << ccad::escapeJson(via.net_id) << "\"}";
  return row.str();
}

std::string trackNetRowJson(const ccad::TrackSegment& track) {
  std::ostringstream row;
  row << "    {\"type\": \"track\", \"id\": \"" << ccad::escapeJson(track.id)
      << "\", \"net_id\": \"" << ccad::escapeJson(track.net_id)
      << "\", \"layer_id\": \"" << ccad::escapeJson(track.layer_id)
      << "\", \"source_route_request_id\": \""
      << ccad::escapeJson(track.source_route_request_id) << "\"}";
  return row.str();
}

std::string zoneNetRowJson(const ccad::BoardZone& zone) {
  std::ostringstream row;
  row << "    {\"type\": \"zone\", \"id\": \"" << ccad::escapeJson(zone.id)
      << "\", \"name\": \"" << ccad::escapeJson(zone.name)
      << "\", \"net_id\": \"" << ccad::escapeJson(zone.net_id)
      << "\", \"layer_ids\": ";
  appendLayerIdsJson(row, zone.layer_ids);
  row << ", \"corner_count\": " << zone.outline.size() << ", \"priority\": " << zone.priority
      << "}";
  return row.str();
}

void countLayer(const ccad::Layer& layer, LayerQuerySummary& summary) {
  if (layer.kind == "copper") {
    ++summary.copper_count;
  } else {
    ++summary.non_copper_count;
  }
  if (layer.visible) {
    ++summary.visible_count;
  } else {
    ++summary.hidden_count;
  }
}

std::string layerRowJson(const ccad::Layer& layer, const std::optional<std::size_t> stack_order) {
  const std::optional<std::size_t> kicad_number = ccad::standardKiCadPcbLayerNumber(layer.id);
  std::ostringstream row;
  row << "    {\"type\": \"layer\", \"id\": \"" << ccad::escapeJson(layer.id)
      << "\", \"name\": \"" << ccad::escapeJson(layer.name)
      << "\", \"kind\": \"" << ccad::escapeJson(layer.kind)
      << "\", \"visible\": " << (layer.visible ? "true" : "false");
  if (kicad_number.has_value()) {
    row << ", \"kicad_layer_number\": " << *kicad_number;
  }
  if (stack_order.has_value()) {
    row << ", \"stack_order\": " << *stack_order;
  }
  row << "}";
  return row.str();
}

std::string layerListJson(const std::string& query_kind, const std::vector<std::string>& rows,
                          const LayerQuerySummary& summary) {
  std::ostringstream out;
  out << "{\n"
      << "  \"query\": {\n"
      << "    \"query_kind\": \"" << ccad::escapeJson(query_kind) << "\"\n"
      << "  },\n"
      << "  \"summary\": {\n"
      << "    \"total\": " << rows.size() << ",\n"
      << "    \"copper_count\": " << summary.copper_count << ",\n"
      << "    \"non_copper_count\": " << summary.non_copper_count << ",\n"
      << "    \"visible_count\": " << summary.visible_count << ",\n"
      << "    \"hidden_count\": " << summary.hidden_count << "\n"
      << "  },\n"
      << "  \"layers\": [\n";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    out << rows.at(i) << (i + 1 == rows.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

void collectPcbNetRows(const ccad::Board& board, const std::string& net_id,
                       const std::string& type_filter, std::vector<std::string>& rows,
                       ConnectableSummary& summary) {
  if (includeConnectableType(type_filter, "pad")) {
    for (const ccad::Pad& pad : board.pads) {
      if (pad.net_id == net_id) {
        ++summary.pad_count;
        rows.push_back(padNetRowJson(board, pad));
      }
    }
  }
  if (includeConnectableType(type_filter, "via")) {
    for (const ccad::Via& via : board.vias) {
      if (via.net_id == net_id) {
        ++summary.via_count;
        rows.push_back(viaNetRowJson(via));
      }
    }
  }
  if (includeConnectableType(type_filter, "track")) {
    for (const ccad::TrackSegment& track : board.tracks) {
      if (track.net_id == net_id) {
        ++summary.track_count;
        rows.push_back(trackNetRowJson(track));
      }
    }
  }
  if (includeConnectableType(type_filter, "zone")) {
    for (const ccad::BoardZone& zone : board.zones) {
      if (zone.net_id == net_id) {
        ++summary.zone_count;
        rows.push_back(zoneNetRowJson(zone));
      }
    }
  }
}

ConnectableSource findConnectableSource(const ccad::Board& board, const std::string& source_id) {
  for (const ccad::Pad& pad : board.pads) {
    if (pad.id == source_id) {
      return ConnectableSource{.type = "pad", .id = pad.id, .net_id = pad.net_id};
    }
  }
  for (const ccad::Via& via : board.vias) {
    if (via.id == source_id) {
      return ConnectableSource{.type = "via", .id = via.id, .net_id = via.net_id};
    }
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    if (track.id == source_id) {
      return ConnectableSource{.type = "track", .id = track.id, .net_id = track.net_id};
    }
  }
  for (const ccad::BoardZone& zone : board.zones) {
    if (zone.id == source_id) {
      return ConnectableSource{.type = "zone", .id = zone.id, .net_id = zone.net_id};
    }
  }
  throw std::runtime_error("unknown connected board object: " + source_id);
}

std::string pcbNetQueryJson(const std::string& query_kind, const std::string& net_id,
                            const std::string& type_filter, const ConnectableSource* source,
                            const std::vector<std::string>& rows,
                            const ConnectableSummary& summary) {
  std::ostringstream out;
  out << "{\n"
      << "  \"query\": {\n"
      << "    \"query_kind\": \"" << ccad::escapeJson(query_kind) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(net_id) << "\",\n"
      << "    \"type_filter\": \"" << ccad::escapeJson(type_filter) << "\",\n"
      << "    \"connectivity_scope\": \"net_equivalent_first_slice\"";
  if (source != nullptr) {
    out << ",\n"
        << "    \"source_id\": \"" << ccad::escapeJson(source->id) << "\",\n"
        << "    \"source_type\": \"" << ccad::escapeJson(source->type) << "\",\n"
        << "    \"source_found\": true";
  }
  out << "\n"
      << "  },\n"
      << "  \"summary\": {\n"
      << "    \"total\": " << rows.size() << ",\n"
      << "    \"pad_count\": " << summary.pad_count << ",\n"
      << "    \"via_count\": " << summary.via_count << ",\n"
      << "    \"track_count\": " << summary.track_count << ",\n"
      << "    \"zone_count\": " << summary.zone_count << "\n"
      << "  },\n"
      << "  \"objects\": [\n";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    out << rows.at(i) << (i + 1 == rows.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

}  // namespace

std::string pcbLayerObjectJson(const ccad::Layer& layer) {
  const std::optional<std::size_t> kicad_number = ccad::standardKiCadPcbLayerNumber(layer.id);
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"layer\",\n"
      << "    \"id\": \"" << ccad::escapeJson(layer.id) << "\",\n"
      << "    \"name\": \"" << ccad::escapeJson(layer.name) << "\",\n"
      << "    \"kind\": \"" << ccad::escapeJson(layer.kind) << "\",\n"
      << "    \"visible\": " << (layer.visible ? "true" : "false");
  if (kicad_number.has_value()) {
    out << ",\n"
        << "    \"kicad_layer_number\": " << *kicad_number;
  }
  out << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string pcbPadObjectJson(const ccad::Pad& pad) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"pad\",\n"
      << "    \"id\": \"" << ccad::escapeJson(pad.id) << "\",\n"
      << "    \"component_id\": \"" << ccad::escapeJson(pad.component_id) << "\",\n"
      << "    \"pin_name\": \"" << ccad::escapeJson(pad.pin_name) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(pad.net_id) << "\",\n"
      << "    \"pad_type\": \"" << ccad::escapeJson(pad.type) << "\",\n"
      << "    \"shape\": \"" << ccad::escapeJson(pad.shape) << "\",\n"
      << "    \"layers\": [";
  for (size_t i = 0; i < pad.layers.size(); ++i) {
    if (i > 0) out << ", ";
    out << "\"" << ccad::escapeJson(pad.layers[i]) << "\"";
  }
  out << "],\n"
      << "    \"position\": {\n";
  writePointJson(out, pad.position, 6);
  out << "\n    },\n"
      << "    \"rotation_degrees\": " << pad.rotation_degrees << ",\n"
      << "    \"size\": {\n";
  writeSizeJson(out, pad.size, 6);
  out << "\n    }";
  if (pad.drill.has_value()) {
    out << ",\n"
        << "    \"drill_nm\": " << pad.drill->nanometers;
  }
  if (pad.roundrect_rratio.has_value()) {
    out << ",\n"
        << "    \"roundrect_rratio\": " << *pad.roundrect_rratio;
  }
  if (pad.chamfer_ratio.has_value()) {
    out << ",\n"
        << "    \"chamfer_ratio\": " << *pad.chamfer_ratio;
  }
  out << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string pcbViaObjectJson(const ccad::Via& via) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"via\",\n"
      << "    \"id\": \"" << ccad::escapeJson(via.id) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(via.net_id) << "\",\n"
      << "    \"position\": {\n";
  writePointJson(out, via.position, 6);
  out << "\n    },\n"
      << "    \"diameter_nm\": " << via.diameter.nanometers << ",\n"
      << "    \"drill_nm\": " << via.drill.nanometers << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string pcbTrackObjectJson(const ccad::TrackSegment& track) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"track\",\n"
      << "    \"id\": \"" << ccad::escapeJson(track.id) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(track.net_id) << "\",\n"
      << "    \"layer_id\": \"" << ccad::escapeJson(track.layer_id) << "\",\n"
      << "    \"start\": {\n";
  writePointJson(out, track.start, 6);
  out << "\n    },\n"
      << "    \"end\": {\n";
  writePointJson(out, track.end, 6);
  out << "\n    },\n"
      << "    \"width_nm\": " << track.width.nanometers << ",\n"
      << "    \"source_route_request_id\": \""
      << ccad::escapeJson(track.source_route_request_id) << "\"\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string pcbBoardGraphicObjectJson(const ccad::BoardGraphic& graphic) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"graphic\",\n"
      << "    \"id\": \"" << ccad::escapeJson(graphic.id) << "\",\n"
      << "    \"kind\": \"" << ccad::escapeJson(graphic.kind) << "\",\n"
      << "    \"layer_id\": \"" << ccad::escapeJson(graphic.layer_id) << "\",\n"
      << "    \"start\": {\n";
  writePointJson(out, graphic.start, 6);
  out << "\n    },\n"
      << "    \"end\": {\n";
  writePointJson(out, graphic.end, 6);
  out << "\n    },\n"
      << "    \"width_nm\": " << graphic.width.nanometers << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string pcbBoardTextObjectJson(const ccad::BoardText& text) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"text\",\n"
      << "    \"id\": \"" << ccad::escapeJson(text.id) << "\",\n"
      << "    \"layer_id\": \"" << ccad::escapeJson(text.layer_id) << "\",\n"
      << "    \"text\": \"" << ccad::escapeJson(text.text) << "\",\n"
      << "    \"position\": {\n";
  writePointJson(out, text.position, 6);
  out << "\n    },\n"
      << "    \"rotation_degrees\": " << text.rotation_degrees << ",\n"
      << "    \"size\": {\n";
  writeSizeJson(out, text.size, 6);
  out << "\n    }\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string pcbRegionObjectJson(const std::string& type, const std::string& id,
                                const std::string& kind, const ccad::Rect& area) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"" << ccad::escapeJson(type) << "\",\n"
      << "    \"id\": \"" << ccad::escapeJson(id) << "\",\n"
      << "    \"kind\": \"" << ccad::escapeJson(kind) << "\",\n"
      << "    \"area\": {\n"
      << "      \"x_nm\": " << area.origin.x.nanometers << ",\n"
      << "      \"y_nm\": " << area.origin.y.nanometers << ",\n"
      << "      \"width_nm\": " << area.size.width.nanometers << ",\n"
      << "      \"height_nm\": " << area.size.height.nanometers << "\n"
      << "    }\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string pcbBoardZoneObjectJson(const ccad::BoardZone& zone) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"zone\",\n"
      << "    \"id\": \"" << ccad::escapeJson(zone.id) << "\",\n"
      << "    \"name\": \"" << ccad::escapeJson(zone.name) << "\",\n"
      << "    \"net_id\": \"" << ccad::escapeJson(zone.net_id) << "\",\n"
      << "    \"layer_ids\": [";
  for (std::size_t i = 0; i < zone.layer_ids.size(); ++i) {
    if (i > 0) out << ", ";
    out << "\"" << ccad::escapeJson(zone.layer_ids.at(i)) << "\"";
  }
  out << "],\n"
      << "    \"corner_count\": " << zone.outline.size() << ",\n"
      << "    \"priority\": " << zone.priority << ",\n"
      << "    \"clearance_nm\": " << zone.clearance.nanometers << ",\n"
      << "    \"min_thickness_nm\": " << zone.min_thickness.nanometers << ",\n"
      << "    \"fill_enabled\": " << (zone.fill_enabled ? "true" : "false") << ",\n"
      << "    \"pad_connection\": \"" << ccad::escapeJson(zone.pad_connection) << "\"\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

void requireKnownPcbObjectType(const std::string& type) {
  if (type.empty() || type == "layer" || type == "pad" || type == "via" || type == "track" ||
      type == "graphic" || type == "text" || type == "zone" || type == "keepout" ||
      type == "placement_region") {
    return;
  }
  throw std::runtime_error("unknown object type: " + type);
}

void requireKnownPcbConnectableObjectType(const std::string& type) {
  if (type.empty() || type == "pad" || type == "via" || type == "track" || type == "zone") {
    return;
  }
  throw std::runtime_error("unknown connectable PCB object type: " + type);
}

std::string listPcbObjectsJson(const ccad::Board& board, const std::string& type_filter) {
  std::vector<std::string> rows;
  auto add_row = [&rows](const std::ostringstream& row) { rows.push_back(row.str()); };

  if (includeObjectType(type_filter, "layer")) {
    for (const ccad::Layer& layer : board.layers) {
      const std::optional<std::size_t> kicad_number = ccad::standardKiCadPcbLayerNumber(layer.id);
      std::ostringstream row;
      row << "    {\"type\": \"layer\", \"id\": \"" << ccad::escapeJson(layer.id)
          << "\", \"name\": \"" << ccad::escapeJson(layer.name) << "\", \"kind\": \""
          << ccad::escapeJson(layer.kind) << "\", \"visible\": "
          << (layer.visible ? "true" : "false");
      if (kicad_number.has_value()) {
        row << ", \"kicad_layer_number\": " << *kicad_number;
      }
      row << "}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "pad")) {
    for (const ccad::Pad& pad : board.pads) {
      const std::vector<std::string> resolved_layers = resolvedPadLayers(board, pad);
      const std::vector<std::size_t> kicad_layer_numbers =
          ccad::standardKiCadPcbLayerNumbersForSet(resolved_layers);
      std::ostringstream row;
      row << "    {\"type\": \"pad\", \"id\": \"" << ccad::escapeJson(pad.id)
          << "\", \"component_id\": \"" << ccad::escapeJson(pad.component_id)
          << "\", \"pin_name\": \"" << ccad::escapeJson(pad.pin_name)
          << "\", \"net_id\": \"" << ccad::escapeJson(pad.net_id)
          << "\", \"pad_type\": \"" << ccad::escapeJson(pad.type)
          << "\", \"shape\": \"" << ccad::escapeJson(pad.shape)
          << "\", \"layer_id\": \""
          << ccad::escapeJson(pad.layers.empty() ? "" : pad.layers.front())
          << "\", \"resolved_layers\": ";
      appendLayerIdsJson(row, resolved_layers);
      row << ", \"kicad_layer_numbers\": ";
      appendLayerNumbersJson(row, kicad_layer_numbers);
      if (pad.drill.has_value()) {
        row << ", \"drill_nm\": " << pad.drill->nanometers;
      }
      if (pad.roundrect_rratio.has_value()) {
        row << ", \"roundrect_rratio\": " << *pad.roundrect_rratio;
      }
      if (pad.chamfer_ratio.has_value()) {
        row << ", \"chamfer_ratio\": " << *pad.chamfer_ratio;
      }
      row << "}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "via")) {
    for (const ccad::Via& via : board.vias) {
      std::ostringstream row;
      row << "    {\"type\": \"via\", \"id\": \"" << ccad::escapeJson(via.id)
          << "\", \"net_id\": \"" << ccad::escapeJson(via.net_id) << "\"}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "track")) {
    for (const ccad::TrackSegment& track : board.tracks) {
      std::ostringstream row;
      row << "    {\"type\": \"track\", \"id\": \"" << ccad::escapeJson(track.id)
          << "\", \"net_id\": \"" << ccad::escapeJson(track.net_id)
          << "\", \"layer_id\": \"" << ccad::escapeJson(track.layer_id)
          << "\", \"source_route_request_id\": \""
          << ccad::escapeJson(track.source_route_request_id) << "\"}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "graphic")) {
    for (const ccad::BoardGraphic& graphic : board.graphics) {
      std::ostringstream row;
      row << "    {\"type\": \"graphic\", \"id\": \"" << ccad::escapeJson(graphic.id)
          << "\", \"kind\": \"" << ccad::escapeJson(graphic.kind)
          << "\", \"layer_id\": \"" << ccad::escapeJson(graphic.layer_id)
          << "\", \"width_nm\": " << graphic.width.nanometers << "}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "text")) {
    for (const ccad::BoardText& text : board.texts) {
      std::ostringstream row;
      row << "    {\"type\": \"text\", \"id\": \"" << ccad::escapeJson(text.id)
          << "\", \"layer_id\": \"" << ccad::escapeJson(text.layer_id)
          << "\", \"text\": \"" << ccad::escapeJson(text.text)
          << "\", \"rotation_degrees\": " << text.rotation_degrees << "}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "zone")) {
    for (const ccad::BoardZone& zone : board.zones) {
      std::ostringstream row;
      row << "    {\"type\": \"zone\", \"id\": \"" << ccad::escapeJson(zone.id)
          << "\", \"name\": \"" << ccad::escapeJson(zone.name)
          << "\", \"net_id\": \"" << ccad::escapeJson(zone.net_id)
          << "\", \"layer_ids\": [";
      for (std::size_t i = 0; i < zone.layer_ids.size(); ++i) {
        if (i > 0) row << ", ";
        row << "\"" << ccad::escapeJson(zone.layer_ids.at(i)) << "\"";
      }
      row << "], \"corner_count\": " << zone.outline.size()
          << ", \"priority\": " << zone.priority << "}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "keepout")) {
    for (const ccad::Keepout& keepout : board.keepouts) {
      std::ostringstream row;
      row << "    {\"type\": \"keepout\", \"id\": \"" << ccad::escapeJson(keepout.id)
          << "\", \"kind\": \"" << ccad::escapeJson(keepout.kind) << "\"}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "placement_region")) {
    for (const ccad::PlacementRegion& region : board.placement_regions) {
      std::ostringstream row;
      row << "    {\"type\": \"placement_region\", \"id\": \"" << ccad::escapeJson(region.id)
          << "\", \"kind\": \"" << ccad::escapeJson(region.kind) << "\"}";
      add_row(row);
    }
  }

  std::ostringstream out;
  out << "{\n"
      << "  \"summary\": {\n"
      << "    \"total\": " << rows.size() << "\n"
      << "  },\n"
      << "  \"objects\": [\n";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    out << rows.at(i) << (i + 1 == rows.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string listPcbNetsJson(const ccad::Board& board) {
  std::map<std::string, PhysicalNetSummary> nets;
  for (const ccad::Pad& pad : board.pads) {
    if (!pad.net_id.empty()) {
      PhysicalNetSummary& summary = nets[pad.net_id];
      summary.id = pad.net_id;
      ++summary.pad_count;
    }
  }
  for (const ccad::Via& via : board.vias) {
    if (!via.net_id.empty()) {
      PhysicalNetSummary& summary = nets[via.net_id];
      summary.id = via.net_id;
      ++summary.via_count;
    }
  }
  for (const ccad::TrackSegment& track : board.tracks) {
    if (!track.net_id.empty()) {
      PhysicalNetSummary& summary = nets[track.net_id];
      summary.id = track.net_id;
      ++summary.track_count;
    }
  }

  std::ostringstream out;
  out << "{\n"
      << "  \"summary\": {\n"
      << "    \"total\": " << nets.size() << "\n"
      << "  },\n"
      << "  \"nets\": [\n";
  std::size_t index = 0;
  for (const auto& entry : nets) {
    const PhysicalNetSummary& net = entry.second;
    out << "    {\"id\": \"" << ccad::escapeJson(net.id) << "\", \"pad_count\": "
        << net.pad_count << ", \"via_count\": " << net.via_count
        << ", \"track_count\": " << net.track_count << "}"
        << (++index == nets.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string listPcbObjectsByNetJson(const ccad::Board& board, const std::string& net_id,
                                    const std::string& type_filter) {
  requireKnownPcbConnectableObjectType(type_filter);
  std::vector<std::string> rows;
  ConnectableSummary summary;
  collectPcbNetRows(board, net_id, type_filter, rows, summary);
  return pcbNetQueryJson("items_by_net", net_id, type_filter, nullptr, rows, summary);
}

std::string listPcbConnectedObjectsJson(const ccad::Board& board, const std::string& source_id,
                                        const std::string& type_filter) {
  requireKnownPcbConnectableObjectType(type_filter);
  const ConnectableSource source = findConnectableSource(board, source_id);
  if (source.net_id.empty()) {
    throw std::runtime_error("connected board object has no assigned net: " + source_id);
  }
  std::vector<std::string> rows;
  ConnectableSummary summary;
  collectPcbNetRows(board, source.net_id, type_filter, rows, summary);
  return pcbNetQueryJson("connected_items", source.net_id, type_filter, &source, rows, summary);
}

std::string listPcbEnabledLayersJson(const ccad::Board& board) {
  std::vector<std::string> rows;
  LayerQuerySummary summary;
  for (const ccad::Layer& layer : board.layers) {
    countLayer(layer, summary);
    rows.push_back(layerRowJson(layer, std::nullopt));
  }
  return layerListJson("enabled_layers", rows, summary);
}

std::string listPcbVisibleLayersJson(const ccad::Board& board) {
  std::vector<std::string> rows;
  LayerQuerySummary summary;
  for (const ccad::Layer& layer : board.layers) {
    if (layer.visible) {
      countLayer(layer, summary);
      rows.push_back(layerRowJson(layer, std::nullopt));
    }
  }
  return layerListJson("visible_layers", rows, summary);
}

std::string getPcbLayerNameJson(const ccad::Board& board, const std::string& layer_id) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      const std::optional<std::size_t> kicad_number = ccad::standardKiCadPcbLayerNumber(layer.id);
      std::ostringstream out;
      out << "{\n"
          << "  \"query\": {\n"
          << "    \"query_kind\": \"layer_name\",\n"
          << "    \"kicad_handler\": \"GetBoardLayerName\"\n"
          << "  },\n"
          << "  \"layer\": {\n"
          << "    \"id\": \"" << ccad::escapeJson(layer.id) << "\",\n"
          << "    \"name\": \"" << ccad::escapeJson(layer.name) << "\",\n"
          << "    \"kind\": \"" << ccad::escapeJson(layer.kind) << "\",\n"
          << "    \"visible\": " << (layer.visible ? "true" : "false");
      if (kicad_number.has_value()) {
        out << ",\n"
            << "    \"kicad_layer_number\": " << *kicad_number;
      }
      out << "\n"
          << "  }\n"
          << "}\n";
      return out.str();
    }
  }
  throw std::runtime_error("unknown layer: " + layer_id);
}

std::string getPcbBoardStackupJson(const ccad::Board& board) {
  std::vector<std::string> rows;
  LayerQuerySummary summary;
  std::size_t stack_order = 0;
  for (const ccad::Layer& layer : board.layers) {
    countLayer(layer, summary);
    rows.push_back(layerRowJson(layer, stack_order));
    ++stack_order;
  }

  std::ostringstream out;
  out << "{\n"
      << "  \"stackup_kind\": \"ccad_board_stackup\",\n"
      << "  \"kicad_handler\": \"GetBoardStackup\",\n"
      << "  \"kicad_parity_scope\": \"enabled_layer_order\",\n"
      << "  \"summary\": {\n"
      << "    \"layer_count\": " << rows.size() << ",\n"
      << "    \"copper_layer_count\": " << summary.copper_count << ",\n"
      << "    \"non_copper_layer_count\": " << summary.non_copper_count << "\n"
      << "  },\n"
      << "  \"layers\": [\n";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    out << rows.at(i) << (i + 1 == rows.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string getPcbDesignRulesJson(const ccad::Board& board) {
  std::ostringstream out;
  out << "{\n"
      << "  \"rules_kind\": \"ccad_board_design_rules\",\n"
      << "  \"kicad_handler\": \"GetBoardDesignRules\",\n"
      << "  \"kicad_parity_scope\": \"minimum_constraints_first_slice\",\n"
      << "  \"rules\": {\n"
      << "    \"copper_clearance_nm\": " << board.design_rules.copper_clearance.nanometers << ",\n"
      << "    \"min_track_width_nm\": " << board.design_rules.min_track_width.nanometers << ",\n"
      << "    \"min_via_annular_ring_nm\": "
      << board.design_rules.min_via_annular_ring.nanometers << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string getPcbOutlineJson(const ccad::Board& board) {
  std::ostringstream out;
  out << "{\n"
      << "  \"outline_kind\": \"ccad_board_outline\",\n"
      << "  \"kicad_handler\": \"GetBoundingBox\",\n"
      << "  \"outline\": {\n"
      << "    \"x_nm\": " << board.outline.origin.x.nanometers << ",\n"
      << "    \"y_nm\": " << board.outline.origin.y.nanometers << ",\n"
      << "    \"width_nm\": " << board.outline.size.width.nanometers << ",\n"
      << "    \"height_nm\": " << board.outline.size.height.nanometers << "\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

std::string listRouteRequestsJson(const ccad::Board& board) {
  std::ostringstream out;
  out << "{\n"
      << "  \"summary\": {\n"
      << "    \"total\": " << board.route_requests.size() << "\n"
      << "  },\n"
      << "  \"route_requests\": [\n";
  for (std::size_t i = 0; i < board.route_requests.size(); ++i) {
    const ccad::RouteRequest& request = board.route_requests.at(i);
    out << "    {\"id\": \"" << ccad::escapeJson(request.id) << "\", \"net_id\": \""
        << ccad::escapeJson(request.net_id) << "\", \"from_object_id\": \""
        << ccad::escapeJson(request.from_object_id) << "\", \"to_object_id\": \""
        << ccad::escapeJson(request.to_object_id) << "\", \"preferred_layer_id\": \""
        << ccad::escapeJson(request.preferred_layer_id) << "\", \"policy\": \""
        << ccad::escapeJson(request.policy) << "\", \"width_nm\": "
        << request.width.nanometers << "}" << (i + 1 == board.route_requests.size() ? "" : ",")
        << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string routeStatusJson(const ccad::Board& board) {
  std::map<std::string, int> routed_segment_counts;
  for (const ccad::TrackSegment& track : board.tracks) {
    if (!track.source_route_request_id.empty()) {
      ++routed_segment_counts[track.source_route_request_id];
    }
  }

  std::vector<std::string> rows;
  int open_count = 0;
  int partial_count = 0;
  for (const ccad::RouteRequest& request : board.route_requests) {
    const int segment_count = routed_segment_counts[request.id];
    const std::string status = segment_count > 0 ? "partial" : "open";
    if (status == "partial") {
      ++partial_count;
    } else {
      ++open_count;
    }

    std::ostringstream row;
    row << "    {\"request_id\": \"" << ccad::escapeJson(request.id)
        << "\", \"status\": \"" << status << "\", \"net_id\": \""
        << ccad::escapeJson(request.net_id) << "\", \"from_object_id\": \""
        << ccad::escapeJson(request.from_object_id) << "\", \"to_object_id\": \""
        << ccad::escapeJson(request.to_object_id) << "\", \"preferred_layer_id\": \""
        << ccad::escapeJson(request.preferred_layer_id) << "\", \"policy\": \""
        << ccad::escapeJson(request.policy) << "\", \"width_nm\": "
        << request.width.nanometers << ", \"routed_segment_count\": " << segment_count << "}";
    rows.push_back(row.str());
  }

  int completed_count = 0;
  for (const auto& entry : routed_segment_counts) {
    bool still_open = false;
    for (const ccad::RouteRequest& request : board.route_requests) {
      if (request.id == entry.first) {
        still_open = true;
        break;
      }
    }
    if (still_open) {
      continue;
    }
    ++completed_count;
    std::ostringstream row;
    row << "    {\"request_id\": \"" << ccad::escapeJson(entry.first)
        << "\", \"status\": \"completed\", \"routed_segment_count\": " << entry.second << "}";
    rows.push_back(row.str());
  }

  std::ostringstream out;
  out << "{\n"
      << "  \"summary\": {\n"
      << "    \"open\": " << open_count << ",\n"
      << "    \"partial\": " << partial_count << ",\n"
      << "    \"completed\": " << completed_count << ",\n"
      << "    \"total\": " << rows.size() << "\n"
      << "  },\n"
      << "  \"routes\": [\n";
  for (std::size_t i = 0; i < rows.size(); ++i) {
    out << rows.at(i) << (i + 1 == rows.size() ? "" : ",") << '\n';
  }
  out << "  ]\n"
      << "}\n";
  return out.str();
}

std::string exportRouteJobJson(const ccad::Board& board, const std::string& request_id_filter) {
  std::vector<const ccad::RouteRequest*> route_requests;
  for (const ccad::RouteRequest& request : board.route_requests) {
    if (request_id_filter.empty() || request.id == request_id_filter) {
      route_requests.push_back(&request);
    }
  }
  if (!request_id_filter.empty() && route_requests.empty()) {
    throw std::runtime_error("unknown route request: " + request_id_filter);
  }

  std::ostringstream out;
  out << "{\n"
      << "  \"route_job\": {\n"
      << "    \"schema_version\": 1,\n"
      << "    \"units\": {\n"
      << "      \"length_unit\": \"nanometer\",\n"
      << "      \"angle_unit\": \"degree\"\n"
      << "    },\n"
      << "    \"summary\": {\n"
      << "      \"layer_count\": " << board.layers.size() << ",\n"
      << "      \"pad_count\": " << board.pads.size() << ",\n"
      << "      \"via_count\": " << board.vias.size() << ",\n"
      << "      \"track_count\": " << board.tracks.size() << ",\n"
      << "      \"graphic_count\": " << board.graphics.size() << ",\n"
      << "      \"text_count\": " << board.texts.size() << ",\n"
      << "      \"zone_count\": " << board.zones.size() << ",\n"
      << "      \"keepout_count\": " << board.keepouts.size() << ",\n"
      << "      \"placement_region_count\": " << board.placement_regions.size() << ",\n"
      << "      \"route_request_count\": " << route_requests.size() << "\n"
      << "    },\n"
      << "    \"board_outline\": {\n"
      << "      \"x_nm\": " << board.outline.origin.x.nanometers << ",\n"
      << "      \"y_nm\": " << board.outline.origin.y.nanometers << ",\n"
      << "      \"width_nm\": " << board.outline.size.width.nanometers << ",\n"
      << "      \"height_nm\": " << board.outline.size.height.nanometers << "\n"
      << "    },\n"
      << "    \"design_rules\": {\n"
      << "      \"copper_clearance_nm\": " << board.design_rules.copper_clearance.nanometers
      << ",\n"
      << "      \"min_track_width_nm\": " << board.design_rules.min_track_width.nanometers
      << ",\n"
      << "      \"min_via_annular_ring_nm\": "
      << board.design_rules.min_via_annular_ring.nanometers << "\n"
      << "    },\n"
      << "    \"layers\": [\n";
  for (std::size_t i = 0; i < board.layers.size(); ++i) {
    const ccad::Layer& layer = board.layers.at(i);
    out << "      {\"id\": \"" << ccad::escapeJson(layer.id) << "\", \"name\": \""
        << ccad::escapeJson(layer.name) << "\", \"kind\": \"" << ccad::escapeJson(layer.kind)
        << "\", \"visible\": " << (layer.visible ? "true" : "false") << "}"
        << (i + 1 == board.layers.size() ? "" : ",") << '\n';
  }
  out << "    ],\n"
      << "    \"physical_objects\": {\n"
      << "      \"pads\": [\n";
  for (std::size_t i = 0; i < board.pads.size(); ++i) {
    const ccad::Pad& pad = board.pads.at(i);
    const std::vector<std::string> resolved_layers = resolvedPadLayers(board, pad);
    const std::vector<std::size_t> kicad_layer_numbers =
        ccad::standardKiCadPcbLayerNumbersForSet(resolved_layers);
    out << "        {\"id\": \"" << ccad::escapeJson(pad.id) << "\", \"net_id\": \""
        << ccad::escapeJson(pad.net_id) << "\", \"layer_id\": \""
        << ccad::escapeJson(pad.layers.empty() ? "" : pad.layers.front())
        << "\", \"pad_type\": \"" << ccad::escapeJson(pad.type)
        << "\", \"shape\": \"" << ccad::escapeJson(pad.shape) << "\", \"layers\": [";
    for (std::size_t layer_index = 0; layer_index < pad.layers.size(); ++layer_index) {
      if (layer_index > 0) out << ", ";
      out << "\"" << ccad::escapeJson(pad.layers.at(layer_index)) << "\"";
    }
    out << "], \"resolved_layers\": ";
    appendLayerIdsJson(out, resolved_layers);
    out << ", \"kicad_layer_numbers\": ";
    appendLayerNumbersJson(out, kicad_layer_numbers);
    out << ", \"x_nm\": " << pad.position.x.nanometers
        << ", \"y_nm\": " << pad.position.y.nanometers << ", \"width_nm\": "
        << pad.size.width.nanometers << ", \"height_nm\": " << pad.size.height.nanometers
        << ", \"rotation_degrees\": " << pad.rotation_degrees;
    if (pad.drill.has_value()) {
      out << ", \"drill_nm\": " << pad.drill->nanometers;
    }
    if (pad.roundrect_rratio.has_value()) {
      out << ", \"roundrect_rratio\": " << *pad.roundrect_rratio;
    }
    if (pad.chamfer_ratio.has_value()) {
      out << ", \"chamfer_ratio\": " << *pad.chamfer_ratio;
    }
    out << "}" << (i + 1 == board.pads.size() ? "" : ",") << '\n';
  }
  out << "      ],\n"
      << "      \"vias\": [\n";
  for (std::size_t i = 0; i < board.vias.size(); ++i) {
    const ccad::Via& via = board.vias.at(i);
    out << "        {\"id\": \"" << ccad::escapeJson(via.id) << "\", \"net_id\": \""
        << ccad::escapeJson(via.net_id) << "\", \"x_nm\": " << via.position.x.nanometers
        << ", \"y_nm\": " << via.position.y.nanometers << ", \"diameter_nm\": "
        << via.diameter.nanometers << ", \"drill_nm\": " << via.drill.nanometers << "}"
        << (i + 1 == board.vias.size() ? "" : ",") << '\n';
  }
  out << "      ],\n"
      << "      \"tracks\": [\n";
  for (std::size_t i = 0; i < board.tracks.size(); ++i) {
    const ccad::TrackSegment& track = board.tracks.at(i);
    out << "        {\"id\": \"" << ccad::escapeJson(track.id) << "\", \"net_id\": \""
        << ccad::escapeJson(track.net_id) << "\", \"layer_id\": \""
        << ccad::escapeJson(track.layer_id) << "\", \"start_x_nm\": "
        << track.start.x.nanometers << ", \"start_y_nm\": " << track.start.y.nanometers
        << ", \"end_x_nm\": " << track.end.x.nanometers << ", \"end_y_nm\": "
        << track.end.y.nanometers << ", \"width_nm\": " << track.width.nanometers
        << ", \"source_route_request_id\": \""
        << ccad::escapeJson(track.source_route_request_id) << "\"}"
        << (i + 1 == board.tracks.size() ? "" : ",") << '\n';
  }
  out << "      ]\n"
      << "    },\n"
      << "    \"keepouts\": [\n";
  for (std::size_t i = 0; i < board.keepouts.size(); ++i) {
    const ccad::Keepout& keepout = board.keepouts.at(i);
    out << "      {\"id\": \"" << ccad::escapeJson(keepout.id) << "\", \"kind\": \""
        << ccad::escapeJson(keepout.kind) << "\", \"x_nm\": "
        << keepout.area.origin.x.nanometers << ", \"y_nm\": "
        << keepout.area.origin.y.nanometers << ", \"width_nm\": "
        << keepout.area.size.width.nanometers << ", \"height_nm\": "
        << keepout.area.size.height.nanometers << "}"
        << (i + 1 == board.keepouts.size() ? "" : ",") << '\n';
  }
  out << "    ],\n"
      << "    \"zones\": [\n";
  for (std::size_t i = 0; i < board.zones.size(); ++i) {
    const ccad::BoardZone& zone = board.zones.at(i);
    out << "      {\"id\": \"" << ccad::escapeJson(zone.id) << "\", \"name\": \""
        << ccad::escapeJson(zone.name) << "\", \"net_id\": \""
        << ccad::escapeJson(zone.net_id) << "\", \"layer_ids\": [";
    for (std::size_t layer_index = 0; layer_index < zone.layer_ids.size(); ++layer_index) {
      if (layer_index > 0) out << ", ";
      out << "\"" << ccad::escapeJson(zone.layer_ids.at(layer_index)) << "\"";
    }
    out << "], \"corner_count\": " << zone.outline.size()
        << ", \"priority\": " << zone.priority << ", \"clearance_nm\": "
        << zone.clearance.nanometers << ", \"min_thickness_nm\": "
        << zone.min_thickness.nanometers << ", \"fill_enabled\": "
        << (zone.fill_enabled ? "true" : "false") << ", \"pad_connection\": \""
        << ccad::escapeJson(zone.pad_connection) << "\"}"
        << (i + 1 == board.zones.size() ? "" : ",") << '\n';
  }
  out << "    ],\n"
      << "    \"placement_regions\": [\n";
  for (std::size_t i = 0; i < board.placement_regions.size(); ++i) {
    const ccad::PlacementRegion& region = board.placement_regions.at(i);
    out << "      {\"id\": \"" << ccad::escapeJson(region.id) << "\", \"kind\": \""
        << ccad::escapeJson(region.kind) << "\", \"x_nm\": "
        << region.area.origin.x.nanometers << ", \"y_nm\": "
        << region.area.origin.y.nanometers << ", \"width_nm\": "
        << region.area.size.width.nanometers << ", \"height_nm\": "
        << region.area.size.height.nanometers << "}"
        << (i + 1 == board.placement_regions.size() ? "" : ",") << '\n';
  }
  out << "    ],\n"
      << "    \"route_requests\": [\n";
  for (std::size_t i = 0; i < route_requests.size(); ++i) {
    const ccad::RouteRequest& request = *route_requests.at(i);
    out << "      {\"id\": \"" << ccad::escapeJson(request.id) << "\", \"net_id\": \""
        << ccad::escapeJson(request.net_id) << "\", \"from_object_id\": \""
        << ccad::escapeJson(request.from_object_id) << "\", \"to_object_id\": \""
        << ccad::escapeJson(request.to_object_id) << "\", \"preferred_layer_id\": \""
        << ccad::escapeJson(request.preferred_layer_id) << "\", \"policy\": \""
        << ccad::escapeJson(request.policy) << "\", \"width_nm\": "
        << request.width.nanometers << "}"
        << (i + 1 == route_requests.size() ? "" : ",") << '\n';
  }
  out << "    ]\n"
      << "  }\n"
      << "}\n";
  return out.str();
}

}  // namespace ccad_cli
