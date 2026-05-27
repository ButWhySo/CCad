#include "project_summary_panel.hpp"

#include <QFrame>
#include <QVBoxLayout>

namespace {

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QString nmToMmText(const std::int64_t value_nm) {
  return QString::number(value_nm / 1000000.0, 'f', 2);
}

QString nmToMmLabel(const std::int64_t value_nm) {
  return nmToMmText(value_nm) + " mm";
}

}  // namespace

ProjectSummaryPanel::ProjectSummaryPanel(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(14, 12, 14, 12);
  layout->setSpacing(10);

  title_ = new QLabel("No project loaded", this);
  title_->setObjectName("title");
  title_->setWordWrap(true);
  subtitle_ = new QLabel("Open a .ccad.json project to review board state.", this);
  subtitle_->setObjectName("subtitle");
  subtitle_->setWordWrap(true);
  status_chip_ = new QLabel("Ready", this);
  status_chip_->setObjectName("statusChip");
  status_chip_->setAlignment(Qt::AlignCenter);

  layout->addWidget(title_);
  layout->addWidget(subtitle_);
  layout->addWidget(status_chip_);
  layout->addWidget(makeValueCard("Copper Clearance"));
  layout->addWidget(makeValueCard("Min Track Width"));
  layout->addWidget(makeValueCard("Via Annular Ring"));
  layout->addWidget(makeValueCard("Layer Breakdown"));
  layout->addWidget(makeValueCard("Layer Visibility"));
  layout->addWidget(makeValueCard("Layers"));
  layout->addWidget(makeValueCard("Components"));
  layout->addWidget(makeValueCard("Nets"));
  layout->addWidget(makeValueCard("Pads"));
  layout->addWidget(makeValueCard("Vias"));
  layout->addWidget(makeValueCard("Tracks"));
  layout->addWidget(makeValueCard("Route Requests"));
  layout->addWidget(makeValueCard("Route Progress"));
  layout->addWidget(makeValueCard("Routed Segments"));
  layout->addWidget(makeValueCard("Placement Regions"));
  layout->addWidget(makeValueCard("Keepouts"));
  layout->addWidget(makeValueCard("Diagnostics"));
  layout->addStretch(1);
}

void ProjectSummaryPanel::renderReview(const ccad::ProjectReview& review) {
  title_->setText(qstr(review.project_name));
  QString board_text = "No board";
  if (review.has_board) {
    const QString board_size =
        nmToMmText(review.board_width_nm) + " mm x " + nmToMmText(review.board_height_nm) + " mm";
    board_text = "Board: " + board_size;
    if (review.board_origin_x_nm != 0 || review.board_origin_y_nm != 0) {
      board_text = "Board: origin " + nmToMmText(review.board_origin_x_nm) + " mm, " +
                   nmToMmText(review.board_origin_y_nm) + " mm; " + board_size;
    }
  }
  subtitle_->setText("Project ID: " + qstr(review.project_id) + "   " + board_text);
  components_value_->setText(QString::number(review.component_count));
  nets_value_->setText(QString::number(review.net_count));
  layers_value_->setText(QString::number(review.layer_count));
  layer_breakdown_value_->setText("copper " + QString::number(review.copper_layer_count) +
                                  " / other " +
                                  QString::number(review.non_copper_layer_count));
  layer_visibility_value_->setText("visible " + QString::number(review.visible_layer_count) +
                                   " / hidden " +
                                   QString::number(review.hidden_layer_count));
  pads_value_->setText(QString::number(review.pad_count));
  vias_value_->setText(QString::number(review.via_count));
  tracks_value_->setText(QString::number(review.track_count));
  route_requests_value_->setText(QString::number(review.route_request_count));
  route_progress_value_->setText("open " + QString::number(review.open_route_count) +
                                 " / partial " + QString::number(review.partial_route_count) +
                                 " / done " +
                                 QString::number(review.completed_route_count));
  routed_segments_value_->setText(QString::number(review.routed_segment_count));
  placement_regions_value_->setText(QString::number(review.placement_region_count));
  keepouts_value_->setText(QString::number(review.keepout_count));
  copper_clearance_value_->setText(nmToMmLabel(review.copper_clearance_nm));
  min_track_width_value_->setText(nmToMmLabel(review.min_track_width_nm));
  via_annular_ring_value_->setText(nmToMmLabel(review.min_via_annular_ring_nm));
  diagnostics_value_->setText(QString::number(review.diagnostics.size()));

  if (review.error_count > 0) {
    setStatusChip("Errors", "#dc2626");
  } else if (review.warning_count > 0) {
    setStatusChip("Warnings", "#d97706");
  } else {
    setStatusChip("Clean", "#059669");
  }
}

void ProjectSummaryPanel::renderLoadFailure(const QString& path) {
  title_->setText("Load failed");
  subtitle_->setText(path);
  components_value_->setText("0");
  nets_value_->setText("0");
  layers_value_->setText("0");
  layer_breakdown_value_->setText("copper 0 / other 0");
  layer_visibility_value_->setText("visible 0 / hidden 0");
  pads_value_->setText("0");
  vias_value_->setText("0");
  tracks_value_->setText("0");
  route_requests_value_->setText("0");
  route_progress_value_->setText("open 0 / partial 0 / done 0");
  routed_segments_value_->setText("0");
  placement_regions_value_->setText("0");
  keepouts_value_->setText("0");
  copper_clearance_value_->setText("0.00 mm");
  min_track_width_value_->setText("0.00 mm");
  via_annular_ring_value_->setText("0.00 mm");
  diagnostics_value_->setText("0");
  setStatusChip("Load failed", "#dc2626");
}

QWidget* ProjectSummaryPanel::makeValueCard(const QString& label) {
  auto* card = new QFrame(this);
  card->setObjectName("summaryCard");
  auto* layout = new QVBoxLayout(card);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(4);

  auto* title = new QLabel(label, card);
  title->setObjectName("cardTitle");
  auto* value = new QLabel("0", card);
  value->setObjectName("cardValue");

  layout->addWidget(title);
  layout->addWidget(value);

  if (label == "Components") {
    components_value_ = value;
  } else if (label == "Nets") {
    nets_value_ = value;
  } else if (label == "Layers") {
    layers_value_ = value;
  } else if (label == "Layer Breakdown") {
    layer_breakdown_value_ = value;
  } else if (label == "Layer Visibility") {
    layer_visibility_value_ = value;
  } else if (label == "Pads") {
    pads_value_ = value;
  } else if (label == "Vias") {
    vias_value_ = value;
  } else if (label == "Tracks") {
    tracks_value_ = value;
  } else if (label == "Route Requests") {
    route_requests_value_ = value;
  } else if (label == "Route Progress") {
    route_progress_value_ = value;
  } else if (label == "Routed Segments") {
    routed_segments_value_ = value;
  } else if (label == "Placement Regions") {
    placement_regions_value_ = value;
  } else if (label == "Keepouts") {
    keepouts_value_ = value;
  } else if (label == "Copper Clearance") {
    copper_clearance_value_ = value;
  } else if (label == "Min Track Width") {
    min_track_width_value_ = value;
  } else if (label == "Via Annular Ring") {
    via_annular_ring_value_ = value;
  } else {
    diagnostics_value_ = value;
  }
  return card;
}

void ProjectSummaryPanel::setStatusChip(const QString& text, const QString& color) {
  status_chip_->setText(text);
  status_chip_->setStyleSheet("color: #ffffff; background: " + color +
                              "; border-radius: 13px; padding: 6px 12px; font-weight: 700;");
}
