#include "ccad_core/canvas.hpp"

#include <map>

namespace ccad {
namespace {

double toMillimeters(const Length& length) {
  return static_cast<double>(length.nanometers) / 1000000.0;
}

}  // namespace

CanvasScene buildCanvasScene(const Project& project) {
  CanvasScene scene;
  if (!project.board.has_value()) {
    return scene;
  }

  scene.has_board = true;
  scene.board_width_nm = project.board->outline.size.width.nanometers;
  scene.board_height_nm = project.board->outline.size.height.nanometers;
  scene.board_origin_x_units = toMillimeters(project.board->outline.origin.x);
  scene.board_origin_y_units = toMillimeters(project.board->outline.origin.y);
  scene.view_width_units = toMillimeters(project.board->outline.size.width);
  scene.view_height_units = toMillimeters(project.board->outline.size.height);

  for (const Layer& layer : project.board->layers) {
    scene.layers.push_back(CanvasLayer{
        .id = layer.id,
        .name = layer.name,
        .kind = layer.kind,
        .visible = layer.visible,
    });
  }

  for (const PlacementRegion& region : project.board->placement_regions) {
    scene.placement_regions.push_back(CanvasPlacementRegion{
        .id = region.id,
        .kind = region.kind,
        .x_units = toMillimeters(region.area.origin.x),
        .y_units = toMillimeters(region.area.origin.y),
        .width_units = toMillimeters(region.area.size.width),
        .height_units = toMillimeters(region.area.size.height),
    });
  }

  for (const Keepout& keepout : project.board->keepouts) {
    scene.keepouts.push_back(CanvasKeepout{
        .id = keepout.id,
        .kind = keepout.kind,
        .x_units = toMillimeters(keepout.area.origin.x),
        .y_units = toMillimeters(keepout.area.origin.y),
        .width_units = toMillimeters(keepout.area.size.width),
        .height_units = toMillimeters(keepout.area.size.height),
    });
  }

  for (const Pad& pad : project.board->pads) {
    scene.pads.push_back(CanvasPad{
        .id = pad.id,
        .net_id = pad.net_id,
        .layer_id = pad.layer_id,
        .x_units = toMillimeters(pad.position.x),
        .y_units = toMillimeters(pad.position.y),
        .width_units = toMillimeters(pad.size.width),
        .height_units = toMillimeters(pad.size.height),
        .rotation_degrees = pad.rotation_degrees,
    });
  }

  for (const Via& via : project.board->vias) {
    scene.vias.push_back(CanvasVia{
        .id = via.id,
        .net_id = via.net_id,
        .x_units = toMillimeters(via.position.x),
        .y_units = toMillimeters(via.position.y),
        .diameter_units = toMillimeters(via.diameter),
        .drill_units = toMillimeters(via.drill),
    });
  }

  for (const TrackSegment& track : project.board->tracks) {
    scene.tracks.push_back(CanvasTrack{
        .id = track.id,
        .net_id = track.net_id,
        .layer_id = track.layer_id,
        .source_route_request_id = track.source_route_request_id,
        .start_x_units = toMillimeters(track.start.x),
        .start_y_units = toMillimeters(track.start.y),
        .end_x_units = toMillimeters(track.end.x),
        .end_y_units = toMillimeters(track.end.y),
        .width_units = toMillimeters(track.width),
    });
  }

  std::map<std::string, std::size_t> routed_segment_counts;
  for (const TrackSegment& track : project.board->tracks) {
    if (!track.source_route_request_id.empty()) {
      ++routed_segment_counts[track.source_route_request_id];
    }
  }
  for (const RouteRequest& request : project.board->route_requests) {
    scene.route_requests.push_back(CanvasRouteRequest{
        .id = request.id,
        .net_id = request.net_id,
        .from_object_id = request.from_object_id,
        .to_object_id = request.to_object_id,
        .preferred_layer_id = request.preferred_layer_id,
        .policy = request.policy,
        .width_nm = request.width.nanometers,
        .routed_segment_count = routed_segment_counts[request.id],
    });
  }
  return scene;
}

}  // namespace ccad

