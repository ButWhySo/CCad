#include "ccad_gui/agent_panel.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QString>

namespace {

bool contains(const QString& haystack, const char* needle) {
  return haystack.contains(QString::fromUtf8(needle));
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  AgentPanel panel;
  panel.setProjectContext("bridge-demo.ccad.json", 17);
  panel.setUiMapProvider([]() {
    return QString("{\"schema_version\":1,\"ui_epoch\":17,\"nodes\":["
                   "{\"id\":\"action:zoom_in\"},{\"id\":\"panel:agent\"}]}\n");
  });
  panel.setSafeActionTrigger([](const QString& id) {
    return QString("{\"schema_version\":1,\"id\":\"%1\",\"performed\":true,"
                   "\"reason\":\"triggered\"}\n")
        .arg(id);
  });

  require(panel.projectText() == "Project bridge-demo.ccad.json",
          "agent panel displays project context");
  require(panel.epochText() == "UI map epoch 17", "agent panel displays UI map epoch");
  require(panel.actionIdText() == "action:zoom_in", "agent panel has safe default action id");

  panel.refreshUiMap();
  require(contains(panel.outputText(), "\"id\":\"panel:agent\""),
          "agent panel refreshes UI map output");
  require(contains(panel.statusText(), "UI map nodes 2"), "agent panel summarizes UI map nodes");

  panel.setActionId("tab:schematic");
  panel.triggerSafeAction();
  require(contains(panel.outputText(), "\"id\":\"tab:schematic\""),
          "agent panel runs safe action callback");
  require(contains(panel.outputText(), "\"performed\":true"),
          "agent panel preserves safe action result JSON");
}
