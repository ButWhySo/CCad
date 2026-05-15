#pragma once

#include "ccad_core/transaction.hpp"

#include <QTableWidget>

#include <vector>

class TransactionTimelinePanel final : public QTableWidget {
 public:
  explicit TransactionTimelinePanel(QWidget* parent = nullptr);

  void renderTransactions(const std::vector<ccad::Transaction>& transactions);
  QString itemText(int row, int column) const;
  QString transactionIdForRow(int row) const;
};
