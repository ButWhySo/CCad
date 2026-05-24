#include "ccad_core/review.hpp"

#include "ccad_core/drc.hpp"

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
    review.board_width_nm = project.board->outline.size.width.nanometers;
    review.board_height_nm = project.board->outline.size.height.nanometers;
    review.layer_count = project.board->layers.size();
    review.pad_count = project.board->pads.size();
    review.via_count = project.board->vias.size();
    review.track_count = project.board->tracks.size();
    review.keepout_count = project.board->keepouts.size();
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

