#pragma once

#include "ccad_core/review.hpp"

#include <QLabel>
#include <QWidget>

class ProjectSummaryPanel final : public QWidget {
 public:
  explicit ProjectSummaryPanel(QWidget* parent = nullptr);

  void renderReview(const ccad::ProjectReview& review);
  void renderLoadFailure(const QString& path);

 private:
  QWidget* makeValueCard(const QString& label);
  void setStatusChip(const QString& text, const QString& color);

  QLabel* title_ = nullptr;
  QLabel* subtitle_ = nullptr;
  QLabel* status_chip_ = nullptr;
  QLabel* components_value_ = nullptr;
  QLabel* nets_value_ = nullptr;
  QLabel* layers_value_ = nullptr;
  QLabel* pads_value_ = nullptr;
  QLabel* vias_value_ = nullptr;
  QLabel* tracks_value_ = nullptr;
  QLabel* placement_regions_value_ = nullptr;
  QLabel* keepouts_value_ = nullptr;
  QLabel* diagnostics_value_ = nullptr;
};
