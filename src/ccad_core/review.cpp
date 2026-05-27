#include "ccad_core/review.hpp"

#include "ccad_core/drc.hpp"

#include <map>
#include <string>

namespace ccad {
namespace {

std::string plural(const std::size_t count, const std::string& singular) {
  if (count == 1) {
    return "1 " + singular;
  }
  return std::to_string(count) + " " + singular + "s";
}

}  // namespace

ProjectReview buildReview(const Project& project) {
  ProjectReview review;
  review.project_id = project.id;
  review.project_name = project.name;
  review.component_count = project.components.size();
  review.net_count = project.nets.size();
  review.constraint_count = project.constraints.size();
  if (project.board.has_value()) {
    review.has_board = true;
    review.board_origin_x_nm = project.board->outline.origin.x.nanometers;
    review.board_origin_y_nm = project.board->outline.origin.y.nanometers;
    review.board_width_nm = project.board->outline.size.width.nanometers;
    review.board_height_nm = project.board->outline.size.height.nanometers;
    review.copper_clearance_nm = project.board->design_rules.copper_clearance.nanometers;
    review.min_track_width_nm = project.board->design_rules.min_track_width.nanometers;
    review.min_via_annular_ring_nm = project.board->design_rules.min_via_annular_ring.nanometers;
    review.layer_count = project.board->layers.size();
    for (const Layer& layer : project.board->layers) {
      if (layer.kind == "copper") {
        ++review.copper_layer_count;
      } else {
        ++review.non_copper_layer_count;
      }
      if (layer.visible) {
        ++review.visible_layer_count;
      } else {
        ++review.hidden_layer_count;
      }
    }
    review.pad_count = project.board->pads.size();
    review.via_count = project.board->vias.size();
    review.track_count = project.board->tracks.size();
    review.placement_region_count = project.board->placement_regions.size();
    review.keepout_count = project.board->keepouts.size();
    review.route_request_count = project.board->route_requests.size();
    std::map<std::string, std::size_t> routed_segment_counts;
    for (const TrackSegment& track : project.board->tracks) {
      if (!track.source_route_request_id.empty()) {
        ++review.routed_segment_count;
        ++routed_segment_counts[track.source_route_request_id];
      }
    }
    for (const RouteRequest& request : project.board->route_requests) {
      if (routed_segment_counts[request.id] > 0) {
        ++review.partial_route_count;
      } else {
        ++review.open_route_count;
      }
    }
    for (const auto& [request_id, segment_count] : routed_segment_counts) {
      bool still_open = false;
      for (const RouteRequest& request : project.board->route_requests) {
        if (request.id == request_id) {
          still_open = true;
          break;
        }
      }
      if (!still_open && segment_count > 0) {
        ++review.completed_route_count;
      }
    }
  }
  review.diagnostics = runErc(project);
  const std::vector<Diagnostic> drc_diagnostics = runDrc(project);
  review.diagnostics.insert(review.diagnostics.end(), drc_diagnostics.begin(),
                            drc_diagnostics.end());

  for (const Diagnostic& diagnostic : review.diagnostics) {
    if (diagnostic.severity == "error") {
      ++review.error_count;
    } else if (diagnostic.severity == "warning") {
      ++review.warning_count;
    }
  }

  if (review.error_count > 0) {
    review.status = "Errors: " + std::to_string(review.error_count) +
                    ", warnings: " + std::to_string(review.warning_count);
  } else if (review.warning_count > 0) {
    review.status = "Warnings: " + std::to_string(review.warning_count);
  } else {
    review.status = "Clean: " + plural(review.component_count, "component") + ", " +
                    plural(review.net_count, "net") + ", " +
                    plural(review.constraint_count, "constraint");
  }

  return review;
}

}  // namespace ccad

