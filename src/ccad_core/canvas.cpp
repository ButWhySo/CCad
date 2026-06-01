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
        .layers = pad.layers,
        .type = pad.type,
        .shape = pad.shape,
        .x_units = toMillimeters(pad.position.x),
        .y_units = toMillimeters(pad.position.y),
        .width_units = toMillimeters(pad.size.width),
        .height_units = toMillimeters(pad.size.height),
        .rotation_degrees = pad.rotation_degrees,
        .drill_units = pad.drill.has_value() ? toMillimeters(*pad.drill) : 0.0,
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

CanvasScene buildSchematicScene(const Project& project) {
  CanvasScene scene;
  scene.has_board = false;
  // Compute bounds based on components
  double min_x = 0;
  double min_y = 0;
  double max_x = 0;
  double max_y = 0;

  for (const Component& comp : project.components) {
    CanvasComponent cc;
    cc.id = comp.id;
    cc.part = comp.part;
    cc.x_units = toMillimeters(comp.position.x);
    cc.y_units = toMillimeters(comp.position.y);
    cc.rotation_degrees = comp.rotation_degrees;
    scene.components.push_back(cc);

    if (cc.x_units < min_x) min_x = cc.x_units;
    if (cc.y_units < min_y) min_y = cc.y_units;
    if (cc.x_units > max_x) max_x = cc.x_units;
    if (cc.y_units > max_y) max_y = cc.y_units;
  }

  for (const WireSegment& wire : project.wires) {
    CanvasWire cw;
    cw.net_id = wire.net_id;
    cw.start_x_units = toMillimeters(wire.start.x);
    cw.start_y_units = toMillimeters(wire.start.y);
    cw.end_x_units = toMillimeters(wire.end.x);
    cw.end_y_units = toMillimeters(wire.end.y);
    scene.wires.push_back(cw);

    if (cw.start_x_units < min_x) min_x = cw.start_x_units;
    if (cw.start_y_units < min_y) min_y = cw.start_y_units;
    if (cw.end_x_units < min_x) min_x = cw.end_x_units;
    if (cw.end_y_units < min_y) min_y = cw.end_y_units;
    
    if (cw.start_x_units > max_x) max_x = cw.start_x_units;
    if (cw.start_y_units > max_y) max_y = cw.start_y_units;
    if (cw.end_x_units > max_x) max_x = cw.end_x_units;
    if (cw.end_y_units > max_y) max_y = cw.end_y_units;
  }

  scene.view_width_units = max_x - min_x + 50.0;
  scene.view_height_units = max_y - min_y + 50.0;

  return scene;
}

CanvasScene buildCanvasScene(const Footprint& footprint) {
  CanvasScene scene;
  scene.has_board = false; // It's just a component preview
  
  for (const FootprintLine& line : footprint.lines) {
    scene.lines.push_back(CanvasLine{
        .id = "line_" + std::to_string(scene.lines.size()),
        .layer_id = line.layer,
        .start_x_units = toMillimeters(line.start.x),
        .start_y_units = toMillimeters(line.start.y),
        .end_x_units = toMillimeters(line.end.x),
        .end_y_units = toMillimeters(line.end.y),
        .width_units = toMillimeters(line.stroke_width)
    });
  }
  
  for (const FootprintArc& arc : footprint.arcs) {
    scene.arcs.push_back(CanvasArc{
        .id = "arc_" + std::to_string(scene.arcs.size()),
        .layer_id = arc.layer,
        .start_x_units = toMillimeters(arc.start.x),
        .start_y_units = toMillimeters(arc.start.y),
        .mid_x_units = toMillimeters(arc.center.x),
        .mid_y_units = toMillimeters(arc.center.y),
        .end_x_units = toMillimeters(arc.end.x),
        .end_y_units = toMillimeters(arc.end.y),
        .width_units = toMillimeters(arc.stroke_width)
    });
  }
  
  for (const FootprintCircle& circle : footprint.circles) {
    scene.circles.push_back(CanvasCircle{
        .id = "circle_" + std::to_string(scene.circles.size()),
        .layer_id = circle.layer,
        .center_x_units = toMillimeters(circle.center.x),
        .center_y_units = toMillimeters(circle.center.y),
        .radius_units = std::abs(toMillimeters(circle.end.x) - toMillimeters(circle.center.x)), // simplistic radius
        .width_units = toMillimeters(circle.stroke_width),
        .fill_type = "none"
    });
  }
  
  for (const FootprintPolyline& poly : footprint.polylines) {
    CanvasPolygon canvas_poly;
    canvas_poly.id = "poly_" + std::to_string(scene.polygons.size());
    canvas_poly.layer_id = poly.layer;
    canvas_poly.width_units = toMillimeters(poly.stroke_width);
    canvas_poly.fill_type = "none";
    for (const Point& pt : poly.points) {
      canvas_poly.pts_x_units.push_back(toMillimeters(pt.x));
      canvas_poly.pts_y_units.push_back(toMillimeters(pt.y));
    }
    scene.polygons.push_back(canvas_poly);
  }
  
  for (const FootprintText& text : footprint.texts) {
    scene.texts.push_back(CanvasText{
        .id = "text_" + std::to_string(scene.texts.size()),
        .layer_id = text.layer,
        .text = text.text,
        .x_units = toMillimeters(text.position.x),
        .y_units = toMillimeters(text.position.y),
        .rotation_degrees = text.rotation_degrees,
        .size_x_units = toMillimeters(text.size_width),
        .size_y_units = toMillimeters(text.size_height)
    });
  }

  for (const FootprintPad& pad : footprint.pads) {
    scene.pads.push_back(CanvasPad{
        .id = "pad_" + pad.number,
        .net_id = "",
        .layers = pad.layers,
        .type = pad.type,
        .shape = pad.shape,
        .x_units = toMillimeters(pad.position.x),
        .y_units = toMillimeters(pad.position.y),
        .width_units = toMillimeters(pad.size.width),
        .height_units = toMillimeters(pad.size.height),
        .rotation_degrees = pad.rotation_degrees,
        .drill_units = pad.drill.has_value() ? toMillimeters(*pad.drill) : 0.0,
    });
  }
  
  return scene;
}

CanvasScene buildCanvasScene(const Symbol& symbol) {
  CanvasScene scene;
  scene.has_board = false;
  
  for (const SymbolLine& line : symbol.lines) {
    scene.lines.push_back(CanvasLine{
        .id = "line_" + std::to_string(scene.lines.size()),
        .layer_id = "symbol",
        .start_x_units = toMillimeters(line.start.x),
        .start_y_units = toMillimeters(line.start.y),
        .end_x_units = toMillimeters(line.end.x),
        .end_y_units = toMillimeters(line.end.y),
        .width_units = toMillimeters(line.stroke_width)
    });
  }
  
  for (const SymbolArc& arc : symbol.arcs) {
    scene.arcs.push_back(CanvasArc{
        .id = "arc_" + std::to_string(scene.arcs.size()),
        .layer_id = "symbol",
        .start_x_units = toMillimeters(arc.start.x),
        .start_y_units = toMillimeters(arc.start.y),
        .mid_x_units = toMillimeters(arc.center.x),
        .mid_y_units = toMillimeters(arc.center.y),
        .end_x_units = toMillimeters(arc.end.x),
        .end_y_units = toMillimeters(arc.end.y),
        .width_units = toMillimeters(arc.stroke_width)
    });
  }
  
  for (const SymbolCircle& circle : symbol.circles) {
    scene.circles.push_back(CanvasCircle{
        .id = "circle_" + std::to_string(scene.circles.size()),
        .layer_id = "symbol",
        .center_x_units = toMillimeters(circle.center.x),
        .center_y_units = toMillimeters(circle.center.y),
        .radius_units = toMillimeters(circle.radius),
        .width_units = toMillimeters(circle.stroke_width),
        .fill_type = circle.fill_type
    });
  }
  
  for (const SymbolPolyline& poly : symbol.polylines) {
    CanvasPolygon canvas_poly;
    canvas_poly.id = "poly_" + std::to_string(scene.polygons.size());
    canvas_poly.layer_id = "symbol";
    canvas_poly.width_units = toMillimeters(poly.stroke_width);
    canvas_poly.fill_type = poly.fill_type;
    for (const Point& pt : poly.points) {
      canvas_poly.pts_x_units.push_back(toMillimeters(pt.x));
      canvas_poly.pts_y_units.push_back(toMillimeters(pt.y));
    }
    scene.polygons.push_back(canvas_poly);
  }
  
  for (const SymbolText& text : symbol.texts) {
    scene.texts.push_back(CanvasText{
        .id = "text_" + std::to_string(scene.texts.size()),
        .layer_id = "symbol",
        .text = text.text,
        .x_units = toMillimeters(text.position.x),
        .y_units = toMillimeters(text.position.y),
        .rotation_degrees = text.rotation_degrees,
        .size_x_units = toMillimeters(text.size),
        .size_y_units = toMillimeters(text.size)
    });
  }

  for (const SymbolPin& pin : symbol.pins) {
    scene.pads.push_back(CanvasPad{
        .id = "pin_" + pin.number,
        .net_id = "",
        .layers = {"symbol"},
        .type = "smd",
        .shape = "rect",
        .x_units = toMillimeters(pin.position.x),
        .y_units = toMillimeters(pin.position.y),
        .width_units = toMillimeters(pin.length),
        .height_units = 0.5,
        .rotation_degrees = pin.rotation_degrees,
    });
  }

  return scene;
}

}  // namespace ccad

