#include "ccad_core/net_chain_bridging.hpp"

#include <algorithm>
#include <cmath>

namespace ccad {

NetChainBridgingReport calculateNetChainBridges(const Project& project, const std::string& net_id) {
  NetChainBridgingReport report;
  report.net_id = net_id;
  report.pending_kicad_features = {
      "full component package length evaluation",
      "differential pair bridge calculation",
      "complex impedance modeling",
      "multi-pin package internal routing"
  };

  if (project.boards.empty()) {
    report.diagnostics.push_back("project has no board");
    return report;
  }

  const Board& board = project.boards.front();

  // Find all components that have at least one pad on the target net.
  // Then check if they have pads on other nets that might be considered "bridged".
  // We'll iterate over unique component_ids from pads.
  
  std::vector<std::string> comp_ids;
  for (const Pad& pad : board.pads) {
    if (std::find(comp_ids.begin(), comp_ids.end(), pad.component_id) == comp_ids.end()) {
      comp_ids.push_back(pad.component_id);
    }
  }

  for (const std::string& comp_id : comp_ids) {
    std::vector<const Pad*> comp_pads;
    for (const Pad& pad : board.pads) {
      if (pad.component_id == comp_id) {
        comp_pads.push_back(&pad);
      }
    }

    if (comp_pads.size() == 2) {
      const Pad* pad1 = comp_pads[0];
      const Pad* pad2 = comp_pads[1];
      
      if (pad1->net_id == net_id || pad2->net_id == net_id) {
        NetChainBridge bridge;
        bridge.component_id = comp_id;
        bridge.pad1_id = pad1->id;
        bridge.pad2_id = pad2->id;
        
        // Simple bridge length is center to center distance
        double dx = static_cast<double>(pad1->position.x.nanometers - pad2->position.x.nanometers);
        double dy = static_cast<double>(pad1->position.y.nanometers - pad2->position.y.nanometers);
        bridge.bridge_length_nm = static_cast<int64_t>(std::sqrt(dx * dx + dy * dy));
        bridge.is_valid = true;
        
        report.bridges.push_back(bridge);
      }
    }
  }

  return report;
}

}  // namespace ccad
