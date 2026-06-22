#include "ccad_core/model.hpp"
#include "ccad_core/transaction.hpp"
#include "test_support.hpp"

#include <algorithm>

namespace {

bool contains(const std::vector<std::string>& values, const std::string& expected) {
  return std::find(values.begin(), values.end(), expected) != values.end();
}

ccad::Project beforeProject() {
  ccad::Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.id = "proj-before";
  project.name = "before";
  return project;
}

ccad::Project afterProject() {
  ccad::Project project = beforeProject();
  project.schematics[0].components.push_back(ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "VDD", .kind = "power"}},
  });
  return project;
}

ccad::Project beforeBoardProject() {
  ccad::Project project;
  project.id = "proj-board-before";
  project.name = "board before";
  project.boards.push_back(ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
          .size = ccad::Size{.width = ccad::millimeters(40),
                              .height = ccad::millimeters(20)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.Cu",
                              .name = "Front copper",
                              .kind = "signal",
                              .visible = true}},
      .pads = {ccad::Pad{.id = "U1.1",
                         .component_id = "U1",
                         .pin_name = "1",
                         .net_id = "N1",
                         .type = "smd",
                         .position = ccad::Point{.x = ccad::millimeters(5),
                                                 .y = ccad::millimeters(5)},
                         .padstack = ccad::Padstack{
                             .layer_set = {"F.Cu"},
                             .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = ccad::Size{.width = ccad::millimeters(1), .height = ccad::millimeters(1)}, .offset = ccad::Point{.x = ccad::millimeters(0), .y = ccad::millimeters(0)}, .roundrect_rratio = 0.0, .chamfer_ratio = 0.0, .chamfer_positions = 0, .trapezoid_delta_size = ccad::Size{.width = ccad::millimeters(0), .height = ccad::millimeters(0)}}}}}
                         }}},
  });
  return project;
}

ccad::Project afterBoardProject() {
  ccad::Project project = beforeBoardProject();
  project.boards[0].outline.size.width = ccad::millimeters(45);
  project.boards[0].vias.push_back(ccad::Via{
      .id = "V1",
      .net_id = "N1",
      .position = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(6)},
      .diameter = ccad::millimeters(0.8),
      .drill = ccad::millimeters(0.4),
  });
  return project;
}

}  // namespace

int main() {
  const ccad::Transaction transaction = ccad::buildTransaction(
      "txn-001", "sch add-component U1", "Add MCU component", beforeProject(), afterProject());

  require(transaction.id == "txn-001", "transaction id set");
  require(transaction.command == "sch add-component U1", "transaction command set");
  require(transaction.summary == "Add MCU component", "transaction summary set");
  require(transaction.before_project_id == "proj-before", "before project id set");
  require(transaction.after_project_id == "proj-before", "after project id set");
  require(transaction.diff.added_count == 1, "transaction diff has added count");
  require(transaction.diff.entries.size() == 1, "transaction diff carries entry");
  require(transaction.diff.entries.at(0).object_id == "U1", "transaction diff identifies object");

  const std::string json = ccad::dumpTransactionJson(transaction);
  require(json.find("\"id\": \"txn-001\"") != std::string::npos, "transaction json has id");
  require(json.find("\"command\": \"sch add-component U1\"") != std::string::npos,
          "transaction json has command");
  require(json.find("\"added\": 1") != std::string::npos, "transaction json has added count");
  require(json.find("\"object_id\": \"U1\"") != std::string::npos,
          "transaction json has diff object id");

  const ccad::Transaction board_transaction = ccad::buildTransaction(
      "txn-002", "pcb board commit", "Resize outline and add via", beforeBoardProject(),
      afterBoardProject());

  require(board_transaction.impact.board_modified, "commit impact marks board modified");
  require(!board_transaction.impact.schematic_modified,
          "commit impact leaves schematic clean for board mutation");
  require(board_transaction.impact.view_dirty, "commit impact marks view dirty");
  require(board_transaction.impact.drc_dirty, "commit impact marks drc dirty");
  require(board_transaction.impact.connectivity_dirty,
          "commit impact marks connectivity dirty");
  require(board_transaction.impact.ratsnest_dirty, "commit impact marks ratsnest dirty");
  require(board_transaction.impact.board_outline_dirty,
          "commit impact marks board outline dirty");
  require(board_transaction.impact.solder_mask_dirty,
          "commit impact marks solder mask dirty for via changes");
  require(contains(board_transaction.impact.dirty_object_ids, "board"),
          "commit impact tracks dirty board outline id");
  require(contains(board_transaction.impact.dirty_object_ids, "V1"),
          "commit impact tracks dirty via id");

  const std::string board_json = ccad::dumpTransactionJson(board_transaction);
  require(board_json.find("\"impact\"") != std::string::npos,
          "transaction json has impact section");
  require(board_json.find("\"board_modified\": true") != std::string::npos,
          "transaction json marks board modified");
  require(board_json.find("\"board_outline_dirty\": true") != std::string::npos,
          "transaction json marks outline dirty");
  require(board_json.find("\"solder_mask_dirty\": true") != std::string::npos,
          "transaction json marks solder mask dirty");
  require(board_json.find("\"dirty_object_ids\"") != std::string::npos,
          "transaction json has dirty object ids");
  require(board_json.find("\"V1\"") != std::string::npos,
          "transaction json includes dirty via id");
}

