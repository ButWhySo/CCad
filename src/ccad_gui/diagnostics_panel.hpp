#pragma once

#include "ccad_core/erc.hpp"

#include <QTableWidget>
#include <vector>

class DiagnosticsPanel final : public QTableWidget {
 public:
  explicit DiagnosticsPanel(QWidget* parent = nullptr);

  void renderDiagnostics(const std::vector<ccad::Diagnostic>& diagnostics);
  QString objectIdForRow(int row) const;
};
