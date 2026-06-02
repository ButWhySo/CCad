#include "dsn_export.hpp"

#include <iomanip>
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
  if (!project.board.has_value()) {
    throw std::runtime_error("Cannot export DSN: project has no board.");
  }
  const Board& board = *project.board;

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

  // Placement
  out << "  (placement\n";
  for (const auto& pair : component_pads) {
    const std::string& comp_id = pair.first;
    out << "    (component " << quote(comp_id) << "\n"
        << "      (place " << quote(comp_id) << " 0.0 0.0 front 0.0)\n"
        << "    )\n";
  }
  out << "  )\n";

  // Library
  out << "  (library\n";
  for (const auto& pair : component_pads) {
    const std::string& comp_id = pair.first;
    const std::vector<Pad>& pads = pair.second;
    out << "    (image " << quote(comp_id) << "\n";
    for (const auto& pad : pads) {
      out << "      (pin " << quote("padstack_" + pad.id) << " " << pad.pin_name << " "
          << formatMm(pad.position.x.nanometers) << " " << formatMm(pad.position.y.nanometers) << ")\n";
    }
    out << "    )\n";
  }
  for (const auto& pad : board.pads) {
    out << "    (padstack " << quote("padstack_" + pad.id) << "\n"
        << "      (shape (rect " << quote(pad.layers.empty() ? "" : pad.layers.front()) << " "
        << formatMm(-pad.size.width.nanometers / 2) << " " << formatMm(-pad.size.height.nanometers / 2) << " "
        << formatMm(pad.size.width.nanometers / 2) << " " << formatMm(pad.size.height.nanometers / 2) << "))\n"
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
