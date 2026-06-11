#include "ccad_core/erc.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

#include <string>
#include <vector>

namespace {

ccad::Project validProject() {
  ccad::Project project;
  project.id = "proj-demo";
  project.name = "demo";
  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].components.push_back(ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "VDD", .kind = "power"},
               ccad::Pin{.name = "GND", .kind = "power"}},
  });
  project.schematics[0].nets.push_back(ccad::Net{
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
  require(ccad::runErc(validProject()).empty(), "valid project has no diagnostics");

  ccad::Project no_schematic;
  no_schematic.id = "proj-board-only";
  no_schematic.name = "board only";
  require(ccad::runErc(no_schematic).empty(),
          "erc skips projects that do not contain a schematic document");

  ccad::Project empty_component_id = validProject();
  empty_component_id.schematics[0].components.at(0).id.clear();
  require(hasCode(ccad::runErc(empty_component_id), "INVALID_COMPONENT_ID"),
          "empty component id reported");

  ccad::Project empty_component_part = validProject();
  empty_component_part.schematics[0].components.at(0).part.clear();
  require(hasCode(ccad::runErc(empty_component_part), "INVALID_COMPONENT_PART"),
          "empty component part reported");

  ccad::Project empty_pin_name = validProject();
  empty_pin_name.schematics[0].components.at(0).pins.at(0).name.clear();
  require(hasCode(ccad::runErc(empty_pin_name), "INVALID_PIN_NAME"),
          "empty pin name reported");

  ccad::Project empty_pin_kind = validProject();
  empty_pin_kind.schematics[0].components.at(0).pins.at(0).kind.clear();
  require(hasCode(ccad::runErc(empty_pin_kind), "INVALID_PIN_KIND"),
          "empty pin kind reported");

  ccad::Project empty_net_id = validProject();
  empty_net_id.schematics[0].nets.at(0).id.clear();
  require(hasCode(ccad::runErc(empty_net_id), "INVALID_NET_ID"), "empty net id reported");

  ccad::Project duplicate_net_id = validProject();
  duplicate_net_id.schematics[0].nets.push_back(duplicate_net_id.schematics[0].nets.front());
  require(hasCode(ccad::runErc(duplicate_net_id), "DUPLICATE_NET_ID"),
          "duplicate net id reported");

  ccad::Project invalid_net_member = validProject();
  invalid_net_member.schematics[0].nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "", .pin_name = "VDD"});
  require(hasCode(ccad::runErc(invalid_net_member), "INVALID_NET_MEMBER"),
          "invalid net member reported");

  ccad::Project unknown_component = validProject();
  unknown_component.schematics[0].nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U404", .pin_name = "VDD"});
  require(hasCode(ccad::runErc(unknown_component), "UNKNOWN_COMPONENT"),
          "unknown component reported");

  ccad::Project unknown_pin = validProject();
  unknown_pin.schematics[0].nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U1", .pin_name = "NOPE"});
  require(hasCode(ccad::runErc(unknown_pin), "UNKNOWN_PIN"), "unknown pin reported");

  ccad::Project duplicate_member = validProject();
  duplicate_member.schematics[0].nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U1", .pin_name = "VDD"});
  require(hasCode(ccad::runErc(duplicate_member), "DUPLICATE_NET_MEMBER"),
          "duplicate net member reported");

  ccad::Project empty;
  empty.schematics.push_back(ccad::Schematic{});
  empty.schematics.push_back(ccad::Schematic{});
  empty.id = "proj-empty";
  empty.name = "empty";
  const std::vector<ccad::Diagnostic> empty_diagnostics = ccad::runErc(empty);
  require(hasCode(empty_diagnostics, "EMPTY_PROJECT"), "empty project reported");
  require(empty_diagnostics.at(0).severity == "warning", "empty project is warning");
}
