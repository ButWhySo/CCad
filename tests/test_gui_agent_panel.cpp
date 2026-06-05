#include "ccad_gui/agent_panel.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>
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
    if (method == "ui.screenshot") {
      return QString("{\"schema_version\":1,\"ok\":true,\"method\":\"ui.screenshot\","
                     "\"result\":{\"performed\":true,"
                     "\"path\":\"artifacts/screenshots/bridge-proof.png\","
                     "\"width\":1280,\"height\":720}}\n");
    }
    if (method == "project.drc") {
      return QString("{\"schema_version\":1,\"ok\":true,\"method\":\"project.drc\","
                     "\"result\":{\"diagnostic_count\":2,\"error_count\":1,"
                     "\"warning_count\":1,\"report_path\":\"artifacts/reports/drc.json\","
                     "\"diagnostics\":[]}}\n");
    }
    if (method == "project.erc") {
      return QString("{\"schema_version\":1,\"ok\":true,\"method\":\"project.erc\","
                     "\"result\":{\"diagnostic_count\":1,\"error_count\":0,"
                     "\"warning_count\":1,\"report_path\":\"artifacts/reports/erc.json\","
                     "\"diagnostics\":[]}}\n");
    }
    if (method == "project.diagnostics") {
      return QString("{\"schema_version\":1,\"ok\":true,\"method\":\"project.diagnostics\","
                     "\"result\":{\"erc_count\":1,\"drc_count\":2,"
                     "\"error_count\":1,\"warning_count\":2}}\n");
    }
    return QString("{\"schema_version\":1,\"ok\":true,\"method\":\"%1\","
                   "\"result\":{\"match_count\":1,\"nodes\":[{\"id\":\"action:add_footprint\"}]}}\n")
        .arg(method);
  });

  require(panel.projectText() == "Project bridge-demo.ccad.json",
          "agent panel displays project context");
  require(panel.epochText() == "UI map epoch 17", "agent panel displays UI map epoch");
  require(panel.actionIdText() == "action:zoom_in", "agent panel has safe default action id");
  require(contains(panel.workspaceStateJson(), "\"panel_layout\":\"vertical_agent_workspace\""),
          "agent panel exposes the vertical workspace layout contract");
  require(contains(panel.workspaceStateJson(), "\"visual_style\":\"agent_command_center_dense\""),
          "agent panel exposes the dense command-center visual style contract");
  require(contains(panel.workspaceStateJson(), "\"workspace_layout_version\":3"),
          "agent panel exposes the third workspace layout version");
  require(contains(panel.workspaceStateJson(), "\"run_state\":\"idle\""),
          "agent panel exposes local run state");
  require(contains(panel.workspaceStateJson(), "\"trace_label\":\"Trace: local-off\""),
          "agent panel exposes local trace state without enabling telemetry");
  require(contains(panel.workspaceStateJson(), "\"plan_items\":["),
          "agent panel serializes visible active plan rows");
  require(contains(panel.workspaceStateJson(), "\"visible_sections\":["),
          "agent panel serializes the visible command-center sections");
  require(panel.findChild<QLabel*>("label:agent_session_title") != nullptr,
          "agent panel exposes a session title for UI-map agents");
  require(panel.findChild<QWidget*>("panel:agent_session_strip") != nullptr,
          "agent panel exposes a compact session strip");
  require(panel.findChild<QWidget*>("panel:agent_mode_strip") != nullptr,
          "agent panel exposes model, mode, and permission chips as one strip");
  require(panel.findChild<QLabel*>("label:agent_model_chip") != nullptr,
          "agent panel exposes the model chip");
  require(panel.findChild<QLabel*>("label:agent_mode_chip") != nullptr,
          "agent panel exposes the mode chip");
  require(panel.findChild<QLabel*>("label:agent_permission_chip") != nullptr,
          "agent panel exposes the local permission chip");
  require(panel.findChild<QLabel*>("label:agent_trace_chip") != nullptr,
          "agent panel exposes the trace chip");
  require(panel.findChild<QLabel*>("label:agent_session_chip") != nullptr,
          "agent panel exposes the session chip");
  require(panel.findChild<QWidget*>("panel:agent_trace_strip") != nullptr,
          "agent panel exposes a trace/session strip");
  require(panel.findChild<QWidget*>("panel:agent_run_controls") != nullptr,
          "agent panel exposes a local run-control strip");
  require(panel.findChild<QLabel*>("label:agent_run_state_chip") != nullptr,
          "agent panel exposes a local run-state chip");
  require(panel.findChild<QPushButton*>("action:agent_pause_run") != nullptr,
          "agent panel exposes a safe pause run action");
  require(panel.findChild<QPushButton*>("action:agent_resume_run") != nullptr,
          "agent panel exposes a safe resume run action");
  require(panel.findChild<QPushButton*>("action:agent_stop_run") != nullptr,
          "agent panel exposes a safe stop run action");
  require(panel.findChild<QWidget*>("tab:agent_command") != nullptr,
          "agent panel exposes a command tab selector");
  require(panel.findChild<QWidget*>("tab:agent_evidence") != nullptr,
          "agent panel exposes an evidence tab selector");
  require(panel.findChild<QWidget*>("tab:agent_approvals") != nullptr,
          "agent panel exposes an approvals tab selector");
  require(panel.findChild<QWidget*>("panel:agent_activity_stream") != nullptr,
          "agent panel exposes a command activity stream");
  require(panel.findChild<QWidget*>("card:agent_activity_1") != nullptr,
          "agent panel renders an initial activity event");
  require(panel.findChild<QWidget*>("panel:agent_active_plan") != nullptr,
          "agent panel exposes a visible active-plan section");
  require(panel.findChild<QWidget*>("panel:agent_plan_row_1") != nullptr,
          "agent panel exposes the first active-plan row");
  require(panel.findChild<QWidget*>("panel:agent_plan_row_2") != nullptr,
          "agent panel exposes the second active-plan row");
  panel.findChild<QPushButton*>("action:agent_pause_run")->click();
  require(contains(panel.workspaceStateJson(), "\"run_state\":\"paused\""),
          "agent panel pause action updates local run state");
  panel.findChild<QPushButton*>("action:agent_resume_run")->click();
  require(contains(panel.workspaceStateJson(), "\"run_state\":\"running\""),
          "agent panel resume action updates local run state");
  panel.findChild<QPushButton*>("action:agent_stop_run")->click();
  require(contains(panel.workspaceStateJson(), "\"run_state\":\"stopped\""),
          "agent panel stop action updates local run state");
  require(panel.findChild<QPushButton*>("action:agent_header_request_context") != nullptr,
          "agent panel exposes a functional header context action");
  require(panel.findChild<QPushButton*>("action:agent_header_trigger_drc") != nullptr,
          "agent panel exposes a functional header diagnostics action");
  require(panel.findChild<QPushButton*>("action:agent_header_clear_output") != nullptr,
          "agent panel exposes a functional header clear action");
  require(panel.findChild<QWidget*>("panel:agent_command_stream") != nullptr,
          "agent panel exposes a command stream section");
  require(panel.findChild<QWidget*>("panel:agent_task_list") != nullptr,
          "agent panel exposes a task-list section");
  require(panel.findChild<QWidget*>("panel:agent_evidence_tray") != nullptr,
          "agent panel exposes an evidence tray section");
  require(panel.findChild<QWidget*>("panel:agent_approval_card") != nullptr,
          "agent panel exposes an approval-card section");
  panel.resize(420, 760);
  panel.show();
  QApplication::processEvents();
  auto* evidence_section = panel.findChild<QWidget*>("panel:agent_evidence_tray");
  auto* approval_section = panel.findChild<QWidget*>("panel:agent_approval_card");
  require(evidence_section->mapTo(&panel, QPoint(0, 0)).y() <
              approval_section->mapTo(&panel, QPoint(0, 0)).y(),
          "agent panel shows pinned evidence before approval cards");
  require(panel.findChild<QLineEdit*>("control:agent_command_input") != nullptr,
          "agent panel exposes a bottom command input");
  require(panel.findChild<QPushButton*>("action:agent_submit_command") != nullptr,
          "agent panel exposes a command submit action");
  require(panel.findChild<QPushButton*>("action:agent_footer_request_context") != nullptr,
          "agent panel exposes a visible request-context footer action");
  require(panel.findChild<QPushButton*>("action:agent_footer_trigger_drc") != nullptr,
          "agent panel exposes a visible trigger-DRC footer action");
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

  panel.setCommandText("Inspect DRC before routing");
  require(panel.commandText() == "Inspect DRC before routing",
          "agent panel stores command prompt text");
  panel.submitCommand();
  require(contains(panel.statusText(), "Command staged"),
          "agent panel stages command prompt text");
  require(contains(panel.resultStateText(), "Command staged"),
          "agent panel reports staged command result");
  require(contains(panel.outputText(), "agent_command_staged"),
          "agent panel writes command staging JSON to the stream");
  require(contains(panel.workspaceStateJson(), "\"command\":\"Inspect DRC before routing\""),
          "agent panel workspace state serializes the staged command");
  require(contains(panel.workspaceStateJson(), "\"activity_events\":["),
          "agent panel workspace state serializes activity events");
  require(contains(panel.workspaceStateJson(), "\"title\":\"Command staged\""),
          "agent panel activity stream records command staging");
  require(panel.findChild<QWidget*>("card:agent_activity_2") != nullptr,
          "agent panel appends command staging as a targetable activity card");

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

  panel.findChild<QPushButton*>("action:agent_footer_request_context")->click();
  require(live_method_seen == "agent.harness_context",
          "agent panel footer context action reuses the harness-context preset");
  panel.findChild<QPushButton*>("action:agent_footer_trigger_drc")->click();
  require(live_method_seen == "project.diagnostics",
          "agent panel footer DRC action reuses the diagnostics preset");
  panel.runToolGuidePreset();

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
  require(contains(workspace_state, "\"evidence_cards\":["),
          "agent panel workspace state serializes evidence cards");
  require(contains(workspace_state, "\"kind\":\"tool_result\""),
          "agent panel stores generic tool-guide evidence as a tool-result card");
  require(contains(workspace_state, "\"method\":\"agent.tool_guide\""),
          "agent panel evidence cards keep the producer method");
  require(contains(workspace_state, "\"title\":\"agent.tool_guide\""),
          "agent panel evidence cards expose a compact title");
  require(contains(workspace_state, "\"trace_id\":\"\""),
          "agent panel evidence cards include trace-ready trace_id");
  require(contains(workspace_state, "\"span_id\":\"\""),
          "agent panel evidence cards include trace-ready span_id");
  require(contains(workspace_state, "\"source\":\"agent_panel\""),
          "agent panel evidence cards identify their local source");
  require(panel.findChild<QWidget*>("card:agent_evidence_1") != nullptr,
          "agent panel renders the first pinned evidence as a targetable card widget");

  panel.clearEvidence();
  require(contains(panel.evidenceText(), "Evidence 0"),
          "agent panel clears pinned evidence");
  require(panel.findChild<QWidget*>("card:agent_evidence_1") == nullptr,
          "agent panel clear removes evidence card widgets");

  panel.setLiveQuery("ui.screenshot", "{\"path\":\"artifacts/screenshots/bridge-proof.png\"}");
  panel.runLiveQuery();
  panel.pinEvidence();
  QString evidence_cards_state = panel.workspaceStateJson();
  require(contains(evidence_cards_state, "\"kind\":\"screenshot\""),
          "agent panel classifies screenshot outputs as screenshot evidence");
  require(contains(evidence_cards_state,
                   "\"artifact_path\":\"artifacts/screenshots/bridge-proof.png\""),
          "agent panel screenshot evidence stores the artifact path");
  require(contains(evidence_cards_state, "\"width\":1280"),
          "agent panel screenshot evidence stores image width metadata");
  require(contains(evidence_cards_state, "\"height\":720"),
          "agent panel screenshot evidence stores image height metadata");

  panel.setLiveQuery("project.drc", "{}");
  panel.runLiveQuery();
  panel.pinEvidence();
  evidence_cards_state = panel.workspaceStateJson();
  require(contains(evidence_cards_state, "\"kind\":\"drc_report\""),
          "agent panel classifies DRC outputs as DRC report evidence");
  require(contains(evidence_cards_state, "\"error_count\":1"),
          "agent panel DRC evidence stores error count metadata");
  require(contains(evidence_cards_state, "\"warning_count\":1"),
          "agent panel DRC evidence stores warning count metadata");

  panel.setLiveQuery("project.erc", "{}");
  panel.runLiveQuery();
  panel.pinEvidence();
  evidence_cards_state = panel.workspaceStateJson();
  require(contains(evidence_cards_state, "\"kind\":\"erc_report\""),
          "agent panel classifies ERC outputs as ERC report evidence");
  require(contains(evidence_cards_state, "\"diagnostic_count\":1"),
          "agent panel ERC evidence stores diagnostic count metadata");

  panel.setLiveQuery("project.diagnostics", "{}");
  panel.runLiveQuery();
  panel.pinEvidence();
  evidence_cards_state = panel.workspaceStateJson();
  require(contains(evidence_cards_state, "\"kind\":\"diagnostics_report\""),
          "agent panel classifies combined diagnostics outputs as diagnostics evidence");
  require(contains(evidence_cards_state, "\"drc_count\":2"),
          "agent panel diagnostics evidence stores DRC count metadata");
  require(contains(evidence_cards_state, "\"erc_count\":1"),
          "agent panel diagnostics evidence stores ERC count metadata");

  panel.clearEvidence();
  require(contains(panel.evidenceText(), "Evidence 0"),
          "agent panel clears typed evidence cards");

  panel.setApprovalRequestText("Approve routing across DC bus");
  require(panel.approvalRequestText() == "Approve routing across DC bus",
          "agent panel stores approval request text");
  panel.requestApproval();
  require(contains(panel.approvalStatusText(), "Approval pending"),
          "agent panel creates a pending local approval");
  require(panel.pendingApprovalCount() == 1,
          "agent panel reports one pending approval");
  QString approval_state = panel.workspaceStateJson();
  require(contains(approval_state, "\"approval_pending_count\":1"),
          "agent panel workspace state serializes pending approval count");
  require(contains(approval_state, "\"approval_request\":\"Approve routing across DC bus\""),
          "agent panel workspace state serializes pending approval request");

  panel.approveNextApproval();
  require(panel.pendingApprovalCount() == 0,
          "agent panel accept clears the pending approval");
  require(contains(panel.approvalStatusText(), "accepted"),
          "agent panel records accepted approval status");
  approval_state = panel.workspaceStateJson();
  require(contains(approval_state, "\"approval_last_decision\":\"accept\""),
          "agent panel workspace state serializes accepted decision");

  panel.setApprovalRequestText("Decline risky delete");
  panel.requestApproval();
  panel.declineNextApproval();
  approval_state = panel.workspaceStateJson();
  require(contains(approval_state, "\"approval_last_decision\":\"decline\""),
          "agent panel workspace state serializes declined decision");

  panel.setApprovalRequestText("Cancel stale prompt");
  panel.requestApproval();
  panel.cancelApproval();
  approval_state = panel.workspaceStateJson();
  require(contains(approval_state, "\"approval_last_decision\":\"cancel\""),
          "agent panel workspace state serializes canceled decision");

  panel.setApprovalRequestText("Clear approvals");
  panel.requestApproval();
  panel.clearApprovals();
  require(panel.pendingApprovalCount() == 0,
          "agent panel clear approvals resets pending approval count");
  require(contains(panel.approvalStatusText(), "Approvals 0"),
          "agent panel clear approvals resets visible status");

  panel.clearOutput();
  require(panel.outputText().isEmpty(), "agent panel clear action clears output");
  require(contains(panel.statusText(), "Output cleared"), "agent panel clear action updates status");
  require(contains(panel.resultStateText(), "Idle"), "agent panel clear action resets result state");
}
