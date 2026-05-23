#include "transaction_timeline_panel.hpp"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableWidgetItem>

namespace {

constexpr int kTransactionIdRole = Qt::UserRole;

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QTableWidgetItem* makeItem(const QString& text) {
  auto* item = new QTableWidgetItem(text);
  item->setFlags(item->flags() & ~Qt::ItemIsEditable);
  return item;
}

QString diffSummary(const ccad::ProjectDiff& diff) {
  return "+" + QString::number(diff.added_count) + "  ~" +
         QString::number(diff.changed_count) + "  -" + QString::number(diff.removed_count);
}

}  // namespace

TransactionTimelinePanel::TransactionTimelinePanel(QWidget* parent) : QTableWidget(parent) {
  setObjectName("transactionTimelineTable");
  setColumnCount(4);
  setHorizontalHeaderLabels({"ID", "Command", "Summary", "Diff"});
  horizontalHeader()->setStretchLastSection(true);
  horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setAlternatingRowColors(true);
  verticalHeader()->setVisible(false);
  setShowGrid(false);
}

void TransactionTimelinePanel::renderTransactions(
    const std::vector<ccad::Transaction>& transactions) {
  if (transactions.empty()) {
    setRowCount(1);
    setItem(0, 0, makeItem("No transactions yet"));
    setItem(0, 1, makeItem(""));
    setItem(0, 2, makeItem(""));
    setItem(0, 3, makeItem(""));
    resizeColumnsToContents();
    return;
  }

  setRowCount(static_cast<int>(transactions.size()));
  for (int row = 0; row < static_cast<int>(transactions.size()); ++row) {
    const ccad::Transaction& transaction = transactions.at(static_cast<std::size_t>(row));
    auto* id_item = makeItem(qstr(transaction.id));
    id_item->setData(kTransactionIdRole, qstr(transaction.id));
    setItem(row, 0, id_item);
    setItem(row, 1, makeItem(qstr(transaction.command)));
    setItem(row, 2, makeItem(qstr(transaction.summary)));
    setItem(row, 3, makeItem(diffSummary(transaction.diff)));
  }
  resizeColumnsToContents();
}

QString TransactionTimelinePanel::itemText(const int row, const int column) const {
  const QTableWidgetItem* table_item = item(row, column);
  if (table_item == nullptr) {
    return {};
  }
  return table_item->text();
}

QString TransactionTimelinePanel::transactionIdForRow(const int row) const {
  const QTableWidgetItem* id_item = item(row, 0);
  if (id_item == nullptr) {
    return {};
  }
  return id_item->data(kTransactionIdRole).toString();
}
