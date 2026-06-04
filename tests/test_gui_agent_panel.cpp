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
  panel.setWorkspaceContext("pcb", "F.Cu", "DC_POS", "route_track", 1, 2);
  require(contains(panel.workspaceText(), "View pcb"), "agent panel displays active view");
  require(contains(panel.workspaceText(), "Layer F.Cu"), "agent panel displays active layer");
  require(contains(panel.workspaceText(), "Net DC_POS"), "agent panel displays active net");
  require(contains(panel.workspaceText(), "Tool route_track"),
          "agent panel displays interaction mode");
  require(contains(panel.diagnosticsText(), "Diagnostics 1 errors / 2 warnings"),
          "agent panel displays diagnostic summary");

  panel.refreshUiMap();
  require(contains(panel.outputText(), "\"id\":\"panel:agent\""),
          "agent panel refreshes UI map output");
  require(contains(panel.statusText(), "UI map nodes 2"), "agent panel summarizes UI map nodes");
  require(contains(panel.resultStateText(), "Map refreshed"),
          "agent panel summarizes refreshed map result");

  panel.setActionId("tab:schematic");
  panel.triggerSafeAction();
  require(contains(panel.outputText(), "\"id\":\"tab:schematic\""),
          "agent panel runs safe action callback");
  require(contains(panel.outputText(), "\"performed\":true"),
          "agent panel preserves safe action result JSON");
  require(contains(panel.resultStateText(), "Performed"),
          "agent panel summarizes performed safe action result");

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
  require(contains(panel.resultStateText(), "OK"),
          "agent panel summarizes successful live query result");

  panel.runHarnessContextPreset();
  require(live_method_seen == "agent.harness_context",
          "agent panel harness preset sends harness context query");
  require(live_payload_seen == "{}", "agent panel harness preset sends empty payload");

  panel.runDiagnosticsPreset();
  require(live_method_seen == "project.diagnostics",
          "agent panel diagnostics preset sends diagnostics query");
  require(live_payload_seen == "{}", "agent panel diagnostics preset sends empty payload");

  panel.runToolGuidePreset();
  require(live_method_seen == "agent.tool_guide",
          "agent panel tool-guide preset sends tool-guide query");
  require(contains(live_payload_seen, "ui.route_track"),
          "agent panel tool-guide preset documents a concrete workflow method");

  panel.setGoalText("Inspect bridge rectifier placement");
  require(panel.goalText() == "Inspect bridge rectifier placement",
          "agent panel stores task goal text");
  panel.stageGoal();
  require(contains(panel.taskStateText(), "Goal staged"),
          "agent panel stages a local task goal");
  require(contains(panel.taskStateText(), "Inspect bridge rectifier placement"),
          "agent panel task state includes staged goal");

  panel.pinEvidence();
  require(contains(panel.evidenceText(), "Evidence 1"),
          "agent panel pins current result as evidence");
  const QString workspace_state = panel.workspaceStateJson();
  require(contains(workspace_state, "\"goal\":\"Inspect bridge rectifier placement\""),
          "agent panel workspace state serializes staged goal");
  require(contains(workspace_state, "\"evidence_count\":1"),
          "agent panel workspace state serializes evidence count");
  require(contains(workspace_state, "agent.tool_guide"),
          "agent panel workspace state serializes evidence entries");

  panel.clearEvidence();
  require(contains(panel.evidenceText(), "Evidence 0"),
          "agent panel clears pinned evidence");

  panel.clearOutput();
  require(panel.outputText().isEmpty(), "agent panel clear action clears output");
  require(contains(panel.statusText(), "Output cleared"), "agent panel clear action updates status");
  require(contains(panel.resultStateText(), "Idle"), "agent panel clear action resets result state");
}
