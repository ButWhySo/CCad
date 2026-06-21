#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace ccad {

struct CrossProbeTarget {
  std::string type;
  std::string id;
  std::string component_id;
  std::string pin_name;
  std::string net_id;
  std::string source;
  int selection_index = -1;
  bool focus = false;
};

struct CrossProbeReport {
  std::string kicad_source = "pcbnew/cross-probing.cpp";
  std::string kicad_class = "PCB_EDIT_FRAME";
  std::string kicad_function = "ExecuteRemoteCommand";
  std::string parity_scope = "cross_probe_packet_resolution_first_slice";
  std::string packet;
  std::string packet_kind = "unknown";
  bool clear_highlight = false;
  bool select_connections = false;
  std::string part_reference;
  std::string pad_number;
  std::vector<std::string> requested_nets;
  std::vector<CrossProbeTarget> targets;
  std::vector<std::string> diagnostics;
  std::vector<std::string> pending_kicad_features;
};

std::string formatCrossProbeClear();
std::string formatCrossProbeNet(std::string_view net_name);
std::string formatCrossProbePart(std::string_view reference);
std::string formatCrossProbePad(std::string_view reference, std::string_view pad_number);

CrossProbeReport resolveCrossProbePacket(const Project& project, std::string_view packet);

}  // namespace ccad
