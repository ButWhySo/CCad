#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"
#include "test_support.hpp"

#include <stdexcept>
#include <string>

using ccad::SchSymbol;
using ccad::Constraint;
using ccad::Net;
using ccad::NetMember;
using ccad::SchPin;
using ccad::Project;

int main() {
  Project project;
  project.id = "proj-demo";
  project.name = "demo";
  project.boards.push_back(ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .design_rules = ccad::DesignRules{.copper_clearance = ccad::millimeters(0.15),
                                        .min_track_width = ccad::millimeters(0.12),
                                        .min_via_annular_ring = ccad::millimeters(0.08),
                                        .min_connection = ccad::millimeters(0.01),
                                        .min_via_diameter = ccad::millimeters(0.60),
                                        .min_through_hole_drill = ccad::millimeters(0.35),
                                        .min_microvia_diameter = ccad::millimeters(0.20),
                                        .min_microvia_drill = ccad::millimeters(0.10),
                                        .min_hole_to_hole = ccad::millimeters(0.25),
                                        .hole_clearance = ccad::millimeters(0.25),
                                        .copper_edge_clearance = ccad::millimeters(0.50),
                                        .silk_clearance = ccad::millimeters(0.02),
                                        .min_groove_width = ccad::millimeters(0.10),
                                        .solder_mask_expansion = ccad::millimeters(0.03),
                                        .solder_mask_min_width = ccad::millimeters(0.10),
                                        .solder_mask_to_copper_clearance =
                                            ccad::millimeters(0.02),
                                        .solder_paste_margin = ccad::millimeters(-0.01),
                                        .solder_paste_margin_ratio = -0.05,
                                        .board_thickness = ccad::millimeters(1.60),
                                        .use_height_for_length_calcs = false,
                                        .tent_vias_front = true,
                                        .tent_vias_back = false,
                                        .cover_vias_front = true,
                                        .cover_vias_back = false,
                                        .plug_vias_front = true,
                                        .plug_vias_back = false,
                                        .cap_vias = true,
                                        .fill_vias = false},
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
                 ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true},
                 ccad::Layer{.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen", .visible = true},
                 ccad::Layer{.id = "Dwgs.User", .name = "User drawings", .kind = "user", .visible = true}},
      .placement_regions = {ccad::PlacementRegion{
          .id = "PR1",
          .kind = "component",
          .area = ccad::Rect{
              .origin = ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(3)},
              .size = ccad::Size{.width = ccad::millimeters(10),
                                  .height = ccad::millimeters(6)}}}},
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
                         .position = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(6)},
                         .rotation_degrees = 90.0,
                         .padstack = ccad::Padstack{
                            .layer_set = {"F.Cu"},
                            .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(1.0)}, .roundrect_rratio = 0.25}}}}
                         }}},
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
          .source_route_request_id = "RR1",
      }},
      .graphics = {ccad::BoardGraphic{.id = "G1",
                                       .kind = "line",
                                       .layer_id = "Dwgs.User",
                                       .start = ccad::Point{.x = ccad::millimeters(3),
                                                           .y = ccad::millimeters(4)},
                                       .end = ccad::Point{.x = ccad::millimeters(16),
                                                         .y = ccad::millimeters(4)},
                                       .width = ccad::millimeters(0.15)}},
      .texts = {ccad::BoardText{.id = "BT1",
                                .layer_id = "F.SilkS",
                                .text = "Bridge rectifier",
                                .position = ccad::Point{.x = ccad::millimeters(7),
                                                        .y = ccad::millimeters(22)},
                                .rotation_degrees = 90.0,
                                .size = ccad::Size{.width = ccad::millimeters(1.5),
                                                   .height = ccad::millimeters(1.5)}}},
      .zones = {ccad::BoardZone{
          .id = "Z_GND",
          .name = "GND pour",
          .net_id = "N_3V3",
          .layer_ids = {"F.Cu", "B.Cu"},
          .outline = {ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(2)},
                      ccad::Point{.x = ccad::millimeters(20), .y = ccad::millimeters(2)},
                      ccad::Point{.x = ccad::millimeters(20), .y = ccad::millimeters(12)},
                      ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(12)}},
          .priority = 1,
          .clearance = ccad::millimeters(0.2),
          .min_thickness = ccad::millimeters(0.25),
          .fill_enabled = true,
          .pad_connection = "thermal",
      }},
      .route_requests = {ccad::RouteRequest{
          .id = "RR1",
          .net_id = "N_3V3",
          .from_object_id = "P1",
          .to_object_id = "V1",
          .preferred_layer_id = "F.Cu",
          .policy = "shortest_safe",
          .width = ccad::millimeters(0.25),
      }},
  });
  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].symbols.push_back(SchSymbol{
      .id = "U1",
      .lib_id = "MCU",
      .pins = {SchPin{.name = "VDD", .number = "", .electrical_type = ccad::ElectricalPinType::PowerIn}, SchPin{.name = "GND", .number = "", .electrical_type = ccad::ElectricalPinType::PowerIn}},
  });
  project.schematics[0].nets.push_back(Net{
      .id = "N_3V3",
      .members = {NetMember{.component_id = "U1", .pin_name = "VDD"}},
  });
  project.schematics[0].groups.push_back(ccad::SchGroup{
      .id = "G1",
      .name = "My Group",
      .members = {"U1", "N_3V3"},
  });
  project.schematics[0].constraints.push_back(Constraint{
      .id = "C_supply",
      .kind = "voltage",
      .target = "N_3V3",
      .value = "3.3V",
  });

  const std::string json = ccad::dumpProjectJson(project);

  require(json.find("\"schema_version\": 2") != std::string::npos, "schema version emitted");
  require(json.find("\"id\": \"proj-demo\"") != std::string::npos, "project id emitted");
  require(json.find("\"board\"") != std::string::npos, "board emitted");
  require(json.find("\"width_nm\": 42000000") != std::string::npos, "board width emitted");
  require(json.find("\"design_rules\"") != std::string::npos, "design rules emitted");
  require(json.find("\"copper_clearance_nm\": 150000") != std::string::npos,
          "copper clearance rule emitted");
  require(json.find("\"min_via_diameter_nm\": 600000") != std::string::npos,
          "minimum via diameter emitted");
  require(json.find("\"min_through_hole_drill_nm\": 350000") != std::string::npos,
          "minimum through hole drill emitted");
  require(json.find("\"solder_mask_expansion_nm\": 30000") != std::string::npos,
          "solder mask expansion emitted");
  require(json.find("\"solder_paste_margin_ratio\": -0.05") != std::string::npos,
          "solder paste margin ratio emitted");
  require(json.find("\"board_thickness_nm\": 1600000") != std::string::npos,
          "board thickness emitted");
  require(json.find("\"placement_regions\"") != std::string::npos,
          "placement regions emitted");
  require(json.find("\"route_requests\"") != std::string::npos, "route requests emitted");
  require(json.find("\"source_route_request_id\": \"RR1\"") != std::string::npos,
          "track route request provenance emitted");
  require(json.find("\"policy\": \"shortest_safe\"") != std::string::npos,
          "route request policy emitted");
  require(json.find("\"keepouts\"") != std::string::npos, "keepouts emitted");
  require(json.find("\"graphics\"") != std::string::npos, "board graphics emitted");
  require(json.find("\"kind\": \"line\"") != std::string::npos,
          "board graphic line kind emitted");
  require(json.find("\"texts\"") != std::string::npos, "board text emitted");
  require(json.find("\"text\": \"Bridge rectifier\"") != std::string::npos,
          "board text value emitted");
  require(json.find("\"zones\"") != std::string::npos, "board zones emitted");
  require(json.find("\"id\": \"Z_GND\"") != std::string::npos, "board zone id emitted");
  require(json.find("\"layer_ids\"") != std::string::npos, "board zone layers emitted");
  require(json.find("\"pad_connection\": \"thermal\"") != std::string::npos,
          "board zone pad connection emitted");
  require(json.find("\"component_id\": \"U1\"") != std::string::npos, "net member emitted");
  if (json.find("\"roundrect_rratio\": 0.25") == std::string::npos) {
    std::cout << "TEST FAILED! JSON IS:\n" << json << "\n";
  }
  require(json.find("\"roundrect_rratio\": 0.25") != std::string::npos,
          "pad roundrect ratio emitted");

  const Project loaded = ccad::loadProjectJson(json);
  require(loaded.id == "proj-demo", "project id round trips");
  require(loaded.name == "demo", "project name round trips");
  require(!loaded.boards.empty(), "board round trips");
  require(loaded.boards[0].outline.size.width.nanometers == 42000000, "board width round trips");
  require(loaded.boards[0].outline.size.height.nanometers == 28000000, "board height round trips");
  require(loaded.boards[0].design_rules.copper_clearance.nanometers == 150000,
          "copper clearance rule round trips");
  require(loaded.boards[0].design_rules.min_track_width.nanometers == 120000,
          "minimum track width rule round trips");
  require(loaded.boards[0].design_rules.min_via_annular_ring.nanometers == 80000,
          "minimum via annular ring rule round trips");
  require(loaded.boards[0].design_rules.min_connection.nanometers == 10000,
          "minimum connection rule round trips");
  require(loaded.boards[0].design_rules.min_via_diameter.nanometers == 600000,
          "minimum via diameter rule round trips");
  require(loaded.boards[0].design_rules.min_through_hole_drill.nanometers == 350000,
          "minimum through hole drill rule round trips");
  require(loaded.boards[0].design_rules.min_hole_to_hole.nanometers == 250000,
          "minimum hole to hole rule round trips");
  require(loaded.boards[0].design_rules.copper_edge_clearance.nanometers == 500000,
          "copper edge clearance rule round trips");
  require(loaded.boards[0].design_rules.solder_mask_expansion.nanometers == 30000,
          "solder mask expansion round trips");
  require(loaded.boards[0].design_rules.solder_mask_min_width.nanometers == 100000,
          "solder mask minimum width round trips");
  require(loaded.boards[0].design_rules.solder_mask_to_copper_clearance.nanometers == 20000,
          "solder mask to copper clearance round trips");
  require(loaded.boards[0].design_rules.solder_paste_margin.nanometers == -10000,
          "solder paste margin round trips");
  require(loaded.boards[0].design_rules.solder_paste_margin_ratio == -0.05,
          "solder paste margin ratio round trips");
  require(loaded.boards[0].design_rules.board_thickness.nanometers == 1600000,
          "board thickness round trips");
  require(!loaded.boards[0].design_rules.use_height_for_length_calcs,
          "height-for-length calculation flag round trips");
  require(loaded.boards[0].design_rules.tent_vias_front, "front via tenting flag round trips");
  require(!loaded.boards[0].design_rules.tent_vias_back, "back via tenting flag round trips");
  require(loaded.boards[0].design_rules.cover_vias_front, "front via covering flag round trips");
  require(!loaded.boards[0].design_rules.cover_vias_back, "back via covering flag round trips");
  require(loaded.boards[0].design_rules.plug_vias_front, "front via plugging flag round trips");
  require(!loaded.boards[0].design_rules.plug_vias_back, "back via plugging flag round trips");
  require(loaded.boards[0].design_rules.cap_vias, "via capping flag round trips");
  require(!loaded.boards[0].design_rules.fill_vias, "via filling flag round trips");
  require(loaded.boards[0].layers.size() == 4, "board layers round trip");
  require(loaded.boards[0].layers.at(2).id == "F.SilkS", "front silkscreen layer round trips");
  require(loaded.boards[0].layers.at(3).id == "Dwgs.User", "user drawing layer round trips");
  require(loaded.boards[0].placement_regions.size() == 1, "board placement regions round trip");
  require(loaded.boards[0].placement_regions.at(0).area.size.width.nanometers == 10000000,
          "placement region width round trips");
  require(loaded.boards[0].keepouts.size() == 1, "board keepouts round trip");
  require(loaded.boards[0].keepouts.at(0).area.size.width.nanometers == 4000000,
          "keepout width round trips");
  require(loaded.boards[0].pads.size() == 1, "board pads round trip");
  require(loaded.boards[0].pads.at(0).position.x.nanometers == 5000000, "pad x round trips");
  require(loaded.boards[0].pads.at(0).rotation_degrees == 90.0, "pad rotation round trips");
  require(!loaded.boards[0].pads.at(0).padstack.copper_props.empty() && loaded.boards[0].pads.at(0).padstack.copper_props.begin()->second.shape.roundrect_rratio > 0, "pad roundrect ratio round trips");
  require(loaded.boards[0].pads.at(0).padstack.copper_props.begin()->second.shape.roundrect_rratio == 0.25, "pad roundrect ratio value round trips");
  require(loaded.boards[0].vias.size() == 1, "board vias round trip");
  require(loaded.boards[0].vias.at(0).drill.nanometers == 400000, "via drill round trips");
  require(loaded.boards[0].tracks.size() == 1, "board tracks round trip");
  require(loaded.boards[0].tracks.at(0).width.nanometers == 250000, "track width round trips");
  require(loaded.boards[0].tracks.at(0).source_route_request_id == "RR1",
          "track route request provenance round trips");
  require(loaded.boards[0].graphics.size() == 1, "board graphics round trip");
  require(loaded.boards[0].graphics.at(0).kind == "line", "board graphic kind round trips");
  require(loaded.boards[0].graphics.at(0).layer_id == "Dwgs.User",
          "board graphic layer round trips");
  require(loaded.boards[0].graphics.at(0).width.nanometers == 150000,
          "board graphic width round trips");
  require(loaded.boards[0].texts.size() == 1, "board texts round trip");
  require(loaded.boards[0].texts.at(0).layer_id == "F.SilkS", "board text layer round trips");
  require(loaded.boards[0].texts.at(0).text == "Bridge rectifier",
          "board text value round trips");
  require(loaded.boards[0].texts.at(0).rotation_degrees == 90.0,
          "board text rotation round trips");
  require(loaded.boards[0].texts.at(0).size.width.nanometers == 1500000,
          "board text size round trips");
  require(loaded.boards[0].zones.size() == 1, "board zones round trip");
  require(loaded.boards[0].zones.at(0).id == "Z_GND", "board zone id round trips");
  require(loaded.boards[0].zones.at(0).name == "GND pour", "board zone name round trips");
  require(loaded.boards[0].zones.at(0).net_id == "N_3V3", "board zone net round trips");
  require(loaded.boards[0].zones.at(0).layer_ids.size() == 2,
          "board zone layer set round trips");
  require(loaded.boards[0].zones.at(0).outline.size() == 4, "board zone outline round trips");
  require(loaded.boards[0].zones.at(0).priority == 1, "board zone priority round trips");
  require(loaded.boards[0].zones.at(0).clearance.nanometers == 200000,
          "board zone clearance round trips");
  require(loaded.boards[0].zones.at(0).min_thickness.nanometers == 250000,
          "board zone min thickness round trips");
  require(loaded.boards[0].zones.at(0).fill_enabled, "board zone fill state round trips");
  require(loaded.boards[0].zones.at(0).pad_connection == "thermal",
          "board zone pad connection round trips");
  require(loaded.boards[0].route_requests.size() == 1, "route requests round trip");
  require(loaded.boards[0].route_requests.at(0).from_object_id == "P1",
          "route request start object round trips");
  require(loaded.boards[0].route_requests.at(0).to_object_id == "V1",
          "route request target object round trips");
  require(loaded.boards[0].route_requests.at(0).preferred_layer_id == "F.Cu",
          "route request preferred layer round trips");
  require(loaded.boards[0].route_requests.at(0).width.nanometers == 250000,
          "route request width round trips");
  require(loaded.schematics[0].symbols.size() == 1, "component count round trips");
  require(loaded.schematics[0].symbols.at(0).pins.size() == 2, "pin count round trips");
  require(loaded.schematics[0].nets.size() == 1, "net count round trips");
  require(loaded.schematics[0].groups.size() == 1, "sch group count round trips");
  require(loaded.schematics[0].groups.at(0).id == "G1", "sch group id round trips");
  require(loaded.schematics[0].groups.at(0).members.size() == 2, "sch group members round trip");
  require(loaded.schematics[0].constraints.size() == 1, "constraint count round trips");
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
