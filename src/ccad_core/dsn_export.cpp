#include "dsn_export.hpp"

#include <iomanip>
#include <cmath>
#include <cstdint>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace ccad {

namespace {

std::string formatMm(std::int64_t nanometers) {
  const double mm = static_cast<double>(nanometers) / 1000000.0;
  std::ostringstream out;
  out << std::fixed << std::setprecision(4) << mm;
  return out.str();
}

std::string quote(const std::string& str) {
  return "\"" + str + "\"";
}

}  // namespace

std::string exportSpecctraDsn(const Project& project) {
  if (!!project.boards.empty()) {
    throw std::runtime_error("Cannot export DSN: project has no board.");
  }
  const Board& board = project.boards[0];

  std::ostringstream out;
  out << "(pcb " << quote(project.name) << "\n"
      << "  (parser\n"
      << "    (string_quote \")\n"
      << "    (space_in_quoted_tokens on)\n"
      << "    (host_cad \"CCad\")\n"
      << "    (host_version \"1.0\")\n"
      << "  )\n"
      << "  (resolution mm 10000)\n"
      << "  (unit mm)\n"
      << "  (structure\n";

  // Layers
  for (const auto& layer : board.layers) {
    if (layer.kind == "copper" || layer.kind == "signal" || layer.kind == "power") {
      out << "    (layer " << quote(layer.id) << " (type signal))\n";
    }
  }

  // Boundary
  out << "    (boundary\n"
      << "      (path pcb 0 "
      << formatMm(board.outline.origin.x.nanometers) << " " << formatMm(board.outline.origin.y.nanometers) << " "
      << formatMm(board.outline.origin.x.nanometers + board.outline.size.width.nanometers) << " " << formatMm(board.outline.origin.y.nanometers) << " "
      << formatMm(board.outline.origin.x.nanometers + board.outline.size.width.nanometers) << " " << formatMm(board.outline.origin.y.nanometers + board.outline.size.height.nanometers) << " "
      << formatMm(board.outline.origin.x.nanometers) << " " << formatMm(board.outline.origin.y.nanometers + board.outline.size.height.nanometers) << " "
      << formatMm(board.outline.origin.x.nanometers) << " " << formatMm(board.outline.origin.y.nanometers) << ")\n"
      << "    )\n";

  // Rules
  out << "    (rule\n"
      << "      (clearance " << formatMm(board.design_rules.copper_clearance.nanometers) << ")\n"
      << "    )\n"
      << "  )\n";

  // Group pads by component
  std::map<std::string, std::vector<Pad>> component_pads;
  for (const auto& pad : board.pads) {
    if (!pad.component_id.empty()) {
      component_pads[pad.component_id].push_back(pad);
    }
  }

  // Map component ID to BoardFootprint
  std::map<std::string, const BoardFootprint*> comp_footprints;
  for (const auto& fp : board.footprints) {
    comp_footprints[fp.reference] = &fp;
  }

  // Placement
  out << "  (placement\n";
  for (const auto& pair : component_pads) {
    const std::string& comp_id = pair.first;
    auto it = comp_footprints.find(comp_id);
    if (it != comp_footprints.end()) {
      const BoardFootprint* fp = it->second;
      std::string layer = fp->layer_id == "B.Cu" ? "back" : "front";
      out << "    (component " << quote(comp_id) << "\n"
          << "      (place " << quote(comp_id) << " "
          << formatMm(fp->position.x.nanometers) << " "
          << formatMm(fp->position.y.nanometers) << " "
          << layer << " "
          << fp->rotation_degrees << ")\n"
          << "    )\n";
    } else {
      out << "    (component " << quote(comp_id) << "\n"
          << "      (place " << quote(comp_id) << " 0.0 0.0 front 0.0)\n"
          << "    )\n";
    }
  }
  out << "  )\n";

  // Library
  out << "  (library\n";
  for (const auto& pair : component_pads) {
    const std::string& comp_id = pair.first;
    const std::vector<Pad>& pads = pair.second;
    out << "    (image " << quote(comp_id) << "\n";
    
    auto it = comp_footprints.find(comp_id);
    double cx = 0, cy = 0, angle = 0;
    if (it != comp_footprints.end()) {
      cx = static_cast<double>(it->second->position.x.nanometers);
      cy = static_cast<double>(it->second->position.y.nanometers);
      angle = it->second->rotation_degrees * 3.14159265358979323846 / 180.0;
    }

    for (const auto& pad : pads) {
      double dx = pad.position.x.nanometers - cx;
      double dy = pad.position.y.nanometers - cy;
      
      // Inverse rotate pad pos to be relative to component
      double local_x = dx * cos(angle) + dy * sin(angle);
      double local_y = -dx * sin(angle) + dy * cos(angle);
      
      out << "      (pin " << quote("padstack_" + pad.id) << " " << pad.pin_name << " "
          << formatMm(static_cast<std::int64_t>(local_x)) << " " << formatMm(static_cast<std::int64_t>(local_y)) << ")\n";
    }
    out << "    )\n";
  }
  for (const auto& pad : board.pads) {
    std::string first_layer = pad.padstack.layer_set.empty() ? "" : pad.padstack.layer_set.front();
    Size psize = {ccad::nanometers(0), ccad::nanometers(0)};
    if (!pad.padstack.copper_props.empty()) {
      psize = pad.padstack.copper_props.begin()->second.shape.size;
    }
    out << "    (padstack " << quote("padstack_" + pad.id) << "\n"
        << "      (shape (rect " << quote(first_layer) << " "
        << formatMm(-psize.width.nanometers / 2) << " " << formatMm(-psize.height.nanometers / 2) << " "
        << formatMm(psize.width.nanometers / 2) << " " << formatMm(psize.height.nanometers / 2) << "))\n"
        << "    )\n";
  }
  for (const auto& via : board.vias) {
    out << "    (padstack " << quote("viastack_" + via.id) << "\n"
        << "      (shape (circle " << quote("F.Cu") << " " << formatMm(via.diameter.nanometers / 2) << "))\n"
        << "      (shape (circle " << quote("B.Cu") << " " << formatMm(via.diameter.nanometers / 2) << "))\n"
        << "      (attach off)\n"
        << "    )\n";
  }
  out << "  )\n";

  // Network
  std::map<std::string, std::vector<std::string>> net_pins;
  for (const auto& pad : board.pads) {
    if (!pad.net_id.empty() && !pad.component_id.empty()) {
      net_pins[pad.net_id].push_back(quote(pad.component_id) + "-" + pad.pin_name);
    }
  }
  out << "  (network\n";
  for (const auto& pair : net_pins) {
    const std::string& net_id = pair.first;
    const std::vector<std::string>& pins = pair.second;
    out << "    (net " << quote(net_id) << " (pins";
    for (const auto& pin : pins) {
      out << " " << pin;
    }
    out << "))\n";
  }
  out << "  )\n";

  // Wiring
  out << "  (wiring\n";
  for (const auto& track : board.tracks) {
    out << "    (wire (path " << quote(track.layer_id) << " " << formatMm(track.width.nanometers) << " "
        << formatMm(track.start.x.nanometers) << " " << formatMm(track.start.y.nanometers) << " "
        << formatMm(track.end.x.nanometers) << " " << formatMm(track.end.y.nanometers) << ") (net " << quote(track.net_id) << "))\n";
  }
  for (const auto& via : board.vias) {
    out << "    (via " << quote("viastack_" + via.id) << " " << formatMm(via.position.x.nanometers) << " " << formatMm(via.position.y.nanometers) << " (net " << quote(via.net_id) << "))\n";
  }
  out << "  )\n";

  out << ")\n";

  return out.str();
}

}  // namespace ccad
