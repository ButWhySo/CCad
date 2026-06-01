#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_gui/review_window.hpp"
#include "test_support.hpp"

#include <QApplication>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

ccad::Project uiMapProject() {
  ccad::Project project;
  project.id = "proj-ui-map";
  project.name = "UI map";
  project.board = ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
          .size = ccad::Size{.width = ccad::millimeters(42),
                              .height = ccad::millimeters(30)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu",
                              .name = "Front copper",
                              .kind = "copper",
                              .visible = true}},
      .placement_regions = {},
      .keepouts = {},
      .pads = {ccad::Pad{.id = "U1.1",
                         .component_id = "U1",
                         .pin_name = "1",
                         .net_id = "N1",
                         .layers = {"F.Cu"},
                         .type = "smd",
                         .shape = "roundrect",
                         .position = ccad::Point{.x = ccad::millimeters(8),
                                                  .y = ccad::millimeters(9)},
                         .size = ccad::Size{.width = ccad::millimeters(1.5),
                                            .height = ccad::millimeters(1.0)},
                         .drill = std::nullopt,
                         .roundrect_rratio = 0.25,
                         .chamfer_ratio = std::nullopt}},
      .vias = {},
      .tracks = {},
      .route_requests = {},
  };
  return project;
}

std::filesystem::path writeProjectFixture() {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() / "ccad-ui-map-test.ccad.json";
  std::ofstream output(path, std::ios::binary);
  output << ccad::dumpProjectJson(uiMapProject());
  return path;
}

bool contains(const QString& haystack, const char* needle) {
  return haystack.contains(QString::fromUtf8(needle));
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  const std::filesystem::path project_path = writeProjectFixture();
  ReviewWindow window;
  window.loadProjectPath(project_path);
  window.show();
  QApplication::processEvents();

  const QString map = window.uiMapJson();
  require(contains(map, "\"schema_version\":1"), "UI map includes schema version");
  require(contains(map, "\"ui_epoch\":"), "UI map includes changing epoch");
  require(contains(map, "\"id\":\"action:add_footprint\""),
          "UI map exposes add footprint action");
  require(contains(map, "\"id\":\"tab:pcb\""), "UI map exposes PCB tab");
  require(contains(map, "\"id\":\"tab:schematic\""), "UI map exposes schematic tab");
  require(contains(map, "\"id\":\"canvas:pcb\""), "UI map exposes PCB canvas");
  require(contains(map, "\"id\":\"canvas_object:U1.1\""),
          "UI map exposes typed canvas object");
  require(contains(map, "\"object_id\":\"U1.1\""), "UI map keeps original object id");
  require(contains(map, "\"layer_id\":\"F.Cu\""), "UI map carries layer metadata");
  require(contains(map, "\"target_x\":"), "UI map carries click target x coordinate");
  require(contains(map, "\"target_y\":"), "UI map carries click target y coordinate");

  const QString validation = window.validateUiMapTargetsJson(false);
  require(contains(validation, "\"summary\":"), "UI map validation includes summary");
  require(contains(validation, "\"failures\":0"), "UI map validation has no misses");
  require(contains(validation, "\"id\":\"action:add_footprint\""),
          "UI map validation covers add footprint action");
  require(contains(validation, "\"id\":\"canvas_object:U1.1\""),
          "UI map validation covers canvas object target");

  const QString action_target = window.uiTargetJsonById("action:add_footprint");
  require(contains(action_target, "\"found\":true"), "action target query finds add footprint");
  require(contains(action_target, "\"role\":\"action\""), "action target query reports role");
  require(contains(action_target, "\"physical_x\":"), "action target reports physical pixels");

  const QString pad_target = window.uiTargetJsonById("canvas_object:U1.1");
  require(contains(pad_target, "\"found\":true"), "canvas object target query finds pad");
  require(contains(pad_target, "\"role\":\"canvas_object\""),
          "canvas object target query reports role");

  const QString board_point = window.uiTargetJsonForBoardPoint(8.0, 9.0);
  require(contains(board_point, "\"found\":true"), "board point target query finds point");
  require(contains(board_point, "\"space\":\"board\""), "board point target reports space");
  require(contains(board_point, "\"scene_x\":"), "board point target reports scene mapping");

  const QString unknown = window.uiTargetJsonById("action:not_real");
  require(contains(unknown, "\"found\":false"), "unknown target id fails explicitly");
  require(contains(unknown, "\"reason\":\"unknown_id\""), "unknown target id reports reason");

  const QString outside = window.uiTargetJsonForBoardPoint(99.0, 99.0);
  require(contains(outside, "\"found\":false"), "outside board point fails explicitly");
  require(contains(outside, "\"reason\":\"outside_board\""),
          "outside board point reports reason");
}
