#include "ccad_core/board_text_var_adapter.hpp"

#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"

#include <cstdlib>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
  if (!condition) {
    throw message;
  }
}

ccad::Project textVariableProject() {
  ccad::Project project;
  project.schema_version = 2;
  project.id = "proj-text-vars";
  project.name = "text-vars";
  project.text_variables["REV"] = "A1";
  project.text_variables["COMPANY"] = "CCad Labs";
  project.boards.push_back(ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(20),
                              .height = ccad::millimeters(10)},
      },
      .design_rules = ccad::DesignRules{},
      .layers = {ccad::Layer{.id = "F.SilkS",
                             .name = "Front silkscreen",
                             .kind = "silkscreen",
                             .visible = true}},
      .placement_regions = {},
      .keepouts = {},
      .pads = {},
      .vias = {},
      .tracks = {},
      .graphics = {},
      .texts = {ccad::BoardText{.id = "TXT1",
                                .layer_id = "F.SilkS",
                                .text = "Rev ${REV} by ${COMPANY}",
                                .position = ccad::Point{.x = ccad::millimeters(5),
                                                        .y = ccad::millimeters(5)},
                                .rotation_degrees = 0.0,
                                .size = ccad::Size{.width = ccad::millimeters(1),
                                                   .height = ccad::millimeters(1)}},
                ccad::BoardText{.id = "TXT2",
                                .layer_id = "F.SilkS",
                                .text = "Missing ${UNKNOWN}",
                                .position = ccad::Point{.x = ccad::millimeters(8),
                                                        .y = ccad::millimeters(5)},
                                .rotation_degrees = 0.0,
                                .size = ccad::Size{.width = ccad::millimeters(1),
                                                   .height = ccad::millimeters(1)}}},
      .zones = {},
      .route_requests = {},
  });
  return project;
}

void test_project_text_variables_round_trip() {
  const ccad::Project project = textVariableProject();
  const std::string json = ccad::dumpProjectJson(project);

  require(json.find("\"text_variables\"") != std::string::npos,
          "project json writes text variables");
  require(json.find("\"REV\": \"A1\"") != std::string::npos,
          "project json writes text variable key");

  const ccad::Project loaded = ccad::loadProjectJson(json);
  require(loaded.text_variables.at("REV") == "A1", "project json reads REV variable");
  require(loaded.text_variables.at("COMPANY") == "CCad Labs",
          "project json reads COMPANY variable");
}

void test_expand_text_variables_reports_resolved_and_unresolved_tokens() {
  const ccad::Project project = textVariableProject();
  std::vector<ccad::TextVariableReference> refs;

  const std::string expanded = ccad::expandTextVariables(
      "Board ${REV} ${UNKNOWN} ${COMPANY}", project.text_variables, &refs);

  require(expanded == "Board A1 ${UNKNOWN} CCad Labs",
          "text variable expansion replaces known variables and leaves unknown tokens");
  require(refs.size() == 3, "expansion records all variable references");
  require(refs.at(0).name == "REV", "first reference keeps token name");
  require(refs.at(0).resolved, "known token is marked resolved");
  require(refs.at(1).name == "UNKNOWN", "unknown reference keeps token name");
  require(!refs.at(1).resolved, "unknown token is marked unresolved");
}

void test_expand_board_texts_uses_project_variables() {
  const ccad::Project project = textVariableProject();
  const ccad::Board& board = project.boards.front();

  const std::vector<ccad::ExpandedBoardText> expanded =
      ccad::expandBoardTexts(project, board);

  require(expanded.size() == 2, "board text expansion returns all board texts");
  require(expanded.at(0).id == "TXT1", "board text expansion keeps text id");
  require(expanded.at(0).expanded_text == "Rev A1 by CCad Labs",
          "board text expansion resolves known project variables");
  require(expanded.at(0).references.size() == 2,
          "board text expansion records known text references");
  require(expanded.at(1).expanded_text == "Missing ${UNKNOWN}",
          "board text expansion leaves unknown board text variable visible");
  require(!expanded.at(1).references.at(0).resolved,
          "board text expansion records unresolved board text variable");
}

}  // namespace

int main() {
  try {
    test_project_text_variables_round_trip();
    test_expand_text_variables_reports_resolved_and_unresolved_tokens();
    test_expand_board_texts_uses_project_variables();
  } catch (const char* message) {
    return message ? 1 : 2;
  }

  return EXIT_SUCCESS;
}
