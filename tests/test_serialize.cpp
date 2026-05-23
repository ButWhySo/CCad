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
  project.board = ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                 ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true}},
      .keepouts = {ccad::Keepout{.id = "K1",
                                 .kind = "placement",
                                 .area = ccad::Rect{
                                     .origin = ccad::Point{.x = ccad::millimeters(20),
                                                           .y = ccad::millimeters(20)},
                                     .size = ccad::Size{.width = ccad::millimeters(4),
                                                        .height = ccad::millimeters(3)}}}},
      .pads = {ccad::Pad{.id = "P1",
                         .component_id = "U1",
                         .pin_name = "VDD",
                         .net_id = "N_3V3",
                         .layer_id = "F.Cu",
                         .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                         .rotation_degrees = 90.0,
                         .size = ccad::Size{.width = ccad::millimeters(1.5),
                                            .height = ccad::millimeters(1.0)}}},
      .vias = {ccad::Via{.id = "V1",
                         .net_id = "N_3V3",
                         .position = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
                         .diameter = ccad::millimeters(0.8),
                         .drill = ccad::millimeters(0.4)}},
      .tracks = {ccad::TrackSegment{
          .id = "T1",
          .net_id = "N_3V3",
          .layer_id = "F.Cu",
          .start = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
          .end = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(9)},
          .width = ccad::millimeters(0.25),
      }},
  };
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
  require(json.find("\"board\"") != std::string::npos, "board emitted");
  require(json.find("\"width_nm\": 42000000") != std::string::npos, "board width emitted");
  require(json.find("\"keepouts\"") != std::string::npos, "keepouts emitted");
  require(json.find("\"component_id\": \"U1\"") != std::string::npos, "net member emitted");

  const Project loaded = ccad::loadProjectJson(json);
  require(loaded.id == "proj-demo", "project id round trips");
  require(loaded.name == "demo", "project name round trips");
  require(loaded.board.has_value(), "board round trips");
  require(loaded.board->outline.size.width.nanometers == 42000000, "board width round trips");
  require(loaded.board->outline.size.height.nanometers == 28000000, "board height round trips");
  require(loaded.board->layers.size() == 2, "board layers round trip");
  require(loaded.board->keepouts.size() == 1, "board keepouts round trip");
  require(loaded.board->keepouts.at(0).area.size.width.nanometers == 4000000,
          "keepout width round trips");
  require(loaded.board->pads.size() == 1, "board pads round trip");
  require(loaded.board->pads.at(0).position.x.nanometers == 5000000, "pad x round trips");
  require(loaded.board->pads.at(0).rotation_degrees == 90.0, "pad rotation round trips");
  require(loaded.board->vias.size() == 1, "board vias round trip");
  require(loaded.board->vias.at(0).drill.nanometers == 400000, "via drill round trips");
  require(loaded.board->tracks.size() == 1, "board tracks round trip");
  require(loaded.board->tracks.at(0).width.nanometers == 250000, "track width round trips");
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
