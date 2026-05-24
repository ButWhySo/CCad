#include "ccad_core/erc.hpp"

#include <set>
#include <string>

namespace ccad {
namespace {

bool componentHasPin(const Component& component, const std::string& pin_name) {
  for (const Pin& pin : component.pins) {
    if (pin.name == pin_name) {
      return true;
    }
  }
  return false;
}

const Component* findComponent(const Project& project, const std::string& component_id) {
  for (const Component& component : project.components) {
    if (component.id == component_id) {
      return &component;
    }
  }
  return nullptr;
}

Diagnostic makeDiagnostic(const std::string& severity, const std::string& code,
                          const std::string& message, const std::string& object_id) {
  return Diagnostic{
      .severity = severity,
      .code = code,
      .message = message,
      .object_id = object_id,
  };
}

}  // namespace

std::vector<Diagnostic> runErc(const Project& project) {
  std::vector<Diagnostic> diagnostics;

  if (project.components.empty() && project.nets.empty()) {
    diagnostics.push_back(makeDiagnostic("warning", "EMPTY_PROJECT",
                                         "Project has no components or nets", project.id));
  }

  std::set<std::string> component_ids;
  for (const Component& component : project.components) {
    if (component.id.empty()) {
      diagnostics.push_back(makeDiagnostic("error", "INVALID_COMPONENT_ID",
                                           "Component ID must not be empty", component.id));
    }
    if (component.part.empty()) {
      diagnostics.push_back(makeDiagnostic("error", "INVALID_COMPONENT_PART",
                                           "Component part must not be empty", component.id));
    }
    if (!component_ids.insert(component.id).second) {
      diagnostics.push_back(makeDiagnostic("error", "DUPLICATE_COMPONENT_ID",
                                           "Component ID appears more than once", component.id));
    }

    std::set<std::string> pin_names;
    for (const Pin& pin : component.pins) {
      if (pin.name.empty()) {
        diagnostics.push_back(makeDiagnostic("error", "INVALID_PIN_NAME",
                                             "Component pin name must not be empty",
                                             component.id));
      }
      if (pin.kind.empty()) {
        diagnostics.push_back(makeDiagnostic("error", "INVALID_PIN_KIND",
                                             "Component pin kind must not be empty",
                                             component.id + "." + pin.name));
      }
      if (!pin_names.insert(pin.name).second) {
        diagnostics.push_back(makeDiagnostic("error", "DUPLICATE_PIN",
                                             "Component pin appears more than once",
                                             component.id + "." + pin.name));
      }
    }
  }

  std::set<std::string> net_ids;
  for (const Net& net : project.nets) {
    if (net.id.empty()) {
      diagnostics.push_back(makeDiagnostic("error", "INVALID_NET_ID", "Net ID must not be empty",
                                           net.id));
    }
    if (!net_ids.insert(net.id).second) {
      diagnostics.push_back(makeDiagnostic("error", "DUPLICATE_NET_ID",
                                           "Net ID appears more than once", net.id));
    }
    std::set<std::string> members;
    for (const NetMember& member : net.members) {
      if (member.component_id.empty() || member.pin_name.empty()) {
        diagnostics.push_back(makeDiagnostic("error", "INVALID_NET_MEMBER",
                                             "Net member must include component_id and pin_name",
                                             net.id));
        continue;
      }
      const std::string member_id = member.component_id + "." + member.pin_name;
      if (!members.insert(member_id).second) {
        diagnostics.push_back(makeDiagnostic("error", "DUPLICATE_NET_MEMBER",
                                             "Net contains the same component pin more than once",
                                             net.id));
      }

      const Component* component = findComponent(project, member.component_id);
      if (component == nullptr) {
        diagnostics.push_back(makeDiagnostic("error", "UNKNOWN_COMPONENT",
                                             "Net references an unknown component",
                                             member.component_id));
        continue;
      }

      if (!componentHasPin(*component, member.pin_name)) {
        diagnostics.push_back(makeDiagnostic("error", "UNKNOWN_PIN",
                                             "Net references an unknown component pin",
                                             member_id));
      }
    }
  }

  return diagnostics;
}

}  // namespace ccad

