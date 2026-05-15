#include "diagnostics_panel.hpp"

#include <QAbstractItemView>
#include <QColor>
#include <QHeaderView>
#include <QTableWidgetItem>

namespace {

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QTableWidgetItem* makeItem(const QString& text) {
  auto* item = new QTableWidgetItem(text);
  item->setFlags(item->flags() & ~Qt::ItemIsEditable);
  return item;
}

}  // namespace

DiagnosticsPanel::DiagnosticsPanel(QWidget* parent) : QTableWidget(parent) {
  setObjectName("diagnosticsTable");
  setColumnCount(4);
  setHorizontalHeaderLabels({"Severity", "Code", "Object", "Message"});
  horizontalHeader()->setStretchLastSection(true);
  horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setAlternatingRowColors(true);
  verticalHeader()->setVisible(false);
  setShowGrid(false);
}

void DiagnosticsPanel::renderDiagnostics(const std::vector<ccad::Diagnostic>& diagnostics) {
  setRowCount(static_cast<int>(diagnostics.size()));
  for (int row = 0; row < static_cast<int>(diagnostics.size()); ++row) {
    const ccad::Diagnostic& diagnostic = diagnostics.at(static_cast<std::size_t>(row));
    auto* severity = makeItem(qstr(diagnostic.severity));
    if (diagnostic.severity == "error") {
      severity->setBackground(QColor("#fee2e2"));
      severity->setForeground(QColor("#991b1b"));
    } else if (diagnostic.severity == "warning") {
      severity->setBackground(QColor("#fef3c7"));
      severity->setForeground(QColor("#92400e"));
    }
    setItem(row, 0, severity);
    setItem(row, 1, makeItem(qstr(diagnostic.code)));
    setItem(row, 2, makeItem(qstr(diagnostic.object_id)));
    setItem(row, 3, makeItem(qstr(diagnostic.message)));
  }
  resizeColumnsToContents();
}
