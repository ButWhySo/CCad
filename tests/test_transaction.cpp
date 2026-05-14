#include "ccad_core/model.hpp"
#include "ccad_core/transaction.hpp"
#include "test_support.hpp"

namespace {

ccad::Project beforeProject() {
  ccad::Project project;
  project.id = "proj-before";
  project.name = "before";
  return project;
}

ccad::Project afterProject() {
  ccad::Project project = beforeProject();
  project.components.push_back(ccad::Component{
      .id = "U1",
      .part = "MCU",
      .pins = {ccad::Pin{.name = "VDD", .kind = "power"}},
  });
  return project;
}

}  // namespace

int main() {
  const ccad::Transaction transaction = ccad::buildTransaction(
      "txn-001", "sch add-component U1", "Add MCU component", beforeProject(), afterProject());

  require(transaction.id == "txn-001", "transaction id set");
  require(transaction.command == "sch add-component U1", "transaction command set");
  require(transaction.summary == "Add MCU component", "transaction summary set");
  require(transaction.before_project_id == "proj-before", "before project id set");
  require(transaction.after_project_id == "proj-before", "after project id set");
  require(transaction.diff.added_count == 1, "transaction diff has added count");
  require(transaction.diff.entries.size() == 1, "transaction diff carries entry");
  require(transaction.diff.entries.at(0).object_id == "U1", "transaction diff identifies object");

  const std::string json = ccad::dumpTransactionJson(transaction);
  require(json.find("\"id\": \"txn-001\"") != std::string::npos, "transaction json has id");
  require(json.find("\"command\": \"sch add-component U1\"") != std::string::npos,
          "transaction json has command");
  require(json.find("\"added\": 1") != std::string::npos, "transaction json has added count");
  require(json.find("\"object_id\": \"U1\"") != std::string::npos,
          "transaction json has diff object id");
}

