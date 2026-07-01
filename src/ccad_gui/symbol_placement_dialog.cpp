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
#include <algorithm>

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
    selected_symbol_.reset();
    if (auto selection = dialog.selection()) {
      selected_symbol_ = *selection;
      symbol_path_edit_->setText(QString::fromStdString(selection->path));
    } else if (auto result = dialog.result()) {
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

  const std::string selected_path = fp_path.toStdString();
  const LibrarySelection* selection =
      (selected_symbol_.has_value() && selected_symbol_->path == selected_path) ? &*selected_symbol_ : nullptr;
  QFileInfo fi(fp_path);
  QString baseName = fi.baseName();
  std::string prefix = "U";
  if (baseName.startsWith("R_") || baseName.startsWith("R", Qt::CaseInsensitive)) prefix = "R";
  else if (baseName.startsWith("C_") || baseName.startsWith("C", Qt::CaseInsensitive)) prefix = "C";
  else if (baseName.startsWith("D_") || baseName.startsWith("D", Qt::CaseInsensitive)) prefix = "D";
  else if (baseName.startsWith("Q_") || baseName.startsWith("Q", Qt::CaseInsensitive)) prefix = "Q";
  else if (baseName.startsWith("L_") || baseName.startsWith("L", Qt::CaseInsensitive)) prefix = "L";

  int max_num = 0;
  for (const auto& comp : project_.schematics[0].symbols) {
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
    ccad::Symbol symbol;
    if (fi.suffix().compare("kicad_sym", Qt::CaseInsensitive) == 0) {
      const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(buffer.str());
      const std::string requested_name = selection != nullptr ? selection->item_name : std::string{};
      auto selected = requested_name.empty()
                          ? symbols.end()
                          : std::find_if(symbols.begin(), symbols.end(), [&](const ccad::Symbol& candidate) {
                              return candidate.name == requested_name;
                            });
      if (selected == symbols.end()) {
        selected = std::find_if(symbols.begin(), symbols.end(), [](const ccad::Symbol& candidate) {
          return !candidate.pins.empty();
        });
      }
      if (selected == symbols.end() && !symbols.empty()) {
        selected = symbols.begin();
      }
      if (selected == symbols.end()) {
        QMessageBox::warning(this, "Invalid Symbol", "The KiCad symbol library has no symbols.");
        return;
      }
      symbol = *selected;
    } else {
      symbol = ccad::loadSymbolJsonFileWithLocalInheritance(fi.absoluteFilePath().toStdString());
    }
    if (symbol.pins.empty()) {
      QMessageBox::warning(this, "Invalid Symbol", "The symbol has no pins.");
      return;
    }

    result_ = SymbolPlacementResult{
        .symbol = std::move(symbol),
        .symbol_path = selected_path,
        .symbol_library_name = selection != nullptr ? selection->library_name : std::string{},
        .symbol_item_name = selection != nullptr ? selection->item_name : baseName.toStdString(),
        .symbol_source_kind = selection != nullptr ? selection->source_kind : std::string{},
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
