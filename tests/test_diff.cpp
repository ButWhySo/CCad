#include "ccad_core/diff.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

#include <string>
#include <vector>

namespace {

ccad::Project baseProject() {
  ccad::Project project;
  project.id = "proj-diff";
  project.name = "diff";
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
  project.board = ccad::Board{
      .outline = ccad::Rect{
          .origin = ccad::Point{.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
          .size = ccad::Size{.width = ccad::millimeters(42), .height = ccad::millimeters(28)},
      },
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper",
                             .visible = true}},
  };
  return project;
}

bool hasEntry(const ccad::ProjectDiff& diff, const std::string& change,
              const std::string& object_type, const std::string& object_id) {
  for (const ccad::DiffEntry& entry : diff.entries) {
    if (entry.change == change && entry.object_type == object_type && entry.object_id == object_id) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main() {
  const ccad::Project clean_before = baseProject();
  const ccad::Project clean_after = baseProject();
  const ccad::ProjectDiff clean = ccad::diffProjects(clean_before, clean_after);
  require(clean.entries.empty(), "identical projects have no diff entries");
  require(clean.added_count == 0, "clean diff added count zero");
  require(clean.removed_count == 0, "clean diff removed count zero");
  require(clean.changed_count == 0, "clean diff changed count zero");

  ccad::Project added = baseProject();
  added.components.push_back(ccad::Component{
      .id = "U2",
      .part = "SENSOR",
      .pins = {ccad::Pin{.name = "OUT", .kind = "signal"}},
  });
  const ccad::ProjectDiff added_diff = ccad::diffProjects(baseProject(), added);
  require(added_diff.added_count == 1, "added count set");
  require(hasEntry(added_diff, "added", "component", "U2"), "added component entry");

  ccad::Project removed = baseProject();
  removed.nets.clear();
  const ccad::ProjectDiff removed_diff = ccad::diffProjects(baseProject(), removed);
  require(removed_diff.removed_count == 1, "removed count set");
  require(hasEntry(removed_diff, "removed", "net", "N_3V3"), "removed net entry");

  ccad::Project changed = baseProject();
  changed.constraints.at(0).value = "3.0V";
  const ccad::ProjectDiff changed_diff = ccad::diffProjects(baseProject(), changed);
  require(changed_diff.changed_count == 1, "changed count set");
  require(hasEntry(changed_diff, "changed", "constraint", "C_3V3"), "changed constraint entry");

  ccad::Project added_layer = baseProject();
  added_layer.board->layers.push_back(
      ccad::Layer{.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true});
  const ccad::ProjectDiff added_layer_diff = ccad::diffProjects(baseProject(), added_layer);
  require(hasEntry(added_layer_diff, "added", "layer", "B.Cu"), "added layer entry");

  ccad::Project changed_layer = baseProject();
  changed_layer.board->layers.at(0).visible = false;
  const ccad::ProjectDiff changed_layer_diff = ccad::diffProjects(baseProject(), changed_layer);
  require(hasEntry(changed_layer_diff, "changed", "layer", "F.Cu"), "changed layer entry");

  ccad::Project removed_layer = baseProject();
  removed_layer.board->layers.clear();
  const ccad::ProjectDiff removed_layer_diff = ccad::diffProjects(baseProject(), removed_layer);
  require(hasEntry(removed_layer_diff, "removed", "layer", "F.Cu"), "removed layer entry");
}

