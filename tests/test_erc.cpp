#include "ccad_core/erc.hpp"
#include "ccad_core/model.hpp"

#include <cassert>
#include <string>
#include <vector>

namespace {

ccad::Project validProject() {
  ccad::Project project;
  project.id = "proj-demo";
  project.name = "demo";
  project.components.push_back(ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "VDD", .kind = "power"},
               ccad::Pin{.name = "GND", .kind = "power"}},
  });
  project.nets.push_back(ccad::Net{
      .id = "N_3V3",
      .members = {ccad::NetMember{.component_id = "U1", .pin_name = "VDD"}},
  });
  return project;
}

bool hasCode(const std::vector<ccad::Diagnostic>& diagnostics, const std::string& code) {
  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.code == code) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  assert(ccad::runErc(validProject()).empty());

  ccad::Project unknown_component = validProject();
  unknown_component.nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U404", .pin_name = "VDD"});
  assert(hasCode(ccad::runErc(unknown_component), "UNKNOWN_COMPONENT"));

  ccad::Project unknown_pin = validProject();
  unknown_pin.nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U1", .pin_name = "NOPE"});
  assert(hasCode(ccad::runErc(unknown_pin), "UNKNOWN_PIN"));

  ccad::Project duplicate_member = validProject();
  duplicate_member.nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U1", .pin_name = "VDD"});
  assert(hasCode(ccad::runErc(duplicate_member), "DUPLICATE_NET_MEMBER"));

  ccad::Project empty;
  empty.id = "proj-empty";
  empty.name = "empty";
  const std::vector<ccad::Diagnostic> empty_diagnostics = ccad::runErc(empty);
  assert(hasCode(empty_diagnostics, "EMPTY_PROJECT"));
  assert(empty_diagnostics.at(0).severity == "warning");
}

