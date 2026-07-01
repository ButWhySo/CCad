#include "ccad_core/pnp_export.hpp"
#include <sstream>
#include <map>
#include <vector>
#include <algorithm>
#include <limits>
#include <iomanip>

namespace ccad {

std::string exportToPnpCsv(const Project& project) {
  std::stringstream ss;
  ss << "Designator,Value,PosX,PosY,Side\n";
  ss << std::fixed << std::setprecision(4);

  const Board* board = primaryBoard(project);
  if (board == nullptr) {
      return ss.str();
  }

  // Group pads by component
  std::map<std::string, std::vector<const Pad*>> comp_pads;
  for (const Pad& pad : board->pads) {
      if (!pad.component_id.empty()) {
          comp_pads[pad.component_id].push_back(&pad);
      }
  }

  // Create a map of component ID to part string
  std::map<std::string, std::string> comp_values;
  if (const Schematic* schematic = primarySchematic(project)) {
    for (const auto& comp : schematic->symbols) {
      comp_values[comp.id] = comp.lib_id;
    }
  }

  // Get sorted list of component IDs
  std::vector<std::string> comp_ids;
  for (const auto& [id, pads] : comp_pads) {
      comp_ids.push_back(id);
  }
  std::sort(comp_ids.begin(), comp_ids.end());

  for (const std::string& id : comp_ids) {
      const auto& pads = comp_pads[id];
      if (pads.empty()) continue;

      double min_x = std::numeric_limits<double>::max();
      double min_y = std::numeric_limits<double>::max();
      double max_x = std::numeric_limits<double>::lowest();
      double max_y = std::numeric_limits<double>::lowest();

      std::string side = "Top";

      for (const Pad* pad : pads) {
          double px = pad->position.x.nanometers / 1000000.0;
          double py = pad->position.y.nanometers / 1000000.0;
          if (px < min_x) min_x = px;
          if (px > max_x) max_x = px;
          if (py < min_y) min_y = py;
          if (py > max_y) max_y = py;

          bool on_bottom = false;
          for (const auto& l : pad->padstack.layer_set) {
            if (l.starts_with("B.")) on_bottom = true;
          }
          if (on_bottom) {
              side = "Bottom";
          }
      }

      double cx = (min_x + max_x) / 2.0;
      double cy = (min_y + max_y) / 2.0;
      std::string val = comp_values.count(id) ? comp_values[id] : "";

      ss << id << "," << val << "," << cx << "," << cy << "," << side << "\n";
  }

  return ss.str();
}

} // namespace ccad
