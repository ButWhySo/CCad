#include "ccad_core/bom_export.hpp"
#include <sstream>
#include <algorithm>

namespace ccad {

std::string exportToBomCsv(const Project& project) {
  std::stringstream ss;
  ss << "Designator,Part\n";

  const Schematic* schematic = primarySchematic(project);
  if (schematic == nullptr) {
    return ss.str();
  }

  // Create a copy of components to sort them by designator
  std::vector<Component> sorted_components = schematic->components;
  std::sort(sorted_components.begin(), sorted_components.end(),
            [](const Component& a, const Component& b) {
              return a.id < b.id;
            });

  for (const auto& comp : sorted_components) {
    // Basic CSV escaping logic if needed in the future, for now just print
    std::string id = comp.id;
    std::string part = comp.part;
    
    // Simple quotes for part if it contains comma
    if (part.find(',') != std::string::npos) {
        part = "\"" + part + "\"";
    }

    ss << id << "," << part << "\n";
  }

  return ss.str();
}

} // namespace ccad
