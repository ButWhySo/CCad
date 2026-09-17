#include "ccad_core/kicad_pcb_export.hpp"

#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad {
namespace {

std::string escapeKiCadString(const std::string& value) {
  std::string escaped;
  escaped.reserve(value.size());
  for (const char ch : value) {
    if (ch == '\\' || ch == '"') {
      escaped.push_back('\\');
    }
    escaped.push_back(ch);
  }
  return escaped;
}

}  // namespace

std::string exportToKiCadPcb(const Project& project) {
  if (!!project.boards.empty()) {
    throw std::runtime_error("project has no board layout");
  }
  const Board& board = project.boards[0];

  std::ostringstream out;
  out << std::fixed << std::setprecision(6);

  out << "(kicad_pcb (version 20211014) (generator ccad)\n";
  out << "  (general\n";
  out << "    (thickness 1.6)\n";
  out << "  )\n";

  // standard layers
  out << "  (layers\n";
  out << "    (0 \"F.Cu\" signal)\n";
  for (int i = 1; i <= 30; ++i) {
    out << "    (" << i << " \"In" << i << ".Cu\" signal)\n";
  }
  out << "    (31 \"B.Cu\" signal)\n";
  out << "    (32 \"B.Adhes\" user \"B.Adhesive\")\n";
  out << "    (33 \"F.Adhes\" user \"F.Adhesive\")\n";
  out << "    (34 \"B.Paste\" user \"B.Paste\")\n";
  out << "    (35 \"F.Paste\" user \"F.Paste\")\n";
  out << "    (36 \"B.SilkS\" user \"B.Silkscreen\")\n";
  out << "    (37 \"F.SilkS\" user \"F.Silkscreen\")\n";
  out << "    (38 \"B.Mask\" user \"B.Mask\")\n";
  out << "    (39 \"F.Mask\" user \"F.Mask\")\n";
  out << "    (40 \"Dwgs.User\" user \"User.Drawings\")\n";
  out << "    (41 \"Cmts.User\" user \"User.Comments\")\n";
  out << "    (42 \"Eco1.User\" user \"User.Eco1\")\n";
  out << "    (43 \"Eco2.User\" user \"User.Eco2\")\n";
  out << "    (44 \"Edge.Cuts\" user)\n";
  out << "    (45 \"Margin\" user)\n";
  out << "    (46 \"B.CrtYd\" user \"B.Courtyard\")\n";
  out << "    (47 \"F.CrtYd\" user \"F.Courtyard\")\n";
  out << "    (48 \"B.Fab\" user \"B.Fab\")\n";
  out << "    (49 \"F.Fab\" user \"F.Fab\")\n";
  out << "    (50 \"User.1\" user)\n";
  out << "    (51 \"User.2\" user)\n";
  out << "    (52 \"User.3\" user)\n";
  out << "    (53 \"User.4\" user)\n";
  out << "    (54 \"User.5\" user)\n";
  out << "    (55 \"User.6\" user)\n";
  out << "    (56 \"User.7\" user)\n";
  out << "    (57 \"User.8\" user)\n";
  out << "    (58 \"User.9\" user)\n";
  out << "  )\n";

  // setup
  double clearance_mm = board.design_rules.copper_clearance.nanometers / 1000000.0;
  double min_track_mm = board.design_rules.min_track_width.nanometers / 1000000.0;
  double min_ring_mm = board.design_rules.min_via_annular_ring.nanometers / 1000000.0;
  double via_drill_mm = 0.3;
  double via_dia_mm = via_drill_mm + 2.0 * min_ring_mm;

  out << "  (setup\n";
  out << "    (stackup\n";
  out << "      (layer \"F.Cu\" (type \"copper\") (thickness 0.035))\n";
  out << "      (layer \"Dielectric\" (type \"core\") (thickness 1.51) (material \"FR4\"))\n";
  out << "      (layer \"B.Cu\" (type \"copper\") (thickness 0.035))\n";
  out << "    )\n";
  out << "  )\n";

  // net classes
  out << "  (netclass \"Default\" \"Default net class\"\n";
  out << "    (clearance " << clearance_mm << ")\n";
  out << "    (trace_width " << min_track_mm << ")\n";
  out << "    (via_dia " << via_dia_mm << ")\n";
  out << "    (via_drill " << via_drill_mm << ")\n";
  out << "  )\n";

  // nets
  std::set<std::string> net_ids;
  if (const Schematic* schematic = primarySchematic(project)) {
    for (const auto& net : schematic->nets) {
      if (!net.id.empty()) {
        net_ids.insert(net.id);
      }
    }
  }
  for (const Pad& pad : board.pads) {
    if (!pad.net_id.empty()) {
      net_ids.insert(pad.net_id);
    }
  }
  for (const Via& via : board.vias) {
    if (!via.net_id.empty()) {
      net_ids.insert(via.net_id);
    }
  }
  for (const TrackSegment& track : board.tracks) {
    if (!track.net_id.empty()) {
      net_ids.insert(track.net_id);
    }
  }
  for (const BoardZone& zone : board.zones) {
    if (!zone.net_id.empty()) {
      net_ids.insert(zone.net_id);
    }
  }
  std::map<std::string, int> net_to_index;
  out << "  (net 0 \"\")\n";
  int net_idx = 1;
  for (const auto& net_id : net_ids) {
    net_to_index[net_id] = net_idx;
    out << "  (net " << net_idx << " \"" << escapeKiCadString(net_id) << "\")\n";
    net_idx++;
  }

  // board outline
  double ox = board.outline.origin.x.nanometers / 1000000.0;
  double oy = board.outline.origin.y.nanometers / 1000000.0;
  double ow = board.outline.size.width.nanometers / 1000000.0;
  double oh = board.outline.size.height.nanometers / 1000000.0;
  out << "  (gr_rect (start " << ox << " " << oy << ") (end "
      << (ox + ow) << " " << (oy + oh)
      << ") (stroke (width 0.1) (type solid)) (layer \"Edge.Cuts\"))\n";

  // footprints (pads grouped by component)
  std::map<std::string, std::vector<const Pad*>> component_pads;
  for (const Pad& pad : board.pads) {
    component_pads[pad.component_id].push_back(&pad);
  }

  for (const auto& [comp_id, pads] : component_pads) {
    std::string ref = comp_id;
    if (ref.empty()) {
      ref = "FreePads";
    }
    std::string part = "SchSymbol";
    if (const Schematic* schematic = primarySchematic(project)) {
      for (const auto& comp : schematic->symbols) {
        if (comp.id == comp_id) {
          part = comp.lib_id;
          break;
        }
      }
    }

    out << "  (footprint \"" << part << "\" (at 0.0 0.0)\n";
    out << "    (property \"Reference\" \"" << ref << "\" (at 0.0 0.0) (layer \"F.SilkS\")\n";
    out << "      (effects (font (size 1 1) (thickness 0.15)))\n";
    out << "    )\n";

    for (const Pad* pad : pads) {
      double px = pad->position.x.nanometers / 1000000.0;
      double py = pad->position.y.nanometers / 1000000.0;
      const Size psize = pad->padstack.copper_props.empty() ? ccad::Size{} : pad->padstack.copper_props.begin()->second.shape.size;
      double pw = psize.width.nanometers / 1000000.0;
      double ph = psize.height.nanometers / 1000000.0;

      std::string pshape = "circle";
      if (!pad->padstack.copper_props.empty()) {
        const auto shape_enum = pad->padstack.copper_props.begin()->second.shape.shape;
        if (shape_enum == ccad::PadShape::Rectangle) pshape = "rect";
        else if (shape_enum == ccad::PadShape::Oval) pshape = "oval";
        else if (shape_enum == ccad::PadShape::Trapezoid) pshape = "trapezoid";
        else if (shape_enum == ccad::PadShape::RoundRect) pshape = "roundrect";
        else if (shape_enum == ccad::PadShape::ChamferedRect) pshape = "chamfered_rect";
        else if (shape_enum == ccad::PadShape::Custom) pshape = "custom";
      }

      out << "    (pad \"" << pad->pin_name << "\" " << pad->type << " " << pshape << " (at "
          << px << " " << py;
      if (pad->rotation_degrees != 0.0) {
        out << " " << pad->rotation_degrees;
      }
      out << ") (size " << pw << " " << ph << ")";
      if (pad->padstack.drill.size.width.nanometers > 0) {
        out << " (drill " << (pad->padstack.drill.size.width.nanometers / 1000000.0) << ")";
      }
      if (!pad->padstack.copper_props.empty() && pad->padstack.copper_props.begin()->second.shape.roundrect_rratio > 0.0) {
        out << " (roundrect_rratio " << pad->padstack.copper_props.begin()->second.shape.roundrect_rratio << ")";
      }
      if (!pad->padstack.copper_props.empty() && pad->padstack.copper_props.begin()->second.shape.chamfer_ratio > 0.0) {
        out << " (chamfer_ratio " << pad->padstack.copper_props.begin()->second.shape.chamfer_ratio << ")";
      }
      if (pad->padstack.secondary_drill.has_value()) {
        out << " (property \"secondary_drill\" \"" << (pad->padstack.secondary_drill->size.width.nanometers / 1000000.0) << "\")";
      }
      if (pad->padstack.tertiary_drill.has_value()) {
        out << " (property \"tertiary_drill\" \"" << (pad->padstack.tertiary_drill->size.width.nanometers / 1000000.0) << "\")";
      }
      if (pad->padstack.front_post_machining.mode == "backdrill" || pad->padstack.back_post_machining.mode == "backdrill") {
        out << " (property \"backdrilled\" \"true\")";
      }
      if (pad->padstack.front_post_machining.mode.has_value()) {
        out << " (property \"front_post_machining\" \"" << (pad->padstack.front_post_machining.size.nanometers / 1000000.0) << "\")";
      }
      if (pad->padstack.back_post_machining.mode.has_value()) {
        out << " (property \"back_post_machining\" \"" << (pad->padstack.back_post_machining.size.nanometers / 1000000.0) << "\")";
      }
      if (!pad->pin_type.empty()) {
        out << " (property \"pin_type\" \"" << escapeKiCadString(pad->pin_type) << "\")";
      }
      if (pad->pad_to_die_length.has_value()) {
        out << " (property \"pad_to_die_length\" \"" << (pad->pad_to_die_length->nanometers / 1000000.0) << "\")";
      }
      if (pad->pad_to_die_delay.has_value()) {
        out << " (property \"pad_to_die_delay\" \"" << *pad->pad_to_die_delay << "\")";
      }

      out << " (layers";
      for (const std::string& layer : pad->padstack.layer_set) {
        out << " \"" << layer << "\"";
      }
      out << ")";

      if (!pad->net_id.empty() && net_to_index.contains(pad->net_id)) {
        out << " (net " << net_to_index.at(pad->net_id) << " \"" << pad->net_id << "\")";
      }
      out << ")\n";
    }
    out << "  )\n";
  }

  // vias
  for (const Via& via : board.vias) {
    double vx = via.position.x.nanometers / 1000000.0;
    double vy = via.position.y.nanometers / 1000000.0;
    double vdia = via.diameter.nanometers / 1000000.0;
    double vdrill = via.drill.nanometers / 1000000.0;

    out << "  (via (at " << vx << " " << vy << ") (size " << vdia << ") (drill " << vdrill
        << ") (layers \"F.Cu\" \"B.Cu\")";
    if (!via.net_id.empty() && net_to_index.contains(via.net_id)) {
      out << " (net " << net_to_index.at(via.net_id) << ")";
    } else {
      out << " (net 0)";
    }
    out << ")\n";
  }

  // tracks
  for (const TrackSegment& track : board.tracks) {
    double sx = track.start.x.nanometers / 1000000.0;
    double sy = track.start.y.nanometers / 1000000.0;
    double ex = track.end.x.nanometers / 1000000.0;
    double ey = track.end.y.nanometers / 1000000.0;
    double tw = track.width.nanometers / 1000000.0;
    std::string layer = track.layer_id.empty() ? "F.Cu" : track.layer_id;

    out << "  (segment (start " << sx << " " << sy << ") (end " << ex << " " << ey
        << ") (width " << tw << ") (layer \"" << layer << "\")";
    if (!track.net_id.empty() && net_to_index.contains(track.net_id)) {
      out << " (net " << net_to_index.at(track.net_id) << ")";
    } else {
      out << " (net 0)";
    }
    out << ")\n";
  }

  // board graphics
  for (const BoardGraphic& graphic : board.graphics) {
    if (graphic.kind != "line") {
      continue;
    }
    const double sx = graphic.start.x.nanometers / 1000000.0;
    const double sy = graphic.start.y.nanometers / 1000000.0;
    const double ex = graphic.end.x.nanometers / 1000000.0;
    const double ey = graphic.end.y.nanometers / 1000000.0;
    const double width = graphic.width.nanometers / 1000000.0;
    const std::string layer = graphic.layer_id.empty() ? "Dwgs.User" : graphic.layer_id;
    out << "  (gr_line (start " << sx << " " << sy << ") (end " << ex << " " << ey
        << ") (stroke (width " << width << ") (type solid)) (layer \"" << layer << "\"))\n";
  }

  // board text
  for (const BoardText& text : board.texts) {
    const double x = text.position.x.nanometers / 1000000.0;
    const double y = text.position.y.nanometers / 1000000.0;
    const double width = text.size.width.nanometers / 1000000.0;
    const double height = text.size.height.nanometers / 1000000.0;
    const std::string layer = text.layer_id.empty() ? "F.SilkS" : text.layer_id;
    out << "  (gr_text \"" << escapeKiCadString(text.text) << "\" (at " << x << " " << y
        << " " << text.rotation_degrees << ") (layer \"" << layer << "\")\n"
        << "    (effects (font (size " << width << " " << height
        << ") (thickness 0.150000)))\n"
        << "  )\n";
  }

  // copper zones
  for (const BoardZone& zone : board.zones) {
    const int net_number =
        !zone.net_id.empty() && net_to_index.contains(zone.net_id) ? net_to_index.at(zone.net_id)
                                                                   : 0;
    const std::string net_name = net_number > 0 ? zone.net_id : "";
    const std::string primary_layer = zone.layer_ids.empty() ? "F.Cu" : zone.layer_ids.front();
    out << "  (zone (net " << net_number << ") (net_name \"" << escapeKiCadString(net_name)
        << "\") ";
    if (zone.layer_ids.size() > 1) {
      out << "(layers";
      for (const std::string& layer_id : zone.layer_ids) {
        out << " \"" << escapeKiCadString(layer_id) << "\"";
      }
      out << ")";
    } else {
      out << "(layer \"" << escapeKiCadString(primary_layer) << "\")";
    }
    out << "\n";
    if (!zone.name.empty()) {
      out << "    (name \"" << escapeKiCadString(zone.name) << "\")\n";
    }
    out << "    (hatch none 0.508000)\n";
    if (zone.priority > 0) {
      out << "    (priority " << zone.priority << ")\n";
    }
    out << "    (connect_pads";
    if (zone.pad_connection == "solid") {
      out << " yes";
    } else if (zone.pad_connection == "pth_thermal") {
      out << " thru_hole_only";
    } else if (zone.pad_connection == "none") {
      out << " no";
    }
    out << " (clearance " << (zone.clearance.nanometers / 1000000.0) << "))\n";
    out << "    (min_thickness " << (zone.min_thickness.nanometers / 1000000.0) << ")\n";
    out << "    (fill" << (zone.fill_enabled ? " yes" : "")
        << " (thermal_gap 0.500000) (thermal_bridge_width 0.500000))\n";
    out << "    (polygon (pts";
    for (const Point& point : zone.outline) {
      out << " (xy " << (point.x.nanometers / 1000000.0) << " "
          << (point.y.nanometers / 1000000.0) << ")";
    }
    out << "))\n";
    if (zone.fill_enabled) {
      const std::vector<std::string> export_layers =
          zone.layer_ids.empty() ? std::vector<std::string>{primary_layer} : zone.layer_ids;
      for (const std::string& layer_id : export_layers) {
        out << "    (filled_polygon (layer \"" << escapeKiCadString(layer_id) << "\") (pts";
        const std::vector<Point>& fill_contour = zone.filled_contours.empty()
                                                     ? zone.outline
                                                     : zone.filled_contours.front();
        for (const Point& point : fill_contour) {
          out << " (xy " << (point.x.nanometers / 1000000.0) << " "
              << (point.y.nanometers / 1000000.0) << ")";
        }
        out << "))\n";
        for (const BoardZone::FilledThermalSpoke& spoke : zone.filled_thermal_spokes) {
          const double x1 = spoke.start.x.nanometers / 1000000.0;
          const double y1 = spoke.start.y.nanometers / 1000000.0;
          const double x2 = spoke.end.x.nanometers / 1000000.0;
          const double y2 = spoke.end.y.nanometers / 1000000.0;
          const double half_width = spoke.width.nanometers / 2000000.0;
          if (x1 != x2 && y1 != y2) {
            continue;
          }
          const double min_x = std::min(x1, x2);
          const double max_x = std::max(x1, x2);
          const double min_y = std::min(y1, y2);
          const double max_y = std::max(y1, y2);
          out << "    (filled_polygon (layer \"" << escapeKiCadString(layer_id) << "\") (pts";
          if (y1 == y2) {
            out << " (xy " << min_x << " " << (y1 - half_width) << ")"
                << " (xy " << max_x << " " << (y1 - half_width) << ")"
                << " (xy " << max_x << " " << (y1 + half_width) << ")"
                << " (xy " << min_x << " " << (y1 + half_width) << ")";
          } else {
            out << " (xy " << (x1 - half_width) << " " << min_y << ")"
                << " (xy " << (x1 + half_width) << " " << min_y << ")"
                << " (xy " << (x1 + half_width) << " " << max_y << ")"
                << " (xy " << (x1 - half_width) << " " << max_y << ")";
          }
          out << "))\n";
        }
      }
    }
    out << "  )\n";
  }

  // keepouts
  for (const Keepout& keepout : board.keepouts) {
    double kx = keepout.area.origin.x.nanometers / 1000000.0;
    double ky = keepout.area.origin.y.nanometers / 1000000.0;
    double kw = keepout.area.size.width.nanometers / 1000000.0;
    double kh = keepout.area.size.height.nanometers / 1000000.0;

    out << "  (zone (net 0) (net_name \"\") (layer \"F.Cu\")\n";
    out << "    (keepout (tracks not_allowed) (vias not_allowed) (pads not_allowed) (copperareas not_allowed))\n";
    out << "    (polygon\n";
    out << "      (pts\n";
    out << "        (xy " << kx << " " << ky << ")\n";
    out << "        (xy " << (kx + kw) << " " << ky << ")\n";
    out << "        (xy " << (kx + kw) << " " << (ky + kh) << ")\n";
    out << "        (xy " << kx << " " << (ky + kh) << ")\n";
    out << "      )\n";
    out << "    )\n";
    out << "  )\n";
  }

  // placement regions
  for (const PlacementRegion& region : board.placement_regions) {
    double rx = region.area.origin.x.nanometers / 1000000.0;
    double ry = region.area.origin.y.nanometers / 1000000.0;
    double rw = region.area.size.width.nanometers / 1000000.0;
    double rh = region.area.size.height.nanometers / 1000000.0;

    out << "  (gr_rect (start " << rx << " " << ry << ") (end "
        << (rx + rw) << " " << (ry + rh)
        << ") (stroke (width 0.1) (type solid)) (layer \"Dwgs.User\"))\n";
  }

  out << ")\n";
  return out.str();
}

}  // namespace ccad
