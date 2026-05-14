#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"

#include <cassert>
#include <string>

using ccad::Component;
using ccad::Constraint;
using ccad::Net;
using ccad::NetMember;
using ccad::Pin;
using ccad::Project;

int main() {
  Project project;
  project.id = "proj-demo";
  project.name = "demo";
  project.components.push_back(Component{
      .id = "U1",
      .part = "MCU",
      .pins = {Pin{.name = "VDD", .kind = "power"}, Pin{.name = "GND", .kind = "power"}},
  });
  project.nets.push_back(Net{
      .id = "N_3V3",
      .members = {NetMember{.component_id = "U1", .pin_name = "VDD"}},
  });
  project.constraints.push_back(Constraint{
      .id = "C_supply",
      .kind = "voltage",
      .target = "N_3V3",
      .value = "3.3V",
  });

  const std::string json = ccad::dumpProjectJson(project);

  assert(json.find("\"schema_version\": 1") != std::string::npos);
  assert(json.find("\"id\": \"proj-demo\"") != std::string::npos);
  assert(json.find("\"component_id\": \"U1\"") != std::string::npos);

  const Project loaded = ccad::loadProjectJson(json);
  assert(loaded.id == "proj-demo");
  assert(loaded.name == "demo");
  assert(loaded.components.size() == 1);
  assert(loaded.components.at(0).pins.size() == 2);
  assert(loaded.nets.size() == 1);
  assert(loaded.constraints.size() == 1);
  assert(ccad::dumpProjectJson(loaded) == json);
}

