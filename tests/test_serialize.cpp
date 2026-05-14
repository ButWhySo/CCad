#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"
#include "test_support.hpp"

#include <stdexcept>
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

  require(json.find("\"schema_version\": 1") != std::string::npos, "schema version emitted");
  require(json.find("\"id\": \"proj-demo\"") != std::string::npos, "project id emitted");
  require(json.find("\"component_id\": \"U1\"") != std::string::npos, "net member emitted");

  const Project loaded = ccad::loadProjectJson(json);
  require(loaded.id == "proj-demo", "project id round trips");
  require(loaded.name == "demo", "project name round trips");
  require(loaded.components.size() == 1, "component count round trips");
  require(loaded.components.at(0).pins.size() == 2, "pin count round trips");
  require(loaded.nets.size() == 1, "net count round trips");
  require(loaded.constraints.size() == 1, "constraint count round trips");
  require(ccad::dumpProjectJson(loaded) == json, "json output is deterministic");

  Project escaped;
  escaped.id = "proj-escaped";
  escaped.name = "line\n tab\t quote\" slash\\";
  const std::string escaped_json = ccad::dumpProjectJson(escaped);
  require(escaped_json.find("\\n") != std::string::npos, "newline escaped");
  require(escaped_json.find("\\t") != std::string::npos, "tab escaped");
  require(ccad::loadProjectJson(escaped_json).name == escaped.name, "escapes round trip");

  bool rejected_trailing_garbage = false;
  try {
    (void)ccad::loadProjectJson(json + " garbage");
  } catch (const std::runtime_error&) {
    rejected_trailing_garbage = true;
  }
  require(rejected_trailing_garbage, "trailing garbage rejected");

  bool rejected_trailing_comma = false;
  try {
    (void)ccad::loadProjectJson("{\"schema_version\": 1,}");
  } catch (const std::runtime_error&) {
    rejected_trailing_comma = true;
  }
  require(rejected_trailing_comma, "trailing comma rejected");
}
