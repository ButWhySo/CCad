#pragma once

#include "ccad_core/diff.hpp"
#include "ccad_core/drc.hpp"
#include "ccad_core/model.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ccad {

// A typed, non-mutating rendering candidate for an approval-gated agent action.
// The caller owns the source project; staging always works on a value copy.
struct RouteTrackPreviewRequest {
  double start_x_mm = 0.0;
  double start_y_mm = 0.0;
  double end_x_mm = 0.0;
  double end_y_mm = 0.0;
  std::string net_id;
  std::string layer_id = "F.Cu";
};

struct RectanglePreviewRequest {
  double start_x_mm = 0.0;
  double start_y_mm = 0.0;
  double end_x_mm = 0.0;
  double end_y_mm = 0.0;
  std::string net_id;
  std::string layer_id = "F.Cu";
};

struct GraphicPreviewRequest {
  double start_x_mm = 0.0;
  double start_y_mm = 0.0;
  double end_x_mm = 0.0;
  double end_y_mm = 0.0;
  std::string layer_id = "F.Cu";
};

struct StagedProjectPreview {
  Project before;
  Project after;
  ProjectDiff diff;
  std::vector<Diagnostic> drc_diagnostics;
  std::string focus_object_id;
};

// Stages the same RouterTool operation used by the native route workflow.
// Returns nullopt with a stable reason when the request cannot produce a real
// candidate. It never changes `project`.
std::optional<StagedProjectPreview> stageRouteTrackPreview(
    const Project& project, const RouteTrackPreviewRequest& request,
    std::string* reason = nullptr);

std::optional<StagedProjectPreview> stageZonePreview(
    const Project& project, const RectanglePreviewRequest& request,
    std::string* reason = nullptr);

std::optional<StagedProjectPreview> stageKeepoutPreview(
    const Project& project, const RectanglePreviewRequest& request,
    std::string* reason = nullptr);

std::optional<StagedProjectPreview> stageGraphicLinePreview(
    const Project& project, const GraphicPreviewRequest& request,
    std::string* reason = nullptr);

}  // namespace ccad
