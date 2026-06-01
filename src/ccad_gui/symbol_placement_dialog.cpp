#include "ccad_gui/symbol_placement_dialog.hpp"

#include "ccad_core/kicad_symbol_import.hpp"

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

SymbolPlacementDialog::SymbolPlacementDialog(const ccad::Project& board, QWidget* parent)
    : QDialog(parent), project_(board) {
  setWindowTitle("Place Symbol");
  setMinimumWidth(420);

  auto* main_layout = new QVBoxLayout(this);

  auto* header = new QLabel("<h3>Place Symbol on Board</h3>");
  main_layout->addWidget(header);

  // Symbol file selection
  auto* file_group = new QGroupBox("Symbol File");
  auto* file_layout = new QHBoxLayout(file_group);
  symbol_path_edit_ = new QLineEdit(this);
  symbol_path_edit_->setPlaceholderText("Select a .ccad-symbol.json file...");
  symbol_path_edit_->setReadOnly(true);
  browse_button_ = new QPushButton("Browse...", this);
  file_layout->addWidget(symbol_path_edit_, 1);
  file_layout->addWidget(browse_button_);
  main_layout->addWidget(file_group);

  // Placement parameters
  auto* params_group = new QGroupBox("Placement Parameters");
  auto* form = new QFormLayout(params_group);


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

  connect(browse_button_, &QPushButton::clicked, this, &SymbolPlacementDialog::browseSymbol);
  connect(button_box, &QDialogButtonBox::accepted, this, &SymbolPlacementDialog::onAccept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
}


void SymbolPlacementDialog::browseSymbol() {
  LibraryBrowserDialog dialog(LibraryType::Symbol, this);
  if (dialog.exec() == QDialog::Accepted) {
    if (auto result = dialog.result()) {
      symbol_path_edit_->setText(QString::fromStdString(*result));
    }
  }
}

void SymbolPlacementDialog::onAccept() {
  const QString fp_path = symbol_path_edit_->text();
  if (fp_path.isEmpty()) {
    QMessageBox::warning(this, "Missing Symbol", "Please select a symbol file.");
    return;
  }

  QFileInfo fi(fp_path);
  QString baseName = fi.baseName();
  std::string prefix = "U";
  if (baseName.startsWith("R_") || baseName.startsWith("R", Qt::CaseInsensitive)) prefix = "R";
  else if (baseName.startsWith("C_") || baseName.startsWith("C", Qt::CaseInsensitive)) prefix = "C";
  else if (baseName.startsWith("D_") || baseName.startsWith("D", Qt::CaseInsensitive)) prefix = "D";
  else if (baseName.startsWith("Q_") || baseName.startsWith("Q", Qt::CaseInsensitive)) prefix = "Q";
  else if (baseName.startsWith("L_") || baseName.startsWith("L", Qt::CaseInsensitive)) prefix = "L";

  int max_num = 0;
  for (const auto& comp : project_.components) {
    if (comp.id.starts_with(prefix)) {
      std::string num_str = comp.id.substr(prefix.length());
      try {
        int num = std::stoi(num_str);
        if (num > max_num) max_num = num;
      } catch (...) {}
    }
  }
  const std::string comp_id = prefix + std::to_string(max_num + 1);


  // Load the symbol
  std::ifstream input(fp_path.toStdString());
  if (!input) {
    QMessageBox::critical(this, "File Error",
                          "Failed to open symbol file:\n" + fp_path);
    return;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();

  try {
    ccad::Symbol symbol = ccad::loadSymbolJson(buffer.str());
    if (symbol.pins.empty()) {
      QMessageBox::warning(this, "Invalid Symbol", "The symbol has no pins.");
      return;
    }

    result_ = SymbolPlacementResult{
        .symbol = std::move(symbol),
        .symbol_path = fp_path.toStdString(),
        .component_id = comp_id,
        .layer_id = "",
        .x_mm = x_spin_->value(),
        .y_mm = y_spin_->value(),
        .rotation_deg = rotation_spin_->value(),
    };
    accept();
  } catch (const std::exception& e) {
    QMessageBox::critical(this, "Parse Error",
                          QString("Failed to parse symbol:\n") + e.what());
  }
}

std::optional<SymbolPlacementResult> SymbolPlacementDialog::result() const {
  return result_;
}
