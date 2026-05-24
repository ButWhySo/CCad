#include "project_summary_panel.hpp"

#include <QFrame>
#include <QVBoxLayout>

namespace {

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

}  // namespace

ProjectSummaryPanel::ProjectSummaryPanel(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(14, 12, 14, 12);
  layout->setSpacing(10);

  title_ = new QLabel("No project loaded", this);
  title_->setObjectName("title");
  subtitle_ = new QLabel("Open a .ccad.json project to review board state.", this);
  subtitle_->setObjectName("subtitle");
  status_chip_ = new QLabel("Ready", this);
  status_chip_->setObjectName("statusChip");
  status_chip_->setAlignment(Qt::AlignCenter);

  layout->addWidget(title_);
  layout->addWidget(subtitle_);
  layout->addWidget(status_chip_);
  layout->addWidget(makeValueCard("Components"));
  layout->addWidget(makeValueCard("Nets"));
  layout->addWidget(makeValueCard("Layers"));
  layout->addWidget(makeValueCard("Pads"));
  layout->addWidget(makeValueCard("Vias"));
  layout->addWidget(makeValueCard("Tracks"));
  layout->addWidget(makeValueCard("Placement Regions"));
  layout->addWidget(makeValueCard("Keepouts"));
  layout->addWidget(makeValueCard("Diagnostics"));
  layout->addStretch(1);
}

void ProjectSummaryPanel::renderReview(const ccad::ProjectReview& review) {
  title_->setText(qstr(review.project_name));
  QString board_text = "No board";
  if (review.has_board) {
    board_text = "Board: " + QString::number(review.board_width_nm / 1000000.0, 'f', 2) +
                 " mm x " + QString::number(review.board_height_nm / 1000000.0, 'f', 2) + " mm";
  }
  subtitle_->setText("Project ID: " + qstr(review.project_id) + "   " + board_text);
  components_value_->setText(QString::number(review.component_count));
  nets_value_->setText(QString::number(review.net_count));
  layers_value_->setText(QString::number(review.layer_count));
  pads_value_->setText(QString::number(review.pad_count));
  vias_value_->setText(QString::number(review.via_count));
  tracks_value_->setText(QString::number(review.track_count));
  placement_regions_value_->setText(QString::number(review.placement_region_count));
  keepouts_value_->setText(QString::number(review.keepout_count));
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
  pads_value_->setText("0");
  vias_value_->setText("0");
  tracks_value_->setText("0");
  placement_regions_value_->setText("0");
  keepouts_value_->setText("0");
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
  } else if (label == "Pads") {
    pads_value_ = value;
  } else if (label == "Vias") {
    vias_value_ = value;
  } else if (label == "Tracks") {
    tracks_value_ = value;
  } else if (label == "Placement Regions") {
    placement_regions_value_ = value;
  } else if (label == "Keepouts") {
    keepouts_value_ = value;
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
