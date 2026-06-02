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
  for (const std::string& layer : pad.layers) {
    sig += layer + "\x1e";
  }
  sig += "\x1f" + pad.type + "\x1f" + pad.shape + "\x1f" + pointSignature(pad.position) + "\x1f" +
         sizeSignature(pad.size) + "\x1f" + std::to_string(pad.rotation_degrees);
  sig += "\x1f" + (pad.drill.has_value() ? std::to_string(pad.drill->nanometers) : "");
  sig += "\x1f" +
         (pad.roundrect_rratio.has_value() ? std::to_string(*pad.roundrect_rratio) : "");
  sig += "\x1f" + (pad.chamfer_ratio.has_value() ? std::to_string(*pad.chamfer_ratio) : "");
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
  diffObjectMap(diff, "component", before.components, after.components, componentSignature);
  diffObjectMap(diff, "net", before.nets, after.nets, netSignature);
  diffObjectMap(diff, "constraint", before.constraints, after.constraints, constraintSignature);
  if (before.board.has_value() && after.board.has_value()) {
    if (outlineSignature(before.board->outline) != outlineSignature(after.board->outline)) {
      ++diff.changed_count;
      diff.entries.push_back(DiffEntry{
          .change = "changed",
          .object_type = "board_outline",
          .object_id = "board",
          .message = "board outline changed",
      });
    }
    if (designRulesSignature(before.board->design_rules) !=
        designRulesSignature(after.board->design_rules)) {
      ++diff.changed_count;
      diff.entries.push_back(DiffEntry{
          .change = "changed",
          .object_type = "design_rules",
          .object_id = "board",
          .message = "design rules changed",
      });
    }
    diffObjectMap(diff, "layer", before.board->layers, after.board->layers, layerSignature);
    diffObjectMap(diff, "placement_region", before.board->placement_regions,
                  after.board->placement_regions, placementRegionSignature);
    diffObjectMap(diff, "keepout", before.board->keepouts, after.board->keepouts,
                  keepoutSignature);
    diffObjectMap(diff, "pad", before.board->pads, after.board->pads, padSignature);
    diffObjectMap(diff, "via", before.board->vias, after.board->vias, viaSignature);
    diffObjectMap(diff, "track", before.board->tracks, after.board->tracks, trackSignature);
    diffObjectMap(diff, "graphic", before.board->graphics, after.board->graphics,
                  graphicSignature);
    diffObjectMap(diff, "text", before.board->texts, after.board->texts, textSignature);
    diffObjectMap(diff, "route_request", before.board->route_requests,
                  after.board->route_requests, routeRequestSignature);
  } else if (!before.board.has_value() && after.board.has_value()) {
    ++diff.added_count;
    diff.entries.push_back(DiffEntry{
        .change = "added",
        .object_type = "board_outline",
        .object_id = "board",
        .message = "board outline added",
    });
    diff.added_count += after.board->layers.size();
    for (const Layer& layer : after.board->layers) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "layer",
          .object_id = layer.id,
          .message = "layer added",
      });
    }
    diff.added_count += after.board->placement_regions.size();
    for (const PlacementRegion& region : after.board->placement_regions) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "placement_region",
          .object_id = region.id,
          .message = "placement_region added",
      });
    }
    diff.added_count += after.board->keepouts.size();
    for (const Keepout& keepout : after.board->keepouts) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "keepout",
          .object_id = keepout.id,
          .message = "keepout added",
      });
    }
    diff.added_count += after.board->pads.size();
    for (const Pad& pad : after.board->pads) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "pad",
          .object_id = pad.id,
          .message = "pad added",
      });
    }
    diff.added_count += after.board->vias.size();
    for (const Via& via : after.board->vias) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "via",
          .object_id = via.id,
          .message = "via added",
      });
    }
    diff.added_count += after.board->tracks.size();
    for (const TrackSegment& track : after.board->tracks) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "track",
          .object_id = track.id,
          .message = "track added",
      });
    }
    diff.added_count += after.board->graphics.size();
    for (const BoardGraphic& graphic : after.board->graphics) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "graphic",
          .object_id = graphic.id,
          .message = "graphic added",
      });
    }
    diff.added_count += after.board->texts.size();
    for (const BoardText& text : after.board->texts) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "text",
          .object_id = text.id,
          .message = "text added",
      });
    }
    diff.added_count += after.board->route_requests.size();
    for (const RouteRequest& route_request : after.board->route_requests) {
      diff.entries.push_back(DiffEntry{
          .change = "added",
          .object_type = "route_request",
          .object_id = route_request.id,
          .message = "route_request added",
      });
    }
  } else if (before.board.has_value() && !after.board.has_value()) {
    ++diff.removed_count;
    diff.entries.push_back(DiffEntry{
        .change = "removed",
        .object_type = "board_outline",
        .object_id = "board",
        .message = "board outline removed",
    });
    diff.removed_count += before.board->layers.size();
    for (const Layer& layer : before.board->layers) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "layer",
          .object_id = layer.id,
          .message = "layer removed",
      });
    }
    diff.removed_count += before.board->placement_regions.size();
    for (const PlacementRegion& region : before.board->placement_regions) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "placement_region",
          .object_id = region.id,
          .message = "placement_region removed",
      });
    }
    diff.removed_count += before.board->keepouts.size();
    for (const Keepout& keepout : before.board->keepouts) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "keepout",
          .object_id = keepout.id,
          .message = "keepout removed",
      });
    }
    diff.removed_count += before.board->pads.size();
    for (const Pad& pad : before.board->pads) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "pad",
          .object_id = pad.id,
          .message = "pad removed",
      });
    }
    diff.removed_count += before.board->vias.size();
    for (const Via& via : before.board->vias) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "via",
          .object_id = via.id,
          .message = "via removed",
      });
    }
    diff.removed_count += before.board->tracks.size();
    for (const TrackSegment& track : before.board->tracks) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "track",
          .object_id = track.id,
          .message = "track removed",
      });
    }
    diff.removed_count += before.board->graphics.size();
    for (const BoardGraphic& graphic : before.board->graphics) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "graphic",
          .object_id = graphic.id,
          .message = "graphic removed",
      });
    }
    diff.removed_count += before.board->texts.size();
    for (const BoardText& text : before.board->texts) {
      diff.entries.push_back(DiffEntry{
          .change = "removed",
          .object_type = "text",
          .object_id = text.id,
          .message = "text removed",
      });
    }
    diff.removed_count += before.board->route_requests.size();
    for (const RouteRequest& route_request : before.board->route_requests) {
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

