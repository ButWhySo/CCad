#include "ccad_core/canvas.hpp"

#include "ccad_core/board_outline_polygon.hpp"
#include "ccad_core/serialize.hpp"

#include <cmath>
#include <map>
#include <optional>
#include <string>

namespace ccad {
namespace {

double toMillimeters(const Length& length) {
  return static_cast<double>(length.nanometers) / 1000000.0;
}

struct CanvasPoint {
  double x_units = 0.0;
  double y_units = 0.0;
};

CanvasPoint rotatePoint(const double x_units, const double y_units, const double rotation_degrees) {
  constexpr double pi = 3.14159265358979323846;
  const double radians = rotation_degrees * pi / 180.0;
  const double cos_theta = std::cos(radians);
  const double sin_theta = std::sin(radians);
  return CanvasPoint{
      .x_units = (x_units * cos_theta) - (y_units * sin_theta),
      .y_units = (x_units * sin_theta) + (y_units * cos_theta),
  };
}

CanvasPoint transformSymbolPoint(const Component& component, const double x_units,
                                 const double y_units) {
  const CanvasPoint rotated = rotatePoint(x_units, y_units, component.rotation_degrees);
  return CanvasPoint{
      .x_units = toMillimeters(component.position.x) + rotated.x_units,
      .y_units = toMillimeters(component.position.y) + rotated.y_units,
  };
}

void includeBounds(double& min_x, double& min_y, double& max_x, double& max_y,
                   const double x_units, const double y_units) {
  if (x_units < min_x) min_x = x_units;
  if (y_units < min_y) min_y = y_units;
  if (x_units > max_x) max_x = x_units;
  if (y_units > max_y) max_y = y_units;
}

void mergePlacedSymbolScene(CanvasScene& scene, const Component& component,
                            const CanvasScene& symbol_scene, double& min_x, double& min_y,
                            double& max_x, double& max_y) {
  const std::string prefix = component.id + ".";

  for (CanvasLine line : symbol_scene.lines) {
    const CanvasPoint start =
        transformSymbolPoint(component, line.start_x_units, line.start_y_units);
    const CanvasPoint end = transformSymbolPoint(component, line.end_x_units, line.end_y_units);
    line.id = prefix + line.id;
    line.start_x_units = start.x_units;
    line.start_y_units = start.y_units;
    line.end_x_units = end.x_units;
    line.end_y_units = end.y_units;
    scene.lines.push_back(line);
    includeBounds(min_x, min_y, max_x, max_y, line.start_x_units, line.start_y_units);
    includeBounds(min_x, min_y, max_x, max_y, line.end_x_units, line.end_y_units);
  }

  for (CanvasArc arc : symbol_scene.arcs) {
    const CanvasPoint start = transformSymbolPoint(component, arc.start_x_units, arc.start_y_units);
    const CanvasPoint mid = transformSymbolPoint(component, arc.mid_x_units, arc.mid_y_units);
    const CanvasPoint end = transformSymbolPoint(component, arc.end_x_units, arc.end_y_units);
    arc.id = prefix + arc.id;
    arc.start_x_units = start.x_units;
    arc.start_y_units = start.y_units;
    arc.mid_x_units = mid.x_units;
    arc.mid_y_units = mid.y_units;
    arc.end_x_units = end.x_units;
    arc.end_y_units = end.y_units;
    scene.arcs.push_back(arc);
    includeBounds(min_x, min_y, max_x, max_y, arc.start_x_units, arc.start_y_units);
    includeBounds(min_x, min_y, max_x, max_y, arc.mid_x_units, arc.mid_y_units);
    includeBounds(min_x, min_y, max_x, max_y, arc.end_x_units, arc.end_y_units);
  }

  for (CanvasCircle circle : symbol_scene.circles) {
    const CanvasPoint center =
        transformSymbolPoint(component, circle.center_x_units, circle.center_y_units);
    circle.id = prefix + circle.id;
    circle.center_x_units = center.x_units;
    circle.center_y_units = center.y_units;
    scene.circles.push_back(circle);
    includeBounds(min_x, min_y, max_x, max_y, circle.center_x_units - circle.radius_units,
                  circle.center_y_units - circle.radius_units);
    includeBounds(min_x, min_y, max_x, max_y, circle.center_x_units + circle.radius_units,
                  circle.center_y_units + circle.radius_units);
  }

  for (CanvasPolygon polygon : symbol_scene.polygons) {
    polygon.id = prefix + polygon.id;
    for (std::size_t i = 0; i < polygon.pts_x_units.size(); ++i) {
      const CanvasPoint point =
          transformSymbolPoint(component, polygon.pts_x_units.at(i), polygon.pts_y_units.at(i));
      polygon.pts_x_units.at(i) = point.x_units;
      polygon.pts_y_units.at(i) = point.y_units;
      includeBounds(min_x, min_y, max_x, max_y, point.x_units, point.y_units);
    }
    scene.polygons.push_back(polygon);
  }

  for (CanvasText text : symbol_scene.texts) {
    const CanvasPoint position = transformSymbolPoint(component, text.x_units, text.y_units);
    text.id = prefix + text.id;
    text.x_units = position.x_units;
    text.y_units = position.y_units;
    text.rotation_degrees += component.rotation_degrees;
    scene.texts.push_back(text);
    includeBounds(min_x, min_y, max_x, max_y, text.x_units, text.y_units);
  }
}

}  // namespace

CanvasScene buildCanvasScene(const Board& board) {
  CanvasScene scene;
  scene.has_board = true;
  scene.board_width_nm = board.outline.size.width.nanometers;
  scene.board_height_nm = board.outline.size.height.nanometers;
  scene.board_origin_x_units = toMillimeters(board.outline.origin.x);
  scene.board_origin_y_units = toMillimeters(board.outline.origin.y);
  scene.view_width_units = toMillimeters(board.outline.size.width);
  scene.view_height_units = toMillimeters(board.outline.size.height);
  scene.board_bounding_boxes.push_back(CanvasBoardBoundingBox{
      .id = "board.bounding_box",
      .class_name = "BOARD_BOUNDING_BOX",
      .layer_id = "LAYER_BOARD_BOUNDING_BOX",
      .skip_struct = true,
      .x_units = toMillimeters(board.outline.origin.x),
      .y_units = toMillimeters(board.outline.origin.y),
      .width_units = toMillimeters(board.outline.size.width),
      .height_units = toMillimeters(board.outline.size.height),
  });

  for (const Layer& layer : board.layers) {
    scene.layers.push_back(CanvasLayer{
        .id = layer.id,
        .name = layer.name,
        .kind = layer.kind,
        .visible = layer.visible,
    });
  }

  for (const PlacementRegion& region : board.placement_regions) {
    scene.placement_regions.push_back(CanvasPlacementRegion{
        .id = region.id,
        .kind = region.kind,
        .x_units = toMillimeters(region.area.origin.x),
        .y_units = toMillimeters(region.area.origin.y),
        .width_units = toMillimeters(region.area.size.width),
        .height_units = toMillimeters(region.area.size.height),
    });
  }

  for (const Keepout& keepout : board.keepouts) {
    scene.keepouts.push_back(CanvasKeepout{
        .id = keepout.id,
        .kind = keepout.kind,
        .x_units = toMillimeters(keepout.area.origin.x),
        .y_units = toMillimeters(keepout.area.origin.y),
        .width_units = toMillimeters(keepout.area.size.width),
        .height_units = toMillimeters(keepout.area.size.height),
    });
  }

  for (const Pad& pad : board.pads) {
      std::string cshape = "circle";
      if (!pad.padstack.copper_props.empty()) {
        const auto shape_enum = pad.padstack.copper_props.begin()->second.shape.shape;
        if (shape_enum == ccad::PadShape::Rectangle) cshape = "rect";
        else if (shape_enum == ccad::PadShape::Oval) cshape = "oval";
        else if (shape_enum == ccad::PadShape::Trapezoid) cshape = "trapezoid";
        else if (shape_enum == ccad::PadShape::RoundRect) cshape = "roundrect";
        else if (shape_enum == ccad::PadShape::ChamferedRect) cshape = "chamfered_rect";
        else if (shape_enum == ccad::PadShape::Custom) cshape = "custom";
      }

    scene.pads.push_back(CanvasPad{
        .id = pad.id,
        .net_id = pad.net_id,
        .layers = pad.padstack.layer_set,
        .type = pad.type,
        .shape = cshape,
        .x_units = toMillimeters(pad.position.x),
        .y_units = toMillimeters(pad.position.y),
        .width_units = pad.padstack.copper_props.empty() ? 0.0 : toMillimeters(pad.padstack.copper_props.begin()->second.shape.size.width),
        .height_units = pad.padstack.copper_props.empty() ? 0.0 : toMillimeters(pad.padstack.copper_props.begin()->second.shape.size.height),
        .rotation_degrees = pad.rotation_degrees,
        .drill_units = toMillimeters(pad.padstack.drill.size.width),
        .secondary_drill_units = pad.padstack.secondary_drill.has_value() ? toMillimeters(pad.padstack.secondary_drill->size.width) : 0.0,
        .tertiary_drill_units = pad.padstack.tertiary_drill.has_value() ? toMillimeters(pad.padstack.tertiary_drill->size.width) : 0.0,
        .backdrilled = (pad.padstack.front_post_machining.mode == "backdrill" || pad.padstack.back_post_machining.mode == "backdrill"),
        .front_post_machining_units = pad.padstack.front_post_machining.mode.has_value() ? toMillimeters(pad.padstack.front_post_machining.size) : 0.0,
        .back_post_machining_units = pad.padstack.back_post_machining.mode.has_value() ? toMillimeters(pad.padstack.back_post_machining.size) : 0.0,
        .pin_type = pad.pin_type,
        .pad_to_die_length_units = pad.pad_to_die_length.has_value() ? toMillimeters(*pad.pad_to_die_length) : 0.0,
        .pad_to_die_delay = pad.pad_to_die_delay.value_or(0.0),
        .roundrect_rratio = pad.padstack.copper_props.empty() ? 0.0 : pad.padstack.copper_props.begin()->second.shape.roundrect_rratio,
        .chamfer_ratio = pad.padstack.copper_props.empty() ? 0.0 : pad.padstack.copper_props.begin()->second.shape.chamfer_ratio,
    });
  }

  for (const Via& via : board.vias) {
    scene.vias.push_back(CanvasVia{
        .id = via.id,
        .net_id = via.net_id,
        .x_units = toMillimeters(via.position.x),
        .y_units = toMillimeters(via.position.y),
        .diameter_units = toMillimeters(via.diameter),
        .drill_units = toMillimeters(via.drill),
    });
  }

  for (const TrackSegment& track : board.tracks) {
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

  for (const BoardGraphic& graphic : board.graphics) {
    if (graphic.kind == "line") {
      scene.lines.push_back(CanvasLine{
          .id = graphic.id,
          .layer_id = graphic.layer_id,
          .start_x_units = toMillimeters(graphic.start.x),
          .start_y_units = toMillimeters(graphic.start.y),
          .end_x_units = toMillimeters(graphic.end.x),
          .end_y_units = toMillimeters(graphic.end.y),
          .width_units = toMillimeters(graphic.width),
      });
    }
  }

  for (const BoardText& text : board.texts) {
    scene.texts.push_back(CanvasText{
        .id = text.id,
        .layer_id = text.layer_id,
        .text = text.text,
        .x_units = toMillimeters(text.position.x),
        .y_units = toMillimeters(text.position.y),
        .rotation_degrees = text.rotation_degrees,
        .size_x_units = toMillimeters(text.size.width),
        .size_y_units = toMillimeters(text.size.height),
    });
  }

  for (const BoardZone& zone : board.zones) {
    CanvasZone canvas_zone{
        .id = zone.id,
        .name = zone.name,
        .net_id = zone.net_id,
        .layer_ids = zone.layer_ids,
        .pts_x_units = {},
        .pts_y_units = {},
        .priority = zone.priority,
        .clearance_units = toMillimeters(zone.clearance),
        .min_thickness_units = toMillimeters(zone.min_thickness),
        .fill_enabled = zone.fill_enabled,
        .pad_connection = zone.pad_connection,
    };
    for (const Point& point : zone.outline) {
      canvas_zone.pts_x_units.push_back(toMillimeters(point.x));
      canvas_zone.pts_y_units.push_back(toMillimeters(point.y));
    }
    scene.zones.push_back(canvas_zone);
  }

  for (const BoardBarcode& barcode : board.barcodes) {
    scene.barcodes.push_back(CanvasBarcode{
        .id = barcode.id,
        .layer_id = barcode.layer_id,
        .text = barcode.text,
        .kind = formatBarcodeType(barcode.kind),
        .x_units = toMillimeters(barcode.position.x),
        .y_units = toMillimeters(barcode.position.y),
        .rotation_degrees = barcode.rotation_degrees,
        .width_units = toMillimeters(barcode.size.width),
        .height_units = toMillimeters(barcode.size.height),
    });
  }

  for (const BoardTarget& target : board.targets) {
    scene.targets.push_back(CanvasTarget{
        .id = target.id,
        .layer_id = target.layer_id,
        .shape = target.shape == TargetShape::Plus ? "Plus" : "X",
        .x_units = toMillimeters(target.position_x),
        .y_units = toMillimeters(target.position_y),
        .size_units = toMillimeters(target.size),
        .line_width_units = toMillimeters(target.line_width),
    });
  }

  for (const BoardDimension& dim : board.dimensions) {
    scene.dimensions.push_back(CanvasDimension{
        .id = dim.id,
        .layer_id = dim.layer_id,
        .text = dim.text,
        .start_x_units = toMillimeters(dim.start.x),
        .start_y_units = toMillimeters(dim.start.y),
        .end_x_units = toMillimeters(dim.end.x),
        .end_y_units = toMillimeters(dim.end.y),
        .text_x_units = toMillimeters(dim.text_position.x),
        .text_y_units = toMillimeters(dim.text_position.y),
    });
  }

  std::map<std::string, std::size_t> routed_segment_counts;
  for (const TrackSegment& track : board.tracks) {
    if (!track.source_route_request_id.empty()) {
      ++routed_segment_counts[track.source_route_request_id];
    }
  }
  for (const RouteRequest& request : board.route_requests) {
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

CanvasScene buildSchematicScene(const Schematic& schematic) {
  CanvasScene scene;
  scene.has_board = false;
  // Compute bounds based on components
  double min_x = 0;
  double min_y = 0;
  double max_x = 0;
  double max_y = 0;
  bool has_bounds = false;
  const auto includeSchematicBounds = [&](const double x_units, const double y_units) {
    if (!has_bounds) {
      min_x = x_units;
      min_y = y_units;
      max_x = x_units;
      max_y = y_units;
      has_bounds = true;
      return;
    }
    includeBounds(min_x, min_y, max_x, max_y, x_units, y_units);
  };

  for (const Component& comp : schematic.components) {
    CanvasComponent cc;
    cc.id = comp.id;
    cc.part = comp.part;
    cc.x_units = toMillimeters(comp.position.x);
    cc.y_units = toMillimeters(comp.position.y);
    cc.rotation_degrees = comp.rotation_degrees;
    cc.has_symbol_graphics = comp.symbol.has_value();
    scene.components.push_back(cc);

    includeSchematicBounds(cc.x_units, cc.y_units);
    if (comp.symbol.has_value()) {
      mergePlacedSymbolScene(scene, comp, buildCanvasScene(*comp.symbol), min_x, min_y, max_x,
                             max_y);
    }
  }

  for (const WireSegment& wire : schematic.wires) {
    CanvasWire cw;
    cw.net_id = wire.net_id;
    cw.start_x_units = toMillimeters(wire.start.x);
    cw.start_y_units = toMillimeters(wire.start.y);
    cw.end_x_units = toMillimeters(wire.end.x);
    cw.end_y_units = toMillimeters(wire.end.y);
    scene.wires.push_back(cw);

    includeSchematicBounds(cw.start_x_units, cw.start_y_units);
    includeSchematicBounds(cw.end_x_units, cw.end_y_units);
  }

  for (const BusSegment& bus : schematic.buses) {
    CanvasBusSegment cbs;
    cbs.bus_id = bus.bus_id;
    cbs.start_x_units = toMillimeters(bus.start.x);
    cbs.start_y_units = toMillimeters(bus.start.y);
    cbs.end_x_units = toMillimeters(bus.end.x);
    cbs.end_y_units = toMillimeters(bus.end.y);
    scene.bus_segments.push_back(cbs);

    includeSchematicBounds(cbs.start_x_units, cbs.start_y_units);
    includeSchematicBounds(cbs.end_x_units, cbs.end_y_units);
  }

  for (const Label& label : schematic.labels) {
    CanvasLabel cl;
    cl.id = label.id;
    cl.text = label.text;
    cl.net_id = label.net_id;
    cl.x_units = toMillimeters(label.position.x);
    cl.y_units = toMillimeters(label.position.y);
    cl.rotation_degrees = label.rotation_degrees;
    cl.global = label.global;
    scene.labels.push_back(cl);

    includeSchematicBounds(cl.x_units, cl.y_units);
  }

  for (const PowerSymbol& ps : schematic.power_symbols) {
    CanvasPowerSymbol cps;
    cps.id = ps.id;
    cps.value = ps.value;
    cps.net_id = ps.net_id;
    cps.x_units = toMillimeters(ps.position.x);
    cps.y_units = toMillimeters(ps.position.y);
    cps.rotation_degrees = ps.rotation_degrees;
    scene.power_symbols.push_back(cps);

    includeSchematicBounds(cps.x_units, cps.y_units);
  }

  if (has_bounds) {
    constexpr double padding_units = 10.0;
    scene.board_origin_x_units = min_x - (padding_units / 2.0);
    scene.board_origin_y_units = min_y - (padding_units / 2.0);
    scene.view_width_units = (max_x - min_x) + padding_units;
    scene.view_height_units = (max_y - min_y) + padding_units;
  } else {
    scene.view_width_units = 50.0;
    scene.view_height_units = 50.0;
  }

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
        .secondary_drill_units = pad.secondary_drill.has_value() ? toMillimeters(*pad.secondary_drill) : 0.0,
        .tertiary_drill_units = pad.tertiary_drill.has_value() ? toMillimeters(*pad.tertiary_drill) : 0.0,
        .backdrilled = pad.backdrilled,
        .front_post_machining_units = pad.front_post_machining.has_value() ? toMillimeters(*pad.front_post_machining) : 0.0,
        .back_post_machining_units = pad.back_post_machining.has_value() ? toMillimeters(*pad.back_post_machining) : 0.0,
        .pin_type = pad.pin_type,
        .pad_to_die_length_units = pad.pad_to_die_length.has_value() ? toMillimeters(*pad.pad_to_die_length) : 0.0,
        .pad_to_die_delay = pad.pad_to_die_delay.value_or(0.0),
        .roundrect_rratio = pad.roundrect_rratio,
        .chamfer_ratio = pad.chamfer_ratio,
    });
  }
  
  return scene;
}

CanvasScene buildCanvasScene(const Symbol& symbol) {
  CanvasScene scene;
  scene.has_board = false;

  for (const SymbolRectangle& rectangle : symbol.rectangles) {
    CanvasPolygon canvas_rectangle;
    canvas_rectangle.id = "rect_" + std::to_string(scene.polygons.size());
    canvas_rectangle.layer_id = "symbol";
    canvas_rectangle.width_units = toMillimeters(rectangle.stroke_width);
    canvas_rectangle.fill_type = rectangle.fill_type;
    canvas_rectangle.pts_x_units = {
        toMillimeters(rectangle.start.x),
        toMillimeters(rectangle.end.x),
        toMillimeters(rectangle.end.x),
        toMillimeters(rectangle.start.x),
    };
    canvas_rectangle.pts_y_units = {
        toMillimeters(rectangle.start.y),
        toMillimeters(rectangle.start.y),
        toMillimeters(rectangle.end.y),
        toMillimeters(rectangle.end.y),
    };
    scene.polygons.push_back(canvas_rectangle);
  }
  
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
    const double start_x_units = toMillimeters(pin.position.x);
    const double start_y_units = toMillimeters(pin.position.y);
    const double pin_length_units =
        pin.length.nanometers == 0 ? 2.54 : toMillimeters(pin.length);
    const CanvasPoint pin_vector = rotatePoint(pin_length_units, 0.0, pin.rotation_degrees);
    scene.lines.push_back(CanvasLine{
        .id = "pin_" + pin.number,
        .layer_id = "symbol_pin",
        .start_x_units = start_x_units,
        .start_y_units = start_y_units,
        .end_x_units = start_x_units + pin_vector.x_units,
        .end_y_units = start_y_units + pin_vector.y_units,
        .width_units = 0.12,
    });
  }

  return scene;
}

}  // namespace ccad

