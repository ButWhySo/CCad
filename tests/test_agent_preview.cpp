#include "ccad_core/agent_preview.hpp"
#include "test_support.hpp"

namespace {

ccad::Project routeProject() {
  ccad::Project project;
  project.id = "preview-project";
  project.boards.push_back(ccad::Board{
      .layers = {ccad::Layer{.id = "F.Cu", .name = "Front copper", .kind = "copper"}},
      .pads = {ccad::Pad{.id = "P1", .net_id = "GND",
                          .position = {ccad::millimeters(1), ccad::millimeters(1)}}},
  });
  return project;
}

}  // namespace

int main() {
  const ccad::Project source = routeProject();
  const ccad::RouteTrackPreviewRequest request{
      .start_x_mm = 1.0,
      .start_y_mm = 1.0,
      .end_x_mm = 6.0,
      .end_y_mm = 1.0,
      .net_id = "GND",
      .layer_id = "F.Cu",
  };
  std::string reason;
  const auto preview = ccad::stageRouteTrackPreview(source, request, &reason);
  require(preview.has_value(), "route preview stages a valid real route");
  require(reason == "staged", "successful staging reports staged");
  require(source.boards[0].tracks.empty(), "preview never mutates source project");
  require(preview->after.boards[0].tracks.size() == 1,
          "staged project contains route candidate");
  require(preview->diff.added_count == 1,
          "preview computes project diff from source to candidate");
  require(preview->focus_object_id == "interactive-track-1",
          "preview identifies the routed object");

  ccad::RouteTrackPreviewRequest invalid = request;
  invalid.net_id.clear();
  require(!ccad::stageRouteTrackPreview(source, invalid, &reason).has_value(),
          "preview rejects missing net instead of inventing one");
  require(reason == "missing_net", "missing net has stable reason");
}
