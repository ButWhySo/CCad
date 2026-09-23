#include "ccad_core/agent_preview.hpp"

#include "ccad_core/router_tool.hpp"

namespace ccad {

namespace {

void setReason(std::string* reason, const std::string& value) {
  if (reason != nullptr) {
    *reason = value;
  }
}

bool containsPoint(const Board& board, const Point& point) {
  const std::int64_t min_x = board.outline.origin.x.nanometers;
  const std::int64_t min_y = board.outline.origin.y.nanometers;
  const std::int64_t max_x = min_x + board.outline.size.width.nanometers;
  const std::int64_t max_y = min_y + board.outline.size.height.nanometers;
  return point.x.nanometers >= min_x && point.x.nanometers <= max_x &&
         point.y.nanometers >= min_y && point.y.nanometers <= max_y;
}

std::optional<StagedProjectPreview> beginPreview(const Project& project,
                                                 std::string* reason) {
  if (project.boards.empty()) {
    setReason(reason, "missing_board");
    return std::nullopt;
  }
  StagedProjectPreview preview;
  preview.before = project;
  preview.after = project;
  return preview;
}

void finishPreview(StagedProjectPreview& preview, std::string* reason) {
  preview.diff = diffProjects(preview.before, preview.after);
  preview.drc_diagnostics = runDrc(preview.after);
  setReason(reason, "staged");
}

bool validRectangle(const RectanglePreviewRequest& request) {
  return request.start_x_mm != request.end_x_mm && request.start_y_mm != request.end_y_mm;
}

std::vector<Point> rectangleOutline(const RectanglePreviewRequest& request) {
  const Length min_x = millimeters(std::min(request.start_x_mm, request.end_x_mm));
  const Length min_y = millimeters(std::min(request.start_y_mm, request.end_y_mm));
  const Length max_x = millimeters(std::max(request.start_x_mm, request.end_x_mm));
  const Length max_y = millimeters(std::max(request.start_y_mm, request.end_y_mm));
  return {{min_x, min_y}, {max_x, min_y}, {max_x, max_y}, {min_x, max_y}};
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

std::optional<StagedProjectPreview> stageZonePreview(
    const Project& project, const RectanglePreviewRequest& request,
    std::string* reason) {
  auto preview = beginPreview(project, reason);
  if (!preview.has_value()) return std::nullopt;
  if (!validRectangle(request)) {
    setReason(reason, "zero_area");
    return std::nullopt;
  }
  Board& board = preview->after.boards.front();
  if (request.layer_id.empty()) {
    setReason(reason, "missing_layer");
    return std::nullopt;
  }
  BoardZone zone;
  zone.id = "preview-zone-" + std::to_string(board.zones.size() + 1);
  zone.name = "Agent preview zone";
  zone.net_id = request.net_id;
  zone.layer_ids = {request.layer_id};
  zone.outline = rectangleOutline(request);
  zone.clearance = board.design_rules.copper_clearance;
  zone.min_thickness = board.design_rules.min_track_width;
  preview->focus_object_id = zone.id;
  board.zones.push_back(std::move(zone));
  finishPreview(*preview, reason);
  return preview;
}

std::optional<StagedProjectPreview> stageKeepoutPreview(
    const Project& project, const RectanglePreviewRequest& request,
    std::string* reason) {
  auto preview = beginPreview(project, reason);
  if (!preview.has_value()) return std::nullopt;
  if (!validRectangle(request)) {
    setReason(reason, "zero_area");
    return std::nullopt;
  }
  Board& board = preview->after.boards.front();
  Keepout keepout;
  keepout.id = "preview-keepout-" + std::to_string(board.keepouts.size() + 1);
  keepout.kind = "routing";
  keepout.area.origin = {millimeters(std::min(request.start_x_mm, request.end_x_mm)),
                         millimeters(std::min(request.start_y_mm, request.end_y_mm))};
  keepout.area.size = {millimeters(std::abs(request.end_x_mm - request.start_x_mm)),
                       millimeters(std::abs(request.end_y_mm - request.start_y_mm))};
  preview->focus_object_id = keepout.id;
  board.keepouts.push_back(std::move(keepout));
  finishPreview(*preview, reason);
  return preview;
}

std::optional<StagedProjectPreview> stageGraphicLinePreview(
    const Project& project, const GraphicPreviewRequest& request,
    std::string* reason) {
  auto preview = beginPreview(project, reason);
  if (!preview.has_value()) return std::nullopt;
  Board& board = preview->after.boards.front();
  const Point start{millimeters(request.start_x_mm), millimeters(request.start_y_mm)};
  const Point end{millimeters(request.end_x_mm), millimeters(request.end_y_mm)};
  if (start.x.nanometers == end.x.nanometers && start.y.nanometers == end.y.nanometers) {
    setReason(reason, "zero_length");
    return std::nullopt;
  }
  if (!containsPoint(board, start) || !containsPoint(board, end)) {
    setReason(reason, "outside_board_outline");
    return std::nullopt;
  }
  BoardGraphic graphic;
  graphic.id = "preview-graphic-" + std::to_string(board.graphics.size() + 1);
  graphic.kind = "line";
  graphic.layer_id = request.layer_id;
  graphic.start = start;
  graphic.end = end;
  graphic.width = board.design_rules.min_track_width;
  preview->focus_object_id = graphic.id;
  board.graphics.push_back(std::move(graphic));
  finishPreview(*preview, reason);
  return preview;
}

}  // namespace ccad
