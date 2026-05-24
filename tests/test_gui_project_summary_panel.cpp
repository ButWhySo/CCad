#include "ccad_gui/project_summary_panel.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QLabel>

namespace {

ccad::ProjectReview baseReview() {
  ccad::ProjectReview review;
  review.project_id = "proj-demo";
  review.project_name = "demo";
  review.has_board = true;
  review.board_width_nm = 44000000;
  review.board_height_nm = 30000000;
  review.layer_count = 2;
  return review;
}

QString subtitleText(ProjectSummaryPanel& panel) {
  auto* subtitle = panel.findChild<QLabel*>("subtitle");
  require(subtitle != nullptr, "summary subtitle is discoverable");
  return subtitle->text();
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  ProjectSummaryPanel panel;
  auto* title = panel.findChild<QLabel*>("title");
  require(title != nullptr, "summary title is discoverable");
  require(title->wordWrap(), "summary title wraps inside the dock");
  auto* subtitle = panel.findChild<QLabel*>("subtitle");
  require(subtitle != nullptr, "summary subtitle is discoverable");
  require(subtitle->wordWrap(), "summary subtitle wraps inside the dock");

  ccad::ProjectReview shifted = baseReview();
  shifted.board_origin_x_nm = 2000000;
  shifted.board_origin_y_nm = 3000000;
  panel.renderReview(shifted);
  const QString shifted_subtitle = subtitleText(panel);
  require(shifted_subtitle.contains("Project ID: proj-demo"), "subtitle contains project id");
  require(shifted_subtitle.contains("origin 2.00 mm, 3.00 mm"),
          "non-zero board origin is visible");
  require(shifted_subtitle.contains("44.00 mm x 30.00 mm"), "board size remains visible");

  ccad::ProjectReview origin_zero = baseReview();
  panel.renderReview(origin_zero);
  require(subtitleText(panel) == "Project ID: proj-demo   Board: 44.00 mm x 30.00 mm",
          "zero-origin board keeps compact summary");
}
