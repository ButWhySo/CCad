#pragma once

#include "ccad_core/symbol.hpp"
#include "ccad_core/model.hpp"
#include "ccad_gui/library_browser_dialog.hpp"

#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QString>

#include <optional>
#include <string>

struct SymbolPlacementResult {
  ccad::Symbol symbol;
  std::string symbol_path;
  std::string symbol_library_name;
  std::string symbol_item_name;
  std::string symbol_source_kind;
  std::string component_id;
  std::string layer_id;
  double x_mm = 0.0;
  double y_mm = 0.0;
  double rotation_deg = 0.0;
};

class SymbolPlacementDialog : public QDialog {
  Q_OBJECT

 public:
  explicit SymbolPlacementDialog(const ccad::Project& board, QWidget* parent = nullptr);

  std::optional<SymbolPlacementResult> result() const;

 private slots:
  void browseSymbol();
  void onAccept();

 private:
  QPushButton* browse_button_ = nullptr;
  QLineEdit* symbol_path_edit_ = nullptr;
  QDoubleSpinBox* x_spin_ = nullptr;
  QDoubleSpinBox* y_spin_ = nullptr;
  QDoubleSpinBox* rotation_spin_ = nullptr;

  std::optional<LibrarySelection> selected_symbol_;
  std::optional<SymbolPlacementResult> result_;
  const ccad::Project& project_;
};
