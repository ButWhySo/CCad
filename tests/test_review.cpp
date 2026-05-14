#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "test_support.hpp"

#include <string>

namespace {

ccad::Project validProject() {
  ccad::Project project;
  project.id = "proj-review";
  project.name = "review";
  project.components.push_back(ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "VDD", .kind = "power"}},
  });
  project.nets.push_back(ccad::Net{
      .id = "N_3V3",
      .members = {ccad::NetMember{.component_id = "U1", .pin_name = "VDD"}},
  });
  project.constraints.push_back(ccad::Constraint{
      .id = "C_3V3",
      .kind = "voltage",
      .target = "N_3V3",
      .value = "3.3V",
  });
  return project;
}

}  // namespace

int main() {
  const ccad::ProjectReview clean = ccad::buildReview(validProject());
  require(clean.project_id == "proj-review", "review keeps project id");
  require(clean.project_name == "review", "review keeps project name");
  require(clean.component_count == 1, "component count set");
  require(clean.net_count == 1, "net count set");
  require(clean.constraint_count == 1, "constraint count set");
  require(clean.error_count == 0, "clean review has no errors");
  require(clean.warning_count == 0, "clean review has no warnings");
  require(clean.status == "Clean: 1 component, 1 net, 1 constraint", "clean status text");

  ccad::Project invalid = validProject();
  invalid.nets.at(0).members.push_back(
      ccad::NetMember{.component_id = "U404", .pin_name = "VDD"});
  const ccad::ProjectReview invalid_review = ccad::buildReview(invalid);
  require(invalid_review.error_count == 1, "invalid review has one error");
  require(invalid_review.warning_count == 0, "invalid review has no warnings");
  require(invalid_review.diagnostics.size() == 1, "invalid review carries diagnostic");
  require(invalid_review.status == "Errors: 1, warnings: 0", "invalid status text");

  ccad::Project empty;
  empty.id = "proj-empty";
  empty.name = "empty";
  const ccad::ProjectReview warning_review = ccad::buildReview(empty);
  require(warning_review.error_count == 0, "warning review has no errors");
  require(warning_review.warning_count == 1, "warning review has one warning");
  require(warning_review.status == "Warnings: 1", "warning status text");
}

