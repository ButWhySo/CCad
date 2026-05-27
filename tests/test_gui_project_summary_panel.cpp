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
  review.route_request_count = 2;
  review.open_route_count = 1;
  review.partial_route_count = 1;
  review.completed_route_count = 3;
  review.routed_segment_count = 4;
  return review;
}

QString subtitleText(ProjectSummaryPanel& panel) {
  auto* subtitle = panel.findChild<QLabel*>("subtitle");
  require(subtitle != nullptr, "summary subtitle is discoverable");
  return subtitle->text();
}

bool hasLabelText(ProjectSummaryPanel& panel, const QString& expected) {
  const QList<QLabel*> labels = panel.findChildren<QLabel*>();
  for (const QLabel* label : labels) {
    if (label->text() == expected) {
      return true;
    }
  }
  return false;
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
  shifted.copper_clearance_nm = 250000;
  shifted.min_track_width_nm = 180000;
  shifted.min_via_annular_ring_nm = 110000;
  panel.renderReview(shifted);
  const QString shifted_subtitle = subtitleText(panel);
  require(shifted_subtitle.contains("Project ID: proj-demo"), "subtitle contains project id");
  require(shifted_subtitle.contains("origin 2.00 mm, 3.00 mm"),
          "non-zero board origin is visible");
  require(shifted_subtitle.contains("44.00 mm x 30.00 mm"), "board size remains visible");
  require(hasLabelText(panel, "Copper Clearance"), "summary labels copper clearance");
  require(hasLabelText(panel, "0.25 mm"), "summary shows copper clearance value");
  require(hasLabelText(panel, "Min Track Width"), "summary labels minimum track width");
  require(hasLabelText(panel, "0.18 mm"), "summary shows minimum track width value");
  require(hasLabelText(panel, "Via Annular Ring"), "summary labels via annular ring");
  require(hasLabelText(panel, "0.11 mm"), "summary shows via annular ring value");
  require(hasLabelText(panel, "Route Requests"), "summary labels route requests");
  require(hasLabelText(panel, "2"), "summary shows route request count");
  require(hasLabelText(panel, "Route Progress"), "summary labels route progress");
  require(hasLabelText(panel, "open 1 / partial 1 / done 3"),
          "summary shows route progress counts");
  require(hasLabelText(panel, "Routed Segments"), "summary labels routed segments");
  require(hasLabelText(panel, "4"), "summary shows routed segment count");

  ccad::ProjectReview origin_zero = baseReview();
  panel.renderReview(origin_zero);
  require(subtitleText(panel) == "Project ID: proj-demo   Board: 44.00 mm x 30.00 mm",
          "zero-origin board keeps compact summary");
}
