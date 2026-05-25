#include "ccad_cli/pcb_object_queries.hpp"

#include "ccad_core/json.hpp"

#include <map>
#include <ostream>
#include <sstream>
#include <vector>

namespace ccad_cli {
namespace {

struct PhysicalNetSummary {
  std::string id;
  int pad_count = 0;
  int via_count = 0;
  int track_count = 0;
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

}  // namespace

std::string pcbLayerObjectJson(const ccad::Layer& layer) {
  std::ostringstream out;
  out << "{\n"
      << "  \"object\": {\n"
      << "    \"type\": \"layer\",\n"
      << "    \"id\": \"" << ccad::escapeJson(layer.id) << "\",\n"
      << "    \"name\": \"" << ccad::escapeJson(layer.name) << "\",\n"
      << "    \"kind\": \"" << ccad::escapeJson(layer.kind) << "\",\n"
      << "    \"visible\": " << (layer.visible ? "true" : "false") << "\n"
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
      << "    \"layer_id\": \"" << ccad::escapeJson(pad.layer_id) << "\",\n"
      << "    \"position\": {\n";
  writePointJson(out, pad.position, 6);
  out << "\n    },\n"
      << "    \"rotation_degrees\": " << pad.rotation_degrees << ",\n"
      << "    \"size\": {\n";
  writeSizeJson(out, pad.size, 6);
  out << "\n    }\n"
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

void requireKnownPcbObjectType(const std::string& type) {
  if (type.empty() || type == "layer" || type == "pad" || type == "via" || type == "track" ||
      type == "keepout" || type == "placement_region") {
    return;
  }
  throw std::runtime_error("unknown object type: " + type);
}

std::string listPcbObjectsJson(const ccad::Board& board, const std::string& type_filter) {
  std::vector<std::string> rows;
  auto add_row = [&rows](const std::ostringstream& row) { rows.push_back(row.str()); };

  if (includeObjectType(type_filter, "layer")) {
    for (const ccad::Layer& layer : board.layers) {
      std::ostringstream row;
      row << "    {\"type\": \"layer\", \"id\": \"" << ccad::escapeJson(layer.id)
          << "\", \"name\": \"" << ccad::escapeJson(layer.name) << "\", \"kind\": \""
          << ccad::escapeJson(layer.kind) << "\", \"visible\": "
          << (layer.visible ? "true" : "false") << "}";
      add_row(row);
    }
  }
  if (includeObjectType(type_filter, "pad")) {
    for (const ccad::Pad& pad : board.pads) {
      std::ostringstream row;
      row << "    {\"type\": \"pad\", \"id\": \"" << ccad::escapeJson(pad.id)
          << "\", \"component_id\": \"" << ccad::escapeJson(pad.component_id)
          << "\", \"pin_name\": \"" << ccad::escapeJson(pad.pin_name)
          << "\", \"net_id\": \"" << ccad::escapeJson(pad.net_id)
          << "\", \"layer_id\": \"" << ccad::escapeJson(pad.layer_id) << "\"}";
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
    out << "        {\"id\": \"" << ccad::escapeJson(pad.id) << "\", \"net_id\": \""
        << ccad::escapeJson(pad.net_id) << "\", \"layer_id\": \""
        << ccad::escapeJson(pad.layer_id) << "\", \"x_nm\": " << pad.position.x.nanometers
        << ", \"y_nm\": " << pad.position.y.nanometers << ", \"width_nm\": "
        << pad.size.width.nanometers << ", \"height_nm\": " << pad.size.height.nanometers
        << "}" << (i + 1 == board.pads.size() ? "" : ",") << '\n';
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
