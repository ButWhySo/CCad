#include "ccad_core/diff.hpp"

#include <map>
#include <string>

namespace ccad {
namespace {

std::string pinSignature(const Pin& pin) {
  return pin.name + "\x1f" + pin.kind;
}

std::string componentSignature(const Component& component) {
  std::string signature = component.part;
  for (const Pin& pin : component.pins) {
    signature += "\x1e" + pinSignature(pin);
  }
  return signature;
}

std::string memberSignature(const NetMember& member) {
  return member.component_id + "\x1f" + member.pin_name;
}

std::string netSignature(const Net& net) {
  std::string signature;
  for (const NetMember& member : net.members) {
    signature += "\x1e" + memberSignature(member);
  }
  return signature;
}

std::string constraintSignature(const Constraint& constraint) {
  return constraint.kind + "\x1f" + constraint.target + "\x1f" + constraint.value;
}

std::string layerSignature(const Layer& layer) {
  return layer.name + "\x1f" + layer.kind + "\x1f" + (layer.visible ? "visible" : "hidden");
}

std::string outlineSignature(const Rect& outline) {
  return std::to_string(outline.origin.x.nanometers) + "\x1f" +
         std::to_string(outline.origin.y.nanometers) + "\x1f" +
         std::to_string(outline.size.width.nanometers) + "\x1f" +
         std::to_string(outline.size.height.nanometers);
}

std::string designRulesSignature(const DesignRules& rules) {
  return std::to_string(rules.copper_clearance.nanometers) + "\x1f" +
         std::to_string(rules.min_track_width.nanometers) + "\x1f" +
         std::to_string(rules.min_via_annular_ring.nanometers);
}

std::string pointSignature(const Point& point) {
  return std::to_string(point.x.nanometers) + "\x1f" + std::to_string(point.y.nanometers);
}

std::string sizeSignature(const Size& size) {
  return std::to_string(size.width.nanometers) + "\x1f" +
         std::to_string(size.height.nanometers);
}

std::string padSignature(const Pad& pad) {
  std::string sig = pad.component_id + "\x1f" + pad.pin_name + "\x1f" + pad.net_id + "\x1f";
  for (const std::string& layer : pad.padstack.layer_set) {
    sig += layer + "\x1e";
  }
  const Size psize = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
  const std::string pshape = pad.padstack.copper_props.empty() ? "circle" : (pad.padstack.copper_props.begin()->second.shape.shape == PadShape::Oval ? "oval" : "circle");

  sig += "\x1f" + pad.type + "\x1f" + pshape + "\x1f" + pointSignature(pad.position) + "\x1f" +
         sizeSignature(psize) + "\x1f" + std::to_string(pad.rotation_degrees);
  sig += "\x1f" + (pad.padstack.drill.size.width.nanometers > 0 ? std::to_string(pad.padstack.drill.size.width.nanometers) : "");
  sig += "\x1f" +
         (!pad.padstack.copper_props.empty() && pad.padstack.copper_props.begin()->second.shape.roundrect_rratio > 0.0 ? std::to_string(pad.padstack.copper_props.begin()->second.shape.roundrect_rratio) : "");
  sig += "\x1f" + (!pad.padstack.copper_props.empty() && pad.padstack.copper_props.begin()->second.shape.chamfer_ratio > 0.0 ? std::to_string(pad.padstack.copper_props.begin()->second.shape.chamfer_ratio) : "");
  return sig;
}

std::string viaSignature(const Via& via) {
  return via.net_id + "\x1f" + pointSignature(via.position) + "\x1f" +
         std::to_string(via.diameter.nanometers) + "\x1f" +
         std::to_string(via.drill.nanometers);
}

std::string trackSignature(const TrackSegment& track) {
  return track.net_id + "\x1f" + track.layer_id + "\x1f" + pointSignature(track.start) +
         "\x1f" + pointSignature(track.end) + "\x1f" +
         std::to_string(track.width.nanometers) + "\x1f" + track.source_route_request_id;
}

std::string graphicSignature(const BoardGraphic& graphic) {
  return graphic.kind + "\x1f" + graphic.layer_id + "\x1f" + pointSignature(graphic.start) +
         "\x1f" + pointSignature(graphic.end) + "\x1f" +
         std::to_string(graphic.width.nanometers);
}

std::string textSignature(const BoardText& text) {
  return text.layer_id + "\x1f" + text.text + "\x1f" + pointSignature(text.position) +
         "\x1f" + std::to_string(text.rotation_degrees) + "\x1f" + sizeSignature(text.size);
}

std::string zoneSignature(const BoardZone& zone) {
  std::string signature =
      zone.name + "\x1f" + zone.net_id + "\x1f" + std::to_string(zone.priority) +
      "\x1f" + std::to_string(zone.clearance.nanometers) + "\x1f" +
      std::to_string(zone.min_thickness.nanometers) + "\x1f" +
      (zone.fill_enabled ? "filled" : "outline") + "\x1f" + zone.pad_connection;
  for (const std::string& layer_id : zone.layer_ids) {
    signature += "\x1e" + layer_id;
  }
  signature += "\x1d";
  for (const Point& point : zone.outline) {
    signature += "\x1e" + pointSignature(point);
  }
  return signature;
}

std::string routeRequestSignature(const RouteRequest& route_request) {
  return route_request.net_id + "\x1f" + route_request.from_object_id + "\x1f" +
         route_request.to_object_id + "\x1f" + route_request.preferred_layer_id + "\x1f" +
         route_request.policy + "\x1f" + std::to_string(route_request.width.nanometers);
}

std::string keepoutSignature(const Keepout& keepout) {
  return keepout.kind + "\x1f" + outlineSignature(keepout.area);
}

std::string placementRegionSignature(const PlacementRegion& region) {
  return region.kind + "\x1f" + outlineSignature(region.area);
}

template <typename T, typename SignatureFn>
void diffObjectMap(ProjectDiff& diff, const std::string& object_type, const std::vector<T>& before,
                   const std::vector<T>& after, SignatureFn signature_fn) {
  std::map<std::string, std::string> before_by_id;
  std::map<std::string, std::string> after_by_id;

  for (const T& item : before) {
    before_by_id[item.id] = signature_fn(item);
  }
  for (const T& item : after) {
    after_by_id[item.id] = signature_fn(item);
  }

  for (const auto& [id, signature] : after_by_id) {
    const auto before_it = before_by_id.find(id);
    if (before_it == before_by_id.end()) {
      ++diff.added_count;
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = object_type,
          .object_id = id,
          .message = object_type + " added",
      });
    } else if (before_it->second != signature) {
      ++diff.changed_count;
      diff.entries.push_back(DiffEntry{
          .change = "changed",
          .object_type = object_type,
          .object_id = id,
          .message = object_type + " changed",
      });
    }
  }

  for (const auto& [id, signature] : before_by_id) {
    (void)signature;
    if (!after_by_id.contains(id)) {
      ++diff.removed_count;
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = object_type,
          .object_id = id,
          .message = object_type + " removed",
      });
    }
  }
}

}  // namespace

ProjectDiff diffProjects(const Project& before, const Project& after) {
  ProjectDiff diff;
  const Schematic empty_schematic;
  const Schematic* before_schematic = primarySchematic(before);
  const Schematic* after_schematic = primarySchematic(after);
  const Schematic& before_sch = before_schematic == nullptr ? empty_schematic : *before_schematic;
  const Schematic& after_sch = after_schematic == nullptr ? empty_schematic : *after_schematic;
  diffObjectMap(diff, "component", before_sch.components, after_sch.components, componentSignature);
  diffObjectMap(diff, "net", before_sch.nets, after_sch.nets, netSignature);
  diffObjectMap(diff, "constraint", before_sch.constraints, after_sch.constraints, constraintSignature);
  if (!before.boards.empty() && !after.boards.empty()) {
    if (outlineSignature(before.boards[0].outline) != outlineSignature(after.boards[0].outline)) {
      ++diff.changed_count;
      diff.entries.push_back(DiffEntry{
          .change = "changed",
          .object_type = "board_outline",
          .object_id = "board",
          .message = "board outline changed",
      });
    }
    if (designRulesSignature(before.boards[0].design_rules) !=
        designRulesSignature(after.boards[0].design_rules)) {
      ++diff.changed_count;
      diff.entries.push_back(DiffEntry{
          .change = "changed",
          .object_type = "design_rules",
          .object_id = "board",
          .message = "design rules changed",
      });
    }
    diffObjectMap(diff, "layer", before.boards[0].layers, after.boards[0].layers, layerSignature);
    diffObjectMap(diff, "placement_region", before.boards[0].placement_regions,
                  after.boards[0].placement_regions, placementRegionSignature);
    diffObjectMap(diff, "keepout", before.boards[0].keepouts, after.boards[0].keepouts,
                  keepoutSignature);
    diffObjectMap(diff, "pad", before.boards[0].pads, after.boards[0].pads, padSignature);
    diffObjectMap(diff, "via", before.boards[0].vias, after.boards[0].vias, viaSignature);
    diffObjectMap(diff, "track", before.boards[0].tracks, after.boards[0].tracks, trackSignature);
    diffObjectMap(diff, "graphic", before.boards[0].graphics, after.boards[0].graphics,
                  graphicSignature);
    diffObjectMap(diff, "text", before.boards[0].texts, after.boards[0].texts, textSignature);
    diffObjectMap(diff, "zone", before.boards[0].zones, after.boards[0].zones, zoneSignature);
    diffObjectMap(diff, "route_request", before.boards[0].route_requests,
                  after.boards[0].route_requests, routeRequestSignature);
  } else if (!!before.boards.empty() && !after.boards.empty()) {
    ++diff.added_count;
    diff.entries.push_back(DiffEntry{
        .change = "added",
        .object_type = "board_outline",
        .object_id = "board",
        .message = "board outline added",
    });
    diff.added_count += after.boards[0].layers.size();
    for (const Layer& layer : after.boards[0].layers) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "layer",
          .object_id = layer.id,
          .message = "layer added",
      });
    }
    diff.added_count += after.boards[0].placement_regions.size();
    for (const PlacementRegion& region : after.boards[0].placement_regions) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "placement_region",
          .object_id = region.id,
          .message = "placement_region added",
      });
    }
    diff.added_count += after.boards[0].keepouts.size();
    for (const Keepout& keepout : after.boards[0].keepouts) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "keepout",
          .object_id = keepout.id,
          .message = "keepout added",
      });
    }
    diff.added_count += after.boards[0].pads.size();
    for (const Pad& pad : after.boards[0].pads) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "pad",
          .object_id = pad.id,
          .message = "pad added",
      });
    }
    diff.added_count += after.boards[0].vias.size();
    for (const Via& via : after.boards[0].vias) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "via",
          .object_id = via.id,
          .message = "via added",
      });
    }
    diff.added_count += after.boards[0].tracks.size();
    for (const TrackSegment& track : after.boards[0].tracks) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "track",
          .object_id = track.id,
          .message = "track added",
      });
    }
    diff.added_count += after.boards[0].graphics.size();
    for (const BoardGraphic& graphic : after.boards[0].graphics) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "graphic",
          .object_id = graphic.id,
          .message = "graphic added",
      });
    }
    diff.added_count += after.boards[0].texts.size();
    for (const BoardText& text : after.boards[0].texts) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "text",
          .object_id = text.id,
          .message = "text added",
      });
    }
    diff.added_count += after.boards[0].zones.size();
    for (const BoardZone& zone : after.boards[0].zones) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "zone",
          .object_id = zone.id,
          .message = "zone added",
      });
    }
    diff.added_count += after.boards[0].route_requests.size();
    for (const RouteRequest& route_request : after.boards[0].route_requests) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "route_request",
          .object_id = route_request.id,
          .message = "route_request added",
      });
    }
  } else if (!before.boards.empty() && !!after.boards.empty()) {
    ++diff.removed_count;
    diff.entries.push_back(DiffEntry{
        .change = "removed",
        .object_type = "board_outline",
        .object_id = "board",
        .message = "board outline removed",
    });
    diff.removed_count += before.boards[0].layers.size();
    for (const Layer& layer : before.boards[0].layers) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "layer",
          .object_id = layer.id,
          .message = "layer removed",
      });
    }
    diff.removed_count += before.boards[0].placement_regions.size();
    for (const PlacementRegion& region : before.boards[0].placement_regions) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "placement_region",
          .object_id = region.id,
          .message = "placement_region removed",
      });
    }
    diff.removed_count += before.boards[0].keepouts.size();
    for (const Keepout& keepout : before.boards[0].keepouts) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "keepout",
          .object_id = keepout.id,
          .message = "keepout removed",
      });
    }
    diff.removed_count += before.boards[0].pads.size();
    for (const Pad& pad : before.boards[0].pads) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "pad",
          .object_id = pad.id,
          .message = "pad removed",
      });
    }
    diff.removed_count += before.boards[0].vias.size();
    for (const Via& via : before.boards[0].vias) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "via",
          .object_id = via.id,
          .message = "via removed",
      });
    }
    diff.removed_count += before.boards[0].tracks.size();
    for (const TrackSegment& track : before.boards[0].tracks) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "track",
          .object_id = track.id,
          .message = "track removed",
      });
    }
    diff.removed_count += before.boards[0].graphics.size();
    for (const BoardGraphic& graphic : before.boards[0].graphics) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "graphic",
          .object_id = graphic.id,
          .message = "graphic removed",
      });
    }
    diff.removed_count += before.boards[0].texts.size();
    for (const BoardText& text : before.boards[0].texts) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "text",
          .object_id = text.id,
          .message = "text removed",
      });
    }
    diff.removed_count += before.boards[0].zones.size();
    for (const BoardZone& zone : before.boards[0].zones) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "zone",
          .object_id = zone.id,
          .message = "zone removed",
      });
    }
    diff.removed_count += before.boards[0].route_requests.size();
    for (const RouteRequest& route_request : before.boards[0].route_requests) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "route_request",
          .object_id = route_request.id,
          .message = "route_request removed",
      });
    }
  }
  return diff;
}

}  // namespace ccad

