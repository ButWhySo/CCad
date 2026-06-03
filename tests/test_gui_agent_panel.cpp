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
  QString live_method_seen;
  QString live_payload_seen;
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
  panel.setLiveQueryProvider([&](const QString& method, const QString& payload) {
    live_method_seen = method;
    live_payload_seen = payload;
    return QString("{\"schema_version\":1,\"ok\":true,\"method\":\"%1\","
                   "\"result\":{\"match_count\":1,\"nodes\":[{\"id\":\"action:add_footprint\"}]}}\n")
        .arg(method);
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

  panel.setLiveQuery("ui.find", "{\"query\":\"add\",\"role\":\"action\",\"limit\":4}");
  require(panel.liveMethodText() == "ui.find", "agent panel stores live query method");
  require(panel.livePayloadText().contains("\"query\":\"add\""),
          "agent panel stores live query payload");
  panel.runLiveQuery();
  require(live_method_seen == "ui.find", "agent panel sends live query method");
  require(live_payload_seen.contains("\"role\":\"action\""),
          "agent panel sends live query payload");
  require(contains(panel.outputText(), "\"id\":\"action:add_footprint\""),
          "agent panel renders live query result");
  require(contains(panel.statusText(), "Live query ui.find"),
          "agent panel reports live query status");
}
