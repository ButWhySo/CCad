#include "ccad_gui/agent_panel.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QFile>
#include <QTextStream>
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
  require(contains(panel.workspaceStateJson(), "\"visual_style\":\"agent_reference_panel_v4\""),
          "agent panel exposes the reference-inspired visual style contract");
  require(contains(panel.workspaceStateJson(), "\"workspace_layout_version\":4"),
          "agent panel exposes the fourth workspace layout version");
  require(contains(panel.workspaceStateJson(), "\"run_state\":\"idle\""),
          "agent panel exposes local run state");
  require(contains(panel.workspaceStateJson(), "\"trace_label\":\"Trace: local-off\""),
          "agent panel exposes local trace state without enabling telemetry");
  require(contains(panel.workspaceStateJson(), "\"trace_id\":\"\""),
          "agent panel starts without a local trace id");
  require(contains(panel.workspaceStateJson(), "\"span_id\":\"\""),
          "agent panel starts without a local span id");
  require(contains(panel.workspaceStateJson(), "\"trace_export_enabled\":false"),
          "agent panel keeps trace export disabled by default");
  require(contains(panel.workspaceStateJson(), "\"trace_link_available\":false"),
          "agent panel reports no external trace link before export is configured");
  require(contains(panel.workspaceStateJson(), "\"plan_items\":["),
          "agent panel serializes visible active plan rows");
  require(contains(panel.workspaceStateJson(), "\"visible_sections\":["),
          "agent panel serializes the visible command-center sections");
  require(contains(panel.workspaceStateJson(), "\"status_rail\""),
          "agent panel reports a compact status rail");
  require(contains(panel.workspaceStateJson(), "\"command_composer\""),
          "agent panel reports a command composer region");
  require(contains(panel.workspaceStateJson(), "\"plan_deck\""),
          "agent panel reports a plan deck region");
  require(contains(panel.workspaceStateJson(), "\"evidence_lane\""),
          "agent panel reports a compact evidence lane");
  require(contains(panel.workspaceStateJson(), "\"approval_lane\""),
          "agent panel reports a compact approval lane");
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
  require(panel.findChild<QWidget*>("panel:agent_trace_links") != nullptr,
          "agent panel exposes a trace-link metadata panel");
  require(panel.findChild<QLabel*>("label:agent_trace_id") != nullptr,
          "agent panel exposes a selectable trace id label");
  require(panel.findChild<QLabel*>("label:agent_span_id") != nullptr,
          "agent panel exposes a selectable span id label");
  require(panel.findChild<QLabel*>("label:agent_trace_status") != nullptr,
          "agent panel exposes trace status");
  require(panel.findChild<QLabel*>("label:agent_trace_export_status") != nullptr,
          "agent panel exposes trace export status");
  auto* trace_context_button = panel.findChild<QPushButton*>("action:agent_new_trace_context");
  require(trace_context_button != nullptr,
          "agent panel exposes a local trace context action");
  require(!trace_context_button->icon().isNull(),
          "agent panel gives the local trace action a visible icon affordance");
  require(trace_context_button->property("agentTraceAction").toBool(),
          "agent panel marks the local trace action as visually prominent");
  require(panel.findChild<QWidget*>("panel:agent_session_binding") != nullptr,
          "agent panel exposes a durable session binding strip");
  require(panel.findChild<QLineEdit*>("control:agent_session_path") != nullptr,
          "agent panel exposes a local session path input");
  require(panel.findChild<QPushButton*>("action:agent_load_session") != nullptr,
          "agent panel exposes a local session load action");
  require(panel.findChild<QPushButton*>("action:agent_checkpoint_session") != nullptr,
          "agent panel exposes a local checkpoint action");
  require(panel.findChild<QLabel*>("label:agent_session_status") != nullptr,
          "agent panel exposes durable session status");
  require(contains(panel.workspaceStateJson(), "\"durable_session_bound\":false"),
          "agent panel starts without a bound durable session");
  require(panel.findChild<QWidget*>("panel:agent_policy_surface") != nullptr,
          "agent panel exposes command policy surface");
  require(panel.findChild<QLabel*>("label:agent_policy_decision") != nullptr,
          "agent panel exposes command policy decision label");
  require(panel.findChild<QLabel*>("label:agent_policy_risk") != nullptr,
          "agent panel exposes command policy risk label");
  require(panel.findChild<QCheckBox*>("control:agent_policy_dry_run") != nullptr,
          "agent panel exposes command policy dry-run toggle");
  require(panel.findChild<QPushButton*>("action:agent_policy_preview") != nullptr,
          "agent panel exposes command policy preview action");
  require(contains(panel.workspaceStateJson(), "\"policy_decision\":\"not_classified\""),
          "agent panel starts with unclassified command policy");
  require(panel.findChild<QWidget*>("panel:agent_provider_controls") != nullptr,
          "agent panel exposes provider controls");
  auto* provider_selector = panel.findChild<QComboBox*>("control:agent_provider_family");
  require(provider_selector != nullptr,
          "agent panel exposes a native provider family selector");
  require(provider_selector->count() >= 5,
          "agent panel lists OpenAI, Anthropic, Gemini, compatible, and local providers");
  require(panel.findChild<QLineEdit*>("control:agent_provider_model") != nullptr,
          "agent panel exposes a provider model hint input");
  require(panel.findChild<QPushButton*>("action:agent_provider_refresh_status") != nullptr,
          "agent panel exposes provider status refresh action");
  require(panel.findChild<QLabel*>("label:agent_provider_status") != nullptr,
          "agent panel exposes provider readiness status");
  require(panel.findChild<QLabel*>("label:agent_provider_env") != nullptr,
          "agent panel exposes provider env-var status");
  require(panel.findChild<QLabel*>("label:agent_provider_execution_status") != nullptr,
          "agent panel exposes provider execution-disabled status");
  QString provider_state = panel.workspaceStateJson();
  require(contains(provider_state, "\"provider_panel_available\":true"),
          "agent panel workspace state reports provider controls are available");
  require(contains(provider_state, "\"provider_id\":\"openai\""),
          "agent panel defaults to the OpenAI provider metadata row");
  require(contains(provider_state, "\"provider_api_key_env\":\"OPENAI_API_KEY\""),
          "agent panel reports the selected provider env var name without value");
  require(contains(provider_state, "\"provider_execution_enabled\":false"),
          "agent panel keeps provider execution disabled");
  require(contains(provider_state, "\"provider_secret_value_visible\":false"),
          "agent panel never exposes provider secret values");
  require(contains(provider_state, "\"provider_network_probe_enabled\":false"),
          "agent panel does not probe provider endpoints from the GUI");
  require(contains(provider_state, "\"provider_status_method\":\"agent.provider_status\""),
          "agent panel points to the existing headless provider status method");
  const int anthropic_index = provider_selector->findData("anthropic");
  require(anthropic_index >= 0, "agent panel can select Anthropic by stable provider id");
  provider_selector->setCurrentIndex(anthropic_index);
  panel.findChild<QLineEdit*>("control:agent_provider_model")->setText("claude-sonnet-4-6");
  panel.findChild<QPushButton*>("action:agent_provider_refresh_status")->click();
  provider_state = panel.workspaceStateJson();
  require(contains(provider_state, "\"provider_id\":\"anthropic\""),
          "agent panel updates provider metadata when the provider selector changes");
  require(contains(provider_state, "\"provider_api_key_env\":\"ANTHROPIC_API_KEY\""),
          "agent panel reports Anthropic env-var readiness metadata");
  require(contains(provider_state, "\"provider_model_hint\":\"claude-sonnet-4-6\""),
          "agent panel serializes the local model hint without executing it");
  require(contains(provider_state, "\"provider_execution_enabled\":false"),
          "agent panel still keeps provider execution disabled after refresh");
  require(contains(panel.outputText(), "\"event\":\"agent_provider_status_refreshed\""),
          "agent panel emits a local provider status refresh event");
  require(panel.findChild<QWidget*>("panel:agent_run_controls") != nullptr,
          "agent panel exposes a local run-control strip");
  require(panel.findChild<QWidget*>("panel:agent_status_rail") != nullptr,
          "agent panel exposes a compact status rail");
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

  QTemporaryDir session_dir;
  require(session_dir.isValid(), "agent panel test creates a temporary session directory");
  const QString session_path = session_dir.path() + "/demo.ccad-agent-session.json";
  QFile session_file(session_path);
  require(session_file.open(QIODevice::WriteOnly | QIODevice::Text),
          "agent panel test writes a local session file");
  QTextStream session_out(&session_file);
  session_out << "{\"schema_version\":1,"
              << "\"session_kind\":\"ccad_agent_session\","
              << "\"session_id\":\"gui-run-001\","
              << "\"thread_id\":\"gui-thread-001\","
              << "\"title\":\"GUI durable session\","
              << "\"project_path\":\"bridge-demo.ccad.json\","
              << "\"created_at\":\"2026-06-05T00:00:00Z\","
              << "\"updated_at\":\"2026-06-05T00:01:00Z\","
              << "\"durability\":\"local_json_checkpoint_file\","
              << "\"resource_uri\":\"ccad-agent-session:gui-run-001\","
              << "\"checkpoint_count\":1,"
              << "\"checkpoints\":[{\"checkpoint_id\":\"cp-001\","
              << "\"sequence\":1,\"kind\":\"visual\","
              << "\"summary\":\"initial screenshot\","
              << "\"artifact_path\":\"artifacts/screenshots/session.png\","
              << "\"created_at\":\"2026-06-05T00:01:00Z\","
              << "\"resource_uri\":\"ccad-agent-checkpoint:gui-run-001/cp-001\"}]}";
  session_file.close();

  panel.setSessionFilePath(session_path);
  panel.bindSessionFile(session_path);
  QString session_state = panel.workspaceStateJson();
  require(contains(session_state, "\"durable_session_bound\":true"),
          "agent panel marks a loaded local session as bound");
  require(contains(session_state, "\"session_file_path\":"),
          "agent panel serializes the local session path");
  require(contains(session_state, "\"durable_session_id\":\"gui-run-001\""),
          "agent panel serializes the durable session id");
  require(contains(session_state, "\"thread_id\":\"gui-thread-001\""),
          "agent panel serializes the durable thread id");
  require(contains(session_state, "\"checkpoint_count\":1"),
          "agent panel serializes existing checkpoint count");
  require(contains(session_state, "\"latest_checkpoint_id\":\"cp-001\""),
          "agent panel serializes the latest checkpoint id");
  require(contains(session_state, "\"replayable\":true"),
          "agent panel marks a session with checkpoints as replayable");
  require(contains(panel.sessionStatusText(), "gui-run-001"),
          "agent panel displays bound session status");

  panel.checkpointSession();
  session_state = panel.workspaceStateJson();
  require(contains(session_state, "\"checkpoint_count\":2"),
          "agent panel increments checkpoint count after local checkpoint");
  require(contains(session_state, "\"latest_checkpoint_id\":\"gui-checkpoint-2\""),
          "agent panel reports the new GUI checkpoint id");
  QFile updated_session(session_path);
  require(updated_session.open(QIODevice::ReadOnly | QIODevice::Text),
          "agent panel test reads updated local session file");
  const QString updated_json = QString::fromUtf8(updated_session.readAll());
  require(contains(updated_json, "\"checkpoint_id\":\"gui-checkpoint-2\""),
          "agent panel writes metadata-only checkpoint to the local session file");

  trace_context_button->click();
  QString trace_state = panel.workspaceStateJson();
  require(contains(trace_state, "\"trace_id\":\"ccad-local-trace-1\""),
          "agent panel creates a deterministic local trace id");
  require(contains(trace_state, "\"span_id\":\"ccad-local-span-1\""),
          "agent panel creates a deterministic local span id");
  require(contains(trace_state, "\"trace_status\":\"local_ready\""),
          "agent panel marks local trace metadata as ready");
  require(contains(trace_state, "\"trace_export_status\":\"export_disabled\""),
          "agent panel keeps exporter status explicit");
  require(contains(trace_state, "\"trace_backend\":\"local_metadata_only\""),
          "agent panel reports local metadata-only trace backend");
  require(contains(trace_state, "\"trace_session_id\":\"gui-run-001\""),
          "agent panel links trace metadata to the bound local session");
  require(contains(trace_state, "\"trace_thread_id\":\"gui-thread-001\""),
          "agent panel links trace metadata to the durable thread id");
  require(contains(trace_state, "\"trace_content_policy\":\"metadata_only_no_prompt_tool_or_design_payloads\""),
          "agent panel records the trace redaction/content policy");
  require(contains(panel.outputText(), "\"event\":\"agent_trace_context_ready\""),
          "agent panel emits a local trace context event");

  require(panel.findChild<QPushButton*>("action:agent_header_request_context") != nullptr,
          "agent panel exposes a functional header context action");
  require(panel.findChild<QPushButton*>("action:agent_header_trigger_drc") != nullptr,
          "agent panel exposes a functional header diagnostics action");
  require(panel.findChild<QPushButton*>("action:agent_header_clear_output") != nullptr,
          "agent panel exposes a functional header clear action");
  require(panel.findChild<QWidget*>("panel:agent_command_stream") != nullptr,
          "agent panel exposes a command stream section");
  require(panel.findChild<QWidget*>("panel:agent_command_composer") != nullptr,
          "agent panel exposes a command composer section");
  require(panel.findChild<QWidget*>("panel:agent_task_list") != nullptr,
          "agent panel exposes a task-list section");
  require(panel.findChild<QWidget*>("panel:agent_plan_deck") != nullptr,
          "agent panel exposes a plan deck section");
  require(panel.findChild<QWidget*>("panel:agent_evidence_tray") != nullptr,
          "agent panel exposes an evidence tray section");
  require(panel.findChild<QWidget*>("panel:agent_evidence_lane") != nullptr,
          "agent panel exposes a compact evidence lane");
  require(panel.findChild<QWidget*>("panel:agent_approval_card") != nullptr,
          "agent panel exposes an approval-card section");
  require(panel.findChild<QWidget*>("panel:agent_approval_lane") != nullptr,
          "agent panel exposes a compact approval lane");
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

  panel.setCommandText("help --format json");
  panel.submitCommand();
  QString policy_state = panel.workspaceStateJson();
  require(contains(policy_state, "\"policy_decision\":\"allow_read\""),
          "agent panel classifies read commands as allow_read");
  require(contains(policy_state, "\"policy_risk_level\":\"low\""),
          "agent panel classifies read commands as low risk");
  require(contains(policy_state, "\"policy_approval_required\":false"),
          "agent panel does not require approval for read commands");
  require(contains(policy_state, "\"policy_would_execute\":true"),
          "agent panel reports read command would execute outside dry run");

  panel.setCommandText("pcb add-via --file board.ccad.json");
  panel.submitCommand();
  policy_state = panel.workspaceStateJson();
  require(contains(policy_state, "\"policy_decision\":\"approval_required\""),
          "agent panel classifies write commands as approval_required");
  require(contains(policy_state, "\"policy_approval_reason\":\"project_mutation\""),
          "agent panel reports project mutation approval reason");
  require(contains(policy_state, "\"policy_risk_level\":\"high\""),
          "agent panel classifies project mutation as high risk");
  require(panel.pendingApprovalCount() == 1,
          "agent panel creates a pending approval from write policy preview");
  require(contains(panel.approvalStatusText(), "project_mutation"),
          "agent panel approval lane carries the policy reason");

  panel.findChild<QCheckBox*>("control:agent_policy_dry_run")->setChecked(true);
  panel.setCommandText("pcb add-via --file board.ccad.json");
  panel.findChild<QPushButton*>("action:agent_policy_preview")->click();
  policy_state = panel.workspaceStateJson();
  require(contains(policy_state, "\"policy_dry_run\":true"),
          "agent panel policy preview honors dry-run toggle");
  require(contains(policy_state, "\"policy_decision\":\"dry_run_only\""),
          "agent panel reports dry-run-only policy decision");
  require(contains(policy_state, "\"policy_would_execute\":false"),
          "agent panel dry-run policy reports no execution");

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
