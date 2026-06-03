#include "ccad_gui/footprint_placement_dialog.hpp"

#include "ccad_core/kicad_footprint_import.hpp"

#include "ccad_gui/library_browser_dialog.hpp"
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include <fstream>
#include <sstream>

FootprintPlacementDialog::FootprintPlacementDialog(const ccad::Board& board, QWidget* parent)
    : QDialog(parent), board_(board) {
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

  layer_combo_ = new QComboBox(this);
  populateLayers(board);
  form->addRow("Layer:", layer_combo_);

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
  LibraryBrowserDialog dialog(LibraryType::Footprint, this);
  if (dialog.exec() == QDialog::Accepted) {
    selected_footprint_.reset();
    if (auto selection = dialog.selection()) {
      selected_footprint_ = *selection;
      footprint_path_edit_->setText(QString::fromStdString(selection->path));
    } else if (auto result = dialog.result()) {
      footprint_path_edit_->setText(QString::fromStdString(*result));
    }
  }
}

void FootprintPlacementDialog::onAccept() {
  const QString fp_path = footprint_path_edit_->text();
  if (fp_path.isEmpty()) {
    QMessageBox::warning(this, "Missing Footprint", "Please select a footprint file.");
    return;
  }

  const std::string selected_path = fp_path.toStdString();
  const LibrarySelection* selection =
      (selected_footprint_.has_value() && selected_footprint_->path == selected_path) ? &*selected_footprint_
                                                                                      : nullptr;
  QFileInfo fi(fp_path);
  QString baseName = fi.baseName();
  std::string prefix = "U";
  if (baseName.startsWith("R_") || baseName.startsWith("R", Qt::CaseInsensitive)) prefix = "R";
  else if (baseName.startsWith("C_") || baseName.startsWith("C", Qt::CaseInsensitive)) prefix = "C";
  else if (baseName.startsWith("D_") || baseName.startsWith("D", Qt::CaseInsensitive)) prefix = "D";
  else if (baseName.startsWith("Q_") || baseName.startsWith("Q", Qt::CaseInsensitive)) prefix = "Q";
  else if (baseName.startsWith("L_") || baseName.startsWith("L", Qt::CaseInsensitive)) prefix = "L";

  int max_num = 0;
  for (const auto& pad : board_.pads) {
    if (pad.component_id.starts_with(prefix)) {
      std::string num_str = pad.component_id.substr(prefix.length());
      try {
        int num = std::stoi(num_str);
        if (num > max_num) max_num = num;
      } catch (...) {}
    }
  }
  const std::string comp_id = prefix + std::to_string(max_num + 1);

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
    ccad::Footprint footprint = fi.suffix().compare("kicad_mod", Qt::CaseInsensitive) == 0
                                     ? ccad::importKiCadFootprint(buffer.str())
                                     : ccad::loadFootprintJson(buffer.str());
    if (footprint.pads.empty()) {
      QMessageBox::warning(this, "Invalid Footprint", "The footprint has no pads.");
      return;
    }

    result_ = FootprintPlacementResult{
        .footprint = std::move(footprint),
        .footprint_path = selected_path,
        .footprint_library_name = selection != nullptr ? selection->library_name : std::string{},
        .footprint_item_name = selection != nullptr ? selection->item_name : baseName.toStdString(),
        .footprint_source_kind = selection != nullptr ? selection->source_kind : std::string{},
        .component_id = comp_id,
        .layer_id = layer_id.toStdString(),
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
