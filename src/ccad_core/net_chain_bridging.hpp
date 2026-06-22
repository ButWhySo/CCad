#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <vector>

namespace ccad {

struct NetChainBridge {
  std::string component_id;
  std::string pad1_id;
  std::string pad2_id;
  int64_t bridge_length_nm = 0;
  bool is_valid = false;
};

struct NetChainBridgingReport {
  std::string kicad_source = "pcbnew/net_chain_bridging.cpp";
  std::string parity_scope = "net_chain_bridging_calculation_first_slice";
  std::string net_id;
  std::vector<NetChainBridge> bridges;
  std::vector<std::string> diagnostics;
  std::vector<std::string> pending_kicad_features;
};

NetChainBridgingReport calculateNetChainBridges(const Project& project, const std::string& net_id);

}  // namespace ccad
