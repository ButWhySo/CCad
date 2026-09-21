#include "ccad_core/agent_preview.hpp"

#include "ccad_core/router_tool.hpp"

namespace ccad {

namespace {

void setReason(std::string* reason, const std::string& value) {
  if (reason != nullptr) {
    *reason = value;
  }
}

}  // namespace

std::optional<StagedProjectPreview> stageRouteTrackPreview(
    const Project& project, const RouteTrackPreviewRequest& request,
    std::string* reason) {
  if (project.boards.empty()) {
    setReason(reason, "missing_board");
    return std::nullopt;
  }
  if (request.net_id.empty()) {
    setReason(reason, "missing_net");
    return std::nullopt;
  }
  if (request.layer_id != "F.Cu" && request.layer_id != "B.Cu") {
    setReason(reason, "unsupported_layer");
    return std::nullopt;
  }

  StagedProjectPreview preview;
  preview.before = project;
  preview.after = project;
  Board& staged_board = preview.after.boards.front();
  const std::size_t previous_track_count = staged_board.tracks.size();

  RouterTool router;
  router.setBoard(&staged_board);
  router.setActiveNet(request.net_id);
  router.routeTrack(request.start_x_mm, request.start_y_mm, request.end_x_mm,
                    request.end_y_mm, request.layer_id == "B.Cu" ? 1 : 0);

  if (staged_board.tracks.size() == previous_track_count) {
    setReason(reason, router.routeBlocked()
                          ? "blocked_obstacle_" + router.blockedReason()
                          : "zero_length");
    return std::nullopt;
  }

  for (std::size_t index = previous_track_count;
       index < staged_board.tracks.size(); ++index) {
    TrackSegment& track = staged_board.tracks[index];
    track.net_id = request.net_id;
    track.layer_id = request.layer_id;
    preview.focus_object_id = track.id;
  }
  preview.diff = diffProjects(preview.before, preview.after);
  preview.drc_diagnostics = runDrc(preview.after);
  setReason(reason, "staged");
  return preview;
}

}  // namespace ccad
