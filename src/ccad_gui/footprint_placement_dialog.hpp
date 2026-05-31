#pragma once

#include "ccad_core/footprint.hpp"
#include "ccad_core/model.hpp"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QString>

#include <optional>
#include <string>

struct FootprintPlacementResult {
  ccad::Footprint footprint;
  std::string footprint_path;
  std::string component_id;
  std::string layer_id;
  double x_mm = 0.0;
  double y_mm = 0.0;
  double rotation_deg = 0.0;
};

class FootprintPlacementDialog : public QDialog {
  Q_OBJECT

 public:
  explicit FootprintPlacementDialog(const ccad::Board& board, QWidget* parent = nullptr);

  std::optional<FootprintPlacementResult> result() const;

 private slots:
  void browseFootprint();
  void onAccept();

 private:
  void populateLayers(const ccad::Board& board);

  QPushButton* browse_button_ = nullptr;
  QLineEdit* footprint_path_edit_ = nullptr;
  QLineEdit* component_id_edit_ = nullptr;
  QComboBox* layer_combo_ = nullptr;
  QDoubleSpinBox* x_spin_ = nullptr;
  QDoubleSpinBox* y_spin_ = nullptr;
  QDoubleSpinBox* rotation_spin_ = nullptr;

  std::optional<FootprintPlacementResult> result_;
};
