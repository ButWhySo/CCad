#include "ccad_gui/footprint_placement_dialog.hpp"

#include "ccad_core/kicad_footprint_import.hpp"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include <fstream>
#include <sstream>

FootprintPlacementDialog::FootprintPlacementDialog(const ccad::Board& board, QWidget* parent)
    : QDialog(parent) {
  setWindowTitle("Place Footprint");
  setMinimumWidth(420);

  auto* main_layout = new QVBoxLayout(this);

  auto* header = new QLabel("<h3>Place Footprint on Board</h3>");
  main_layout->addWidget(header);

  // Footprint file selection
  auto* file_group = new QGroupBox("Footprint File");
  auto* file_layout = new QHBoxLayout(file_group);
  footprint_path_edit_ = new QLineEdit(this);
  footprint_path_edit_->setPlaceholderText("Select a .ccad-footprint.json file...");
  footprint_path_edit_->setReadOnly(true);
  browse_button_ = new QPushButton("Browse...", this);
  file_layout->addWidget(footprint_path_edit_, 1);
  file_layout->addWidget(browse_button_);
  main_layout->addWidget(file_group);

  // Placement parameters
  auto* params_group = new QGroupBox("Placement Parameters");
  auto* form = new QFormLayout(params_group);

  component_id_edit_ = new QLineEdit(this);
  component_id_edit_->setPlaceholderText("e.g. R1, U2, C3");
  form->addRow("Component ID:", component_id_edit_);

  layer_combo_ = new QComboBox(this);
  populateLayers(board);
  form->addRow("Layer:", layer_combo_);

  x_spin_ = new QDoubleSpinBox(this);
  x_spin_->setRange(0.001, 9999.0);
  x_spin_->setDecimals(3);
  x_spin_->setSuffix(" mm");
  x_spin_->setValue(10.0);
  form->addRow("X Position:", x_spin_);

  y_spin_ = new QDoubleSpinBox(this);
  y_spin_->setRange(0.001, 9999.0);
  y_spin_->setDecimals(3);
  y_spin_->setSuffix(" mm");
  y_spin_->setValue(10.0);
  form->addRow("Y Position:", y_spin_);

  rotation_spin_ = new QDoubleSpinBox(this);
  rotation_spin_->setRange(-360.0, 360.0);
  rotation_spin_->setDecimals(1);
  rotation_spin_->setSuffix(" °");
  rotation_spin_->setValue(0.0);
  form->addRow("Rotation:", rotation_spin_);

  main_layout->addWidget(params_group);

  // Buttons
  auto* button_box = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  main_layout->addWidget(button_box);

  connect(browse_button_, &QPushButton::clicked, this, &FootprintPlacementDialog::browseFootprint);
  connect(button_box, &QDialogButtonBox::accepted, this, &FootprintPlacementDialog::onAccept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void FootprintPlacementDialog::populateLayers(const ccad::Board& board) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.kind == "copper") {
      layer_combo_->addItem(
          QString::fromStdString(layer.id + " - " + layer.name),
          QString::fromStdString(layer.id));
    }
  }
  if (layer_combo_->count() == 0) {
    layer_combo_->addItem("(no copper layers)", QString());
  }
}

void FootprintPlacementDialog::browseFootprint() {
  const QString path = QFileDialog::getOpenFileName(
      this, "Select Footprint File", QString(),
      "CCad Footprint (*.ccad-footprint.json);;All Files (*)");
  if (!path.isEmpty()) {
    footprint_path_edit_->setText(path);
  }
}

void FootprintPlacementDialog::onAccept() {
  const QString fp_path = footprint_path_edit_->text();
  if (fp_path.isEmpty()) {
    QMessageBox::warning(this, "Missing Footprint", "Please select a footprint file.");
    return;
  }

  const QString comp_id = component_id_edit_->text().trimmed();
  if (comp_id.isEmpty()) {
    QMessageBox::warning(this, "Missing Component ID", "Please enter a component ID.");
    return;
  }

  const QString layer_id = layer_combo_->currentData().toString();
  if (layer_id.isEmpty()) {
    QMessageBox::warning(this, "No Copper Layer", "No copper layer is available for placement.");
    return;
  }

  // Load the footprint
  std::ifstream input(fp_path.toStdString());
  if (!input) {
    QMessageBox::critical(this, "File Error",
                          "Failed to open footprint file:\n" + fp_path);
    return;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();

  try {
    ccad::Footprint footprint = ccad::loadFootprintJson(buffer.str());
    if (footprint.pads.empty()) {
      QMessageBox::warning(this, "Invalid Footprint", "The footprint has no pads.");
      return;
    }

    result_ = FootprintPlacementResult{
        .footprint = std::move(footprint),
        .footprint_path = fp_path.toStdString(),
        .component_id = comp_id.toStdString(),
        .layer_id = layer_id.toStdString(),
        .x_mm = x_spin_->value(),
        .y_mm = y_spin_->value(),
        .rotation_deg = rotation_spin_->value(),
    };
    accept();
  } catch (const std::exception& e) {
    QMessageBox::critical(this, "Parse Error",
                          QString("Failed to parse footprint:\n") + e.what());
  }
}

std::optional<FootprintPlacementResult> FootprintPlacementDialog::result() const {
  return result_;
}
