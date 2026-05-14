#include "ccad_core/review.hpp"

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
  review.diagnostics = runErc(project);

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

