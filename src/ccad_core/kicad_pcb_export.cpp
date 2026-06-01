#include "ccad_core/kicad_pcb_export.hpp"

#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad {

std::string exportToKiCadPcb(const Project& project) {
  if (!project.board.has_value()) {
    throw std::runtime_error("project has no board layout");
  }
  const Board& board = *project.board;

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
  out << "    (44 \"B.CrtYd\" user \"B.Courtyard\")\n";
  out << "    (45 \"F.CrtYd\" user \"F.Courtyard\")\n";
  out << "    (46 \"B.Fab\" user \"B.Fab\")\n";
  out << "    (47 \"F.Fab\" user \"F.Fab\")\n";
  out << "    (48 \"User.1\" user \"User.1\")\n";
  out << "    (49 \"User.2\" user \"User.2\")\n";
  out << "    (50 \"User.3\" user \"User.3\")\n";
  out << "    (51 \"User.4\" user \"User.4\")\n";
  out << "    (52 \"User.5\" user \"User.5\")\n";
  out << "    (53 \"User.6\" user \"User.6\")\n";
  out << "    (54 \"User.7\" user \"User.7\")\n";
  out << "    (55 \"User.8\" user \"User.8\")\n";
  out << "    (56 \"User.9\" user \"User.9\")\n";
  out << "    (58 \"Edge.Cuts\" user)\n";
  out << "    (59 \"Margin\" user)\n";
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
  std::map<std::string, int> net_to_index;
  out << "  (net 0 \"\")\n";
  int net_idx = 1;
  for (const auto& net : project.nets) {
    net_to_index[net.id] = net_idx;
    out << "  (net " << net_idx << " \"" << net.id << "\")\n";
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
    std::string part = "Component";
    for (const auto& comp : project.components) {
      if (comp.id == comp_id) {
        part = comp.part;
        break;
      }
    }

    out << "  (footprint \"" << part << "\" (at 0.0 0.0)\n";
    out << "    (property \"Reference\" \"" << ref << "\" (at 0.0 0.0) (layer \"F.SilkS\")\n";
    out << "      (effects (font (size 1 1) (thickness 0.15)))\n";
    out << "    )\n";

    for (const Pad* pad : pads) {
      double px = pad->position.x.nanometers / 1000000.0;
      double py = pad->position.y.nanometers / 1000000.0;
      double pw = pad->size.width.nanometers / 1000000.0;
      double ph = pad->size.height.nanometers / 1000000.0;

      out << "    (pad \"" << pad->pin_name << "\" " << pad->type << " " << pad->shape << " (at "
          << px << " " << py;
      if (pad->rotation_degrees != 0.0) {
        out << " " << pad->rotation_degrees;
      }
      out << ") (size " << pw << " " << ph << ")";
      if (pad->drill.has_value()) {
        out << " (drill " << (pad->drill->nanometers / 1000000.0) << ")";
      }
      if (pad->roundrect_rratio.has_value()) {
        out << " (roundrect_rratio " << *pad->roundrect_rratio << ")";
      }
      if (pad->chamfer_ratio.has_value()) {
        out << " (chamfer_ratio " << *pad->chamfer_ratio << ")";
      }

      out << " (layers";
      for (const std::string& layer : pad->layers) {
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
