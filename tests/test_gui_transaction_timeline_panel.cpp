#include "ccad_gui/transaction_timeline_panel.hpp"
#include "ccad_core/transaction.hpp"
#include "test_support.hpp"

#include <QApplication>

namespace {

ccad::Project baseProject() {
  ccad::Project project;
  project.id = "proj";
  project.name = "timeline";
  return project;
}

ccad::Transaction addComponentTransaction() {
  ccad::Project before = baseProject();
  ccad::Project after = before;
  after.components.push_back(ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "VDD", .kind = "power"}},
  });
  return ccad::buildTransaction("txn-001", "sch add-component U1", "Add MCU", before, after);
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  TransactionTimelinePanel panel;
  panel.renderTransactions({});
  require(panel.rowCount() == 1, "empty timeline has one status row");
  require(panel.itemText(0, 0) == "No transactions yet", "empty timeline status text");
  require(panel.transactionIdForRow(0).isEmpty(), "empty status row has no transaction id");

  panel.renderTransactions({addComponentTransaction()});
  require(panel.rowCount() == 1, "timeline has one transaction row");
  require(panel.itemText(0, 0) == "txn-001", "transaction id column");
  require(panel.itemText(0, 1) == "sch add-component U1", "transaction command column");
  require(panel.itemText(0, 2) == "Add MCU", "transaction summary column");
  require(panel.itemText(0, 3) == "+1  ~0  -0", "transaction diff summary column");
  require(panel.transactionIdForRow(0) == "txn-001", "transaction row exposes id");
  require(panel.transactionIdForRow(99).isEmpty(), "out of range row has no transaction id");
}
