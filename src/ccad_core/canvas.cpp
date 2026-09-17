#include "ccad_core/canvas.hpp"

#include "ccad_core/board_outline_polygon.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/third_party/qrcodegen/qrcodegen.hpp"

#include <cmath>
#include <algorithm>
#include <map>
#include <optional>
#include <string>

namespace ccad {
namespace {

// toMillimeters is now provided by geometry.hpp

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




void includeBounds(double& min_x, double& min_y, double& max_x, double& max_y,
                   const double x_units, const double y_units) {
  if (x_units < min_x) min_x = x_units;
  if (y_units < min_y) min_y = y_units;
  if (x_units > max_x) max_x = x_units;
  if (y_units > max_y) max_y = y_units;
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

  for (const BoardFootprint& footprint : board.footprints) {
    scene.footprints.push_back(CanvasFootprint{
        .id = footprint.reference,
        .reference = footprint.reference,
        .value = footprint.value,
        .layer_id = footprint.layer_id,
        .x_units = toMillimeters(footprint.position.x),
        .y_units = toMillimeters(footprint.position.y),
        .rotation_degrees = footprint.rotation_degrees,
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

  for (const TrackArc& arc : board.track_arcs) {
    scene.track_arcs.push_back(CanvasTrackArc{
        .id = arc.id,
        .net_id = arc.net_id,
        .layer_id = arc.layer_id,
        .start_x_units = toMillimeters(arc.start.x),
        .start_y_units = toMillimeters(arc.start.y),
        .mid_x_units = toMillimeters(arc.mid.x),
        .mid_y_units = toMillimeters(arc.mid.y),
        .end_x_units = toMillimeters(arc.end.x),
        .end_y_units = toMillimeters(arc.end.y),
        .width_units = toMillimeters(arc.width)
    });
  }

  for (const BoardGraphic& graphic : board.graphics) {
    if (graphic.kind == "arc") {
      scene.arcs.push_back(CanvasArc{
          .id = graphic.id,
          .layer_id = graphic.layer_id,
          .start_x_units = toMillimeters(graphic.start.x),
          .start_y_units = toMillimeters(graphic.start.y),
          .mid_x_units = graphic.mid.has_value() ? toMillimeters(graphic.mid->x) : 0.0,
          .mid_y_units = graphic.mid.has_value() ? toMillimeters(graphic.mid->y) : 0.0,
          .end_x_units = toMillimeters(graphic.end.x),
          .end_y_units = toMillimeters(graphic.end.y),
          .width_units = toMillimeters(graphic.width)
      });
    } else if (graphic.kind == "circle") {
      scene.circles.push_back(CanvasCircle{
          .id = graphic.id,
          .layer_id = graphic.layer_id,
          .center_x_units = toMillimeters(graphic.start.x),
          .center_y_units = toMillimeters(graphic.start.y),
          .radius_units = std::abs(toMillimeters(graphic.end.x) - toMillimeters(graphic.start.x)),
          .width_units = toMillimeters(graphic.width),
          .fill_type = "none",
      });
    } else if (graphic.kind == "line") {
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
        .thermal_spokes = {},
    };
    for (const Point& point : zone.outline) {
      canvas_zone.pts_x_units.push_back(toMillimeters(point.x));
      canvas_zone.pts_y_units.push_back(toMillimeters(point.y));
    }
    for (std::size_t index = 0; index < zone.filled_thermal_spokes.size(); ++index) {
      const auto& spoke = zone.filled_thermal_spokes.at(index);
      canvas_zone.thermal_spokes.push_back(CanvasLine{
          .id = zone.id + "_thermal_" + std::to_string(index),
          .layer_id = zone.layer_ids.empty() ? "" : zone.layer_ids.front(),
          .start_x_units = toMillimeters(spoke.start.x),
          .start_y_units = toMillimeters(spoke.start.y),
          .end_x_units = toMillimeters(spoke.end.x),
          .end_y_units = toMillimeters(spoke.end.y),
          .width_units = toMillimeters(spoke.width)});
    }
    scene.zones.push_back(canvas_zone);
  }

  for (const BoardTeardrop& td : board.teardrops) {
    CanvasZone shape;
    shape.id = td.id;
    shape.name = "Teardrop";
    shape.net_id = td.net_id;
    shape.layer_ids = {td.layer_id};
    shape.is_teardrop = true;
    for (const Point& point : td.outline) {
      shape.pts_x_units.push_back(toMillimeters(point.x));
      shape.pts_y_units.push_back(toMillimeters(point.y));
    }
    scene.zones.push_back(std::move(shape));
  }

  for (const BoardBarcode& barcode : board.barcodes) {
    CanvasBarcode cb{
        .id = barcode.id,
        .layer_id = barcode.layer_id,
        .text = barcode.text,
        .kind = formatBarcodeType(barcode.kind),
        .x_units = toMillimeters(barcode.position.x),
        .y_units = toMillimeters(barcode.position.y),
        .rotation_degrees = barcode.rotation_degrees,
        .width_units = toMillimeters(barcode.size.width),
        .height_units = toMillimeters(barcode.size.height),
        .modules = {}
    };
    
    if (barcode.kind == BarcodeType::QRCode || barcode.kind == BarcodeType::MicroQRCode) {
      qrcodegen::QrCode::Ecc ecc = qrcodegen::QrCode::Ecc::LOW;
      if (barcode.error_correction == BarcodeEcc::Medium) ecc = qrcodegen::QrCode::Ecc::MEDIUM;
      else if (barcode.error_correction == BarcodeEcc::Quartile) ecc = qrcodegen::QrCode::Ecc::QUARTILE;
      else if (barcode.error_correction == BarcodeEcc::High) ecc = qrcodegen::QrCode::Ecc::HIGH;
      
      try {
        qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(barcode.text.c_str(), ecc);
        int size = qr.getSize();
        double w_unit = cb.width_units / size;
        double h_unit = cb.height_units / size;
        
        for (int y = 0; y < size; ++y) {
          for (int x = 0; x < size; ++x) {
            if (qr.getModule(x, y)) {
               cb.modules.push_back(CanvasBarcode::Module{
                 .x_units = -cb.width_units / 2.0 + (x + 0.5) * w_unit,
                 .y_units = -cb.height_units / 2.0 + (y + 0.5) * h_unit,
                 .w_units = w_unit,
                 .h_units = h_unit
               });
            }
          }
        }
      } catch (const std::exception&) {
         // ignore parsing errors for now
      }
    } else {
       // Dummy block for non-QR codes to at least render something
       cb.modules.push_back(CanvasBarcode::Module{0.0, 0.0, cb.width_units, cb.height_units});
    }
    
    scene.barcodes.push_back(cb);
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
    CanvasDimension cdim{
        .id = dim.id,
        .layer_id = dim.layer_id,
        .text = dim.text,
        .text_x_units = toMillimeters(dim.text_position.x),
        .text_y_units = toMillimeters(dim.text_position.y),
        .lines = {},
        .arrows = {}
    };

    double sx = toMillimeters(dim.start.x);
    double sy = toMillimeters(dim.start.y);
    double ex = toMillimeters(dim.end.x);
    double ey = toMillimeters(dim.end.y);
    double tx = cdim.text_x_units;
    double ty = cdim.text_y_units;

    double dx = ex - sx;
    double dy = ey - sy;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist > 0.001) {
       double ux = dx / dist;
       double uy = dy / dist;

       double v1x = sx - tx;
       double v1y = sy - ty;
       double proj1 = v1x * ux + v1y * uy;
       double cross_sx = tx + proj1 * ux;
       double cross_sy = ty + proj1 * uy;
       
       double v2x = ex - tx;
       double v2y = ey - ty;
       double proj2 = v2x * ux + v2y * uy;
       double cross_ex = tx + proj2 * ux;
       double cross_ey = ty + proj2 * uy;
       
       cdim.lines.push_back({cross_sx, cross_sy, cross_ex, cross_ey});
       
       double overhang = 1.0;
       double ext_dx = -uy * overhang;
       double ext_dy = ux * overhang;
       double cross_dist1 = v1x * (-uy) + v1y * (ux);
       if (cross_dist1 < 0) { ext_dx = -ext_dx; ext_dy = -ext_dy; }
       
       cdim.lines.push_back({sx, sy, cross_sx + ext_dx, cross_sy + ext_dy});
       cdim.lines.push_back({ex, ey, cross_ex + ext_dx, cross_ey + ext_dy});
       
       double arrow_len = 1.5;
       double arrow_width = 0.5;

       {
         CanvasDimension::ArrowPolygon arrow1;
         arrow1.pts_x_units = {cross_sx, cross_sx + arrow_len * ux + arrow_width * uy,
                                cross_sx + arrow_len * ux - arrow_width * uy};
         arrow1.pts_y_units = {cross_sy, cross_sy + arrow_len * uy - arrow_width * ux,
                                cross_sy + arrow_len * uy + arrow_width * ux};
         cdim.arrows.push_back(arrow1);
       }

       {
         CanvasDimension::ArrowPolygon arrow2;
         arrow2.pts_x_units = {cross_ex, cross_ex - arrow_len * ux + arrow_width * uy,
                                cross_ex - arrow_len * ux - arrow_width * uy};
         arrow2.pts_y_units = {cross_ey, cross_ey - arrow_len * uy - arrow_width * ux,
                                cross_ey - arrow_len * uy + arrow_width * ux};
         cdim.arrows.push_back(arrow2);
       }
    }

    scene.dimensions.push_back(cdim);
  }

  for (const BoardReferenceImage& ref : board.reference_images) {
    scene.reference_images.push_back(CanvasReferenceImage{
        .id = ref.id,
        .layer_id = ref.layer,
        .data = ref.data,
        .x_units = ref.x_mm,
        .y_units = ref.y_mm,
        .scale = ref.scale,
        .opacity = ref.opacity,
    });
  }

  for (const BoardTable& table : board.tables) {
    scene.tables.push_back(CanvasTable{
        .id = table.id,
        .layer_id = table.layer,
        .x_units = table.x_mm,
        .y_units = table.y_mm,
        .rows = table.rows,
        .cols = table.cols,
        .width_units = table.width_mm,
        .height_units = table.height_mm,
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
  const auto transformPlacedPoint = [](const SchSymbol& comp, const double x_units,
                                       const double y_units) {
    const CanvasPoint rotated = rotatePoint(x_units, y_units, comp.rotation_degrees);
    return CanvasPoint{
        .x_units = toMillimeters(comp.position.x) + rotated.x_units,
        .y_units = toMillimeters(comp.position.y) + rotated.y_units,
    };
  };

  const bool needs_display_layout = schematic.symbols.size() > 1 &&
      std::all_of(schematic.symbols.begin(), schematic.symbols.end(), [](const SchSymbol& comp) {
        return comp.position.x.nanometers == 0 && comp.position.y.nanometers == 0;
      });
  for (std::size_t comp_index = 0; comp_index < schematic.symbols.size(); ++comp_index) {
    SchSymbol comp = schematic.symbols[comp_index];
    if (needs_display_layout) {
      constexpr double column_spacing = 25.0;
      constexpr double row_spacing = 18.0;
      comp.position = {millimeters(15.0 + (comp_index % 4) * column_spacing),
                       millimeters(15.0 + (comp_index / 4) * row_spacing)};
    }
    CanvasComponent cc;
    cc.id = comp.id;
    cc.part = comp.lib_id;
    cc.x_units = toMillimeters(comp.position.x);
    cc.y_units = toMillimeters(comp.position.y);
    cc.rotation_degrees = comp.rotation_degrees;
    cc.has_symbol_graphics = comp.symbol.has_value();
    scene.symbols.push_back(cc);

    includeSchematicBounds(cc.x_units, cc.y_units);

    if (comp.symbol.has_value()) {
      CanvasScene local_scene = buildCanvasScene(*comp.symbol);
      for (CanvasLine line : local_scene.lines) {
        const CanvasPoint start =
            transformPlacedPoint(comp, line.start_x_units, line.start_y_units);
        const CanvasPoint end = transformPlacedPoint(comp, line.end_x_units, line.end_y_units);
        line.id = comp.id + "." + line.id;
        line.start_x_units = start.x_units;
        line.start_y_units = start.y_units;
        line.end_x_units = end.x_units;
        line.end_y_units = end.y_units;
        scene.lines.push_back(line);
        includeSchematicBounds(start.x_units, start.y_units);
        includeSchematicBounds(end.x_units, end.y_units);
      }
      for (CanvasArc arc : local_scene.arcs) {
        const CanvasPoint start =
            transformPlacedPoint(comp, arc.start_x_units, arc.start_y_units);
        const CanvasPoint mid = transformPlacedPoint(comp, arc.mid_x_units, arc.mid_y_units);
        const CanvasPoint end = transformPlacedPoint(comp, arc.end_x_units, arc.end_y_units);
        arc.id = comp.id + "." + arc.id;
        arc.start_x_units = start.x_units;
        arc.start_y_units = start.y_units;
        arc.mid_x_units = mid.x_units;
        arc.mid_y_units = mid.y_units;
        arc.end_x_units = end.x_units;
        arc.end_y_units = end.y_units;
        scene.arcs.push_back(arc);
        includeSchematicBounds(start.x_units, start.y_units);
        includeSchematicBounds(mid.x_units, mid.y_units);
        includeSchematicBounds(end.x_units, end.y_units);
      }
      for (CanvasCircle circle : local_scene.circles) {
        const CanvasPoint center =
            transformPlacedPoint(comp, circle.center_x_units, circle.center_y_units);
        circle.id = comp.id + "." + circle.id;
        circle.center_x_units = center.x_units;
        circle.center_y_units = center.y_units;
        scene.circles.push_back(circle);
        includeSchematicBounds(center.x_units - circle.radius_units,
                               center.y_units - circle.radius_units);
        includeSchematicBounds(center.x_units + circle.radius_units,
                               center.y_units + circle.radius_units);
      }
      for (CanvasPolygon polygon : local_scene.polygons) {
        polygon.id = comp.id + "." + polygon.id;
        for (std::size_t i = 0; i < polygon.pts_x_units.size() &&
                                i < polygon.pts_y_units.size(); ++i) {
          const CanvasPoint transformed =
              transformPlacedPoint(comp, polygon.pts_x_units.at(i), polygon.pts_y_units.at(i));
          polygon.pts_x_units.at(i) = transformed.x_units;
          polygon.pts_y_units.at(i) = transformed.y_units;
          includeSchematicBounds(transformed.x_units, transformed.y_units);
        }
        scene.polygons.push_back(polygon);
      }
      for (CanvasText text : local_scene.texts) {
        const CanvasPoint position = transformPlacedPoint(comp, text.x_units, text.y_units);
        text.id = comp.id + "." + text.id;
        text.x_units = position.x_units;
        text.y_units = position.y_units;
        text.rotation_degrees += comp.rotation_degrees;
        scene.texts.push_back(text);
        includeSchematicBounds(position.x_units, position.y_units);
      }
    }
  }

  for (const SchWire& wire : schematic.wires) {
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

  for (const SchBus& bus : schematic.buses) {
    CanvasSchBus cbs;
    cbs.id = bus.id;
    cbs.bus_id = bus.bus_id;
    cbs.start_x_units = toMillimeters(bus.start.x);
    cbs.start_y_units = toMillimeters(bus.start.y);
    cbs.end_x_units = toMillimeters(bus.end.x);
    cbs.end_y_units = toMillimeters(bus.end.y);
    scene.bus_segments.push_back(cbs);

    includeSchematicBounds(cbs.start_x_units, cbs.start_y_units);
    includeSchematicBounds(cbs.end_x_units, cbs.end_y_units);
  }

  for (const SchLabel& label : schematic.labels) {
    CanvasLabel cl;
    cl.id = label.id;
    cl.text = label.text;
    cl.net_id = label.net_id;
    cl.x_units = toMillimeters(label.position.x);
    cl.y_units = toMillimeters(label.position.y);
    cl.rotation_degrees = label.rotation_degrees;
    cl.global = (label.type == LabelType::Global);
    scene.labels.push_back(cl);

    includeSchematicBounds(cl.x_units, cl.y_units);
  }

  for (const SchPowerSymbol& ps : schematic.power_symbols) {
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

  for (const SchJunction& junction : schematic.junctions) {
    CanvasCircle cc;
    cc.id = junction.id;
    cc.layer_id = "schematic_junction";
    cc.center_x_units = toMillimeters(junction.position.x);
    cc.center_y_units = toMillimeters(junction.position.y);
    cc.radius_units = junction.diameter.nanometers == 0 ? 0.4 : toMillimeters(junction.diameter) / 2.0;
    cc.width_units = 0.0;
    cc.fill_type = "solid";
    scene.circles.push_back(cc);
    includeSchematicBounds(cc.center_x_units - cc.radius_units, cc.center_y_units - cc.radius_units);
    includeSchematicBounds(cc.center_x_units + cc.radius_units, cc.center_y_units + cc.radius_units);
  }

  for (const SchNoConnect& nc : schematic.no_connects) {
    const double x = toMillimeters(nc.position.x);
    const double y = toMillimeters(nc.position.y);
    const double d = 0.75;
    scene.lines.push_back(CanvasLine{nc.id + "_1", "schematic_noconnect", x - d, y - d, x + d, y + d, 0.2});
    scene.lines.push_back(CanvasLine{nc.id + "_2", "schematic_noconnect", x - d, y + d, x + d, y - d, 0.2});
    includeSchematicBounds(x - d, y - d);
    includeSchematicBounds(x + d, y + d);
  }

  for (const SchText& text : schematic.texts) {
    CanvasText ct;
    ct.id = text.id;
    ct.layer_id = "schematic_text";
    ct.text = text.text;
    ct.x_units = toMillimeters(text.position.x);
    ct.y_units = toMillimeters(text.position.y);
    ct.rotation_degrees = text.rotation_degrees;
    ct.size_x_units = toMillimeters(text.size.width);
    ct.size_y_units = toMillimeters(text.size.height);
    scene.texts.push_back(ct);
    includeSchematicBounds(ct.x_units, ct.y_units);
  }

  for (const SchTextBox& textbox : schematic.textboxes) {
    CanvasText ct;
    ct.id = textbox.id;
    ct.layer_id = "schematic_text";
    ct.text = textbox.text;
    ct.x_units = toMillimeters(textbox.area.origin.x) + toMillimeters(textbox.area.size.width) / 2.0;
    ct.y_units = toMillimeters(textbox.area.origin.y) + toMillimeters(textbox.area.size.height) / 2.0;
    ct.rotation_degrees = 0.0;
    ct.size_x_units = toMillimeters(textbox.size.width);
    ct.size_y_units = toMillimeters(textbox.size.height);
    scene.texts.push_back(ct);

    CanvasPolygon cp;
    cp.id = textbox.id + "_box";
    cp.layer_id = "schematic_graphic";
    cp.width_units = 0.2;
    cp.fill_type = "none";
    const double left = toMillimeters(textbox.area.origin.x);
    const double right = left + toMillimeters(textbox.area.size.width);
    const double top = toMillimeters(textbox.area.origin.y);
    const double bottom = top + toMillimeters(textbox.area.size.height);
    cp.pts_x_units = {left, right, right, left};
    cp.pts_y_units = {top, top, bottom, bottom};
    scene.polygons.push_back(cp);

    includeSchematicBounds(left, top);
    includeSchematicBounds(right, bottom);
  }

  for (const SchGraphic& graphic : schematic.graphics) {
    if (graphic.kind == "line") {
      CanvasLine cl;
      cl.id = graphic.id;
      cl.layer_id = "schematic_graphic";
      cl.start_x_units = toMillimeters(graphic.start.x);
      cl.start_y_units = toMillimeters(graphic.start.y);
      cl.end_x_units = toMillimeters(graphic.end.x);
      cl.end_y_units = toMillimeters(graphic.end.y);
      cl.width_units = toMillimeters(graphic.width);
      scene.lines.push_back(cl);
      includeSchematicBounds(cl.start_x_units, cl.start_y_units);
      includeSchematicBounds(cl.end_x_units, cl.end_y_units);
    } else if (graphic.kind == "rectangle") {
      CanvasPolygon cp;
      cp.id = graphic.id;
      cp.layer_id = "schematic_graphic";
      cp.width_units = toMillimeters(graphic.width);
      cp.fill_type = "none";
      const double sx = toMillimeters(graphic.start.x);
      const double sy = toMillimeters(graphic.start.y);
      const double ex = toMillimeters(graphic.end.x);
      const double ey = toMillimeters(graphic.end.y);
      cp.pts_x_units = {sx, ex, ex, sx};
      cp.pts_y_units = {sy, sy, ey, ey};
      scene.polygons.push_back(cp);
      includeSchematicBounds(sx, sy);
      includeSchematicBounds(ex, ey);
    } else if (graphic.kind == "polygon") {
      CanvasPolygon cp;
      cp.id = graphic.id;
      cp.layer_id = "schematic_graphic";
      cp.width_units = toMillimeters(graphic.width);
      cp.fill_type = "none";
      for (const Point& pt : graphic.points) {
        const double x = toMillimeters(pt.x);
        const double y = toMillimeters(pt.y);
        cp.pts_x_units.push_back(x);
        cp.pts_y_units.push_back(y);
        includeSchematicBounds(x, y);
      }
      scene.polygons.push_back(cp);
    } else if (graphic.kind == "circle") {
      CanvasCircle cc;
      cc.id = graphic.id;
      cc.layer_id = "schematic_graphic";
      cc.center_x_units = toMillimeters(graphic.start.x);
      cc.center_y_units = toMillimeters(graphic.start.y);
      const double ex = toMillimeters(graphic.end.x);
      const double ey = toMillimeters(graphic.end.y);
      cc.radius_units = std::hypot(ex - cc.center_x_units, ey - cc.center_y_units);
      cc.width_units = toMillimeters(graphic.width);
      cc.fill_type = "none";
      scene.circles.push_back(cc);
      includeSchematicBounds(cc.center_x_units - cc.radius_units, cc.center_y_units - cc.radius_units);
      includeSchematicBounds(cc.center_x_units + cc.radius_units, cc.center_y_units + cc.radius_units);
    } else if (graphic.kind == "arc") {
      CanvasArc ca;
      ca.id = graphic.id;
      ca.layer_id = "schematic_graphic";
      ca.start_x_units = toMillimeters(graphic.start.x);
      ca.start_y_units = toMillimeters(graphic.start.y);
      ca.end_x_units = toMillimeters(graphic.end.x);
      ca.end_y_units = toMillimeters(graphic.end.y);
      if (!graphic.points.empty()) {
        ca.mid_x_units = toMillimeters(graphic.points.front().x);
        ca.mid_y_units = toMillimeters(graphic.points.front().y);
      }
      ca.width_units = toMillimeters(graphic.width);
      scene.arcs.push_back(ca);
      includeSchematicBounds(ca.start_x_units, ca.start_y_units);
      includeSchematicBounds(ca.end_x_units, ca.end_y_units);
    }
  }

  for (const SchSheet& sheet : schematic.sheets) {
    CanvasPolygon cp;
    cp.id = sheet.id + "_box";
    cp.layer_id = "schematic_graphic";
    cp.width_units = 0.4;
    cp.fill_type = "none";
    const double sx = toMillimeters(sheet.position.x);
    const double sy = toMillimeters(sheet.position.y);
    const double ex = sx + toMillimeters(sheet.size.width);
    const double ey = sy + toMillimeters(sheet.size.height);
    cp.pts_x_units = {sx, ex, ex, sx};
    cp.pts_y_units = {sy, sy, ey, ey};
    scene.polygons.push_back(cp);
    includeSchematicBounds(sx, sy);
    includeSchematicBounds(ex, ey);
    
    CanvasText ct;
    ct.id = sheet.id + "_name";
    ct.layer_id = "schematic_text";
    ct.text = sheet.name;
    ct.x_units = sx;
    ct.y_units = sy - 1.5;
    ct.size_x_units = 1.5;
    ct.size_y_units = 1.5;
    scene.texts.push_back(ct);

    for (const SchSheetPin& pin : sheet.pins) {
      CanvasLine cl;
      cl.id = pin.id + "_line";
      cl.layer_id = "schematic_pin";
      cl.start_x_units = toMillimeters(pin.position.x);
      cl.start_y_units = toMillimeters(pin.position.y);
      const double px = toMillimeters(pin.position.x);
      const double py = toMillimeters(pin.position.y);
      // Prefer the imported KiCad side; infer it for legacy data.
      double ox = px;
      double oy = py;
      if (pin.side == "left" || (pin.side.empty() && std::abs(px - sx) < 0.1)) ox += 2.0;
      else if (pin.side == "right" || (pin.side.empty() && std::abs(px - ex) < 0.1)) ox -= 2.0;
      else if (pin.side == "top" || (pin.side.empty() && std::abs(py - sy) < 0.1)) oy += 2.0;
      else if (pin.side == "bottom" || (pin.side.empty() && std::abs(py - ey) < 0.1)) oy -= 2.0;
      cl.end_x_units = ox;
      cl.end_y_units = oy;
      cl.width_units = 0.2;
      scene.lines.push_back(cl);

      CanvasText pct;
      pct.id = pin.id + "_text";
      pct.layer_id = "schematic_text";
      pct.text = pin.name;
      pct.x_units = ox;
      pct.y_units = oy;
      pct.size_x_units = 1.0;
      pct.size_y_units = 1.0;
      scene.texts.push_back(pct);
    }
  }

  for (const SchMarker& marker : schematic.markers) {
    CanvasText ct;
    ct.id = marker.id;
    ct.layer_id = "schematic_graphic";
    ct.text = "?";
    ct.x_units = toMillimeters(marker.position.x);
    ct.y_units = toMillimeters(marker.position.y);
    ct.size_x_units = 2.0;
    ct.size_y_units = 2.0;
    scene.texts.push_back(ct);
    includeSchematicBounds(ct.x_units, ct.y_units);
  }

  for (const SchBusEntry& entry : schematic.bus_entries) {
    CanvasLine cl;
    cl.id = entry.id;
    cl.layer_id = "schematic_wire";
    cl.start_x_units = toMillimeters(entry.position.x);
    cl.start_y_units = toMillimeters(entry.position.y);
    cl.end_x_units = cl.start_x_units + toMillimeters(entry.size.width);
    cl.end_y_units = cl.start_y_units + toMillimeters(entry.size.height);
    cl.width_units = entry.kind == "bus" ? 0.4 : 0.2;
    scene.lines.push_back(cl);
    includeSchematicBounds(cl.start_x_units, cl.start_y_units);
    includeSchematicBounds(cl.end_x_units, cl.end_y_units);
  }

  for (const SchRuleArea& area : schematic.rule_areas) {
    CanvasPolygon cp;
    cp.id = area.id;
    cp.layer_id = "schematic_graphic";
    cp.width_units = 0.2;
    cp.fill_type = "none";
    for (const Point& pt : area.outline) {
      const double x = toMillimeters(pt.x);
      const double y = toMillimeters(pt.y);
      cp.pts_x_units.push_back(x);
      cp.pts_y_units.push_back(y);
      includeSchematicBounds(x, y);
    }
    scene.polygons.push_back(cp);
  }

  for (const SchTable& table : schematic.tables) {
    CanvasTable ct;
    ct.id = table.id;
    ct.layer_id = "schematic_graphic";
    ct.x_units = toMillimeters(table.position.x);
    ct.y_units = toMillimeters(table.position.y);
    ct.width_units = toMillimeters(table.size.width);
    ct.height_units = toMillimeters(table.size.height);
    ct.rows = table.rows;
    ct.cols = table.cols;
    scene.tables.push_back(ct);
    includeSchematicBounds(ct.x_units, ct.y_units);
    includeSchematicBounds(ct.x_units + ct.width_units, ct.y_units + ct.height_units);
  }

  for (const SchBitmap& bitmap : schematic.bitmaps) {
    CanvasReferenceImage cri;
    cri.id = bitmap.id;
    cri.layer_id = "schematic_graphic";
    cri.x_units = toMillimeters(bitmap.position.x);
    cri.y_units = toMillimeters(bitmap.position.y);
    cri.scale = bitmap.scale;
    cri.data = bitmap.data;
    cri.opacity = 1.0;
    scene.reference_images.push_back(cri);
    includeSchematicBounds(cri.x_units, cri.y_units);
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
    double pin_rotation_degrees = 0.0;
    if (pin.orientation == PinOrientation::Up) pin_rotation_degrees = 90.0;
    else if (pin.orientation == PinOrientation::Left) pin_rotation_degrees = 180.0;
    else if (pin.orientation == PinOrientation::Down) pin_rotation_degrees = 270.0;
    
    const CanvasPoint pin_vector = rotatePoint(pin_length_units, 0.0, pin_rotation_degrees);
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


void includeBounds(double& min_x, double& min_y, double& max_x, double& max_y,
                   const double x_units, const double y_units) {
  if (x_units < min_x) min_x = x_units;
  if (y_units < min_y) min_y = y_units;
  if (x_units > max_x) max_x = x_units;
  if (y_units > max_y) max_y = y_units;
}

}  // namespace ccad

