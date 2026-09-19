#pragma once

#include <QWidget>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>
#include <memory>

#include "ccad_core/agent_orchestrator.hpp"
class QLabel;
class QFrame;
class QPushButton;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QTextEdit;
class QTextBrowser;
class QVBoxLayout;
class QProcess;
class QProgressBar;
class QListWidget;

class AgentPanel final : public QWidget {
 public:
  using UiMapProvider = std::function<QString()>;
  using SafeActionTrigger = std::function<QString(const QString&)>;
  using LiveQueryProvider = std::function<QString(const QString&, const QString&)>;
  using ContextProvider = std::function<std::string()>;
  using ConfigStateCallback = std::function<void(const QJsonObject&)>;
  using ProviderStateCallback = std::function<void(const QJsonObject&)>;
  using MarketplaceCatalogCallback = std::function<void(const QJsonObject&)>;
  using ModelCatalogCallback = std::function<void(const QJsonObject&)>;
  using McpStatusCallback = std::function<void(const QJsonObject&)>;
  using ComponentWizardCallback = std::function<void(const QJsonObject&)>;
  using GridSettingsCallback = std::function<void(double, bool)>;

  struct AgentSessionMetadata {
    QString session_id;
    QString thread_id;
    QString title;
    QString project_path;
    QString latest_checkpoint_id;
    int checkpoint_count = 0;
    bool replayable = false;
    QJsonObject queue_state;
  };

  explicit AgentPanel(QWidget* parent = nullptr);
  ~AgentPanel() override;

  void setUiMapProvider(UiMapProvider provider);
  void setSafeActionTrigger(SafeActionTrigger trigger);
  void setLiveQueryProvider(LiveQueryProvider provider);
  void setContextProvider(ContextProvider provider);
  void setProviderSecret(const QString& provider_id, const QString& secret);
  void setConfigStateCallback(ConfigStateCallback cb);
  void setProviderStateCallback(ProviderStateCallback cb);
  void setMarketplaceCatalogCallback(MarketplaceCatalogCallback cb);
  void setModelCatalogCallback(ModelCatalogCallback cb);
  void setMcpStatusCallback(McpStatusCallback cb);
  void setComponentWizardCallback(ComponentWizardCallback cb);
  void setGridSettingsCallback(GridSettingsCallback cb);
  void setProjectContext(const QString& project_label, int ui_map_epoch);
  void setWorkspaceContext(const QString& active_view,
                           const QString& active_layer,
                           const QString& active_net,
                           const QString& interaction_mode,
                           int error_count,
                           int warning_count);
  void setActionId(const QString& action_id);
  void setLiveQuery(const QString& method, const QString& payload);
  void setGoalText(const QString& goal);
  void setCommandText(const QString& command);
  void setSessionFilePath(const QString& path);

  void refreshUiMap();
  void triggerSafeAction();
  void runLiveQuery();
  void runHarnessContextPreset();
  void runDiagnosticsPreset();
  void runToolGuidePreset();
  void clearOutput();
  void stageGoal();
  void pinEvidence();
  void clearEvidence();
  void setApprovalRequestText(const QString& request);
  void requestApproval();
  void submitCommand();
  void bindSessionFile(const QString& path);
  void checkpointSession();
  void approveNextApproval();
  void declineNextApproval();
  void cancelApproval();
  void clearApprovals();
  void showProposal(const QString& summary, const QStringList& changes);
  void clearProposal();

  QString projectText() const;
  QString epochText() const;
  QString statusText() const;
  QString workspaceText() const;
  QString diagnosticsText() const;
  QString resultStateText() const;
  QString actionIdText() const;
  QString liveMethodText() const;
  QString livePayloadText() const;
  QString goalText() const;
  QString commandText() const;
  QString taskStateText() const;
  QString evidenceText() const;
  QString sessionPathText() const;
  QString sessionStatusText() const;
  QString approvalRequestText() const;
  QString approvalStatusText() const;
  int pendingApprovalCount() const;
  bool proposalVisible() const;
  QString outputText() const;
  QString workspaceStateJson() const;

  void sendJsonRpc(const QString& method, const QJsonObject& params);

 private:
  struct EvidenceCard {
    QString id;
    QString kind;
    QString title;
    QString summary;
    QString method;
    QString artifact_path;
    QString created_at;
    QString trace_id;
    QString span_id;
    QString source;
    int diagnostic_count = -1;
    int error_count = -1;
    int warning_count = -1;
    int drc_count = -1;
    int erc_count = -1;
    int width = -1;
    int height = -1;
  };

  struct ActivityEvent {
    QString id;
    QString kind;
    QString title;
    QString detail;
    QString method;
    QString created_at;
  };

  void renderEvidenceCards();
  QJsonObject evidenceCardJson(const EvidenceCard& card) const;
  void addActivityEvent(const QString& kind,
                        const QString& title,
                        const QString& detail,
                        const QString& method);
  void renderActivityEvents();
  QJsonObject activityEventJson(const ActivityEvent& event) const;
  void updateRunState(const QString& state, const QString& title, const QString& detail);
  void updateRunQueueLabels();
  QJsonObject runQueueStateObject() const;
  void cancelRunQueue();
  void clearRunQueue();
  void pauseRun();
  void resumeRun();
  void stopRun();
  void createLocalTraceContext();
  void refreshProviderStatus();
  void updateProviderControls();
  QJsonObject providerStateObject() const;
  void previewCommandPolicy();
  void classifyCommandPolicy(const QString& command, bool record_activity);
  QJsonObject policyStateObject() const;
  void applySessionMetadata(const AgentSessionMetadata& metadata, const QString& path);
  void resetSessionBinding(const QString& status);
  
  // New UI mapping methods for the Chat layout
  void appendChatMessage(const QString& role, const QString& text);
  void renderChatChecklist();

  // Python IPC
  void startPythonBackend();
  void handlePythonOutput();
  void handlePythonError();
  void submitChat();
  bool eventFilter(QObject* obj, QEvent* event) override;

  void showSlashPopup();
  void hideSlashPopup();
  void filterSlashCommands();
  void executeSlashCommand(const QString& cmd);

  // Chat UI Elements
  QTextBrowser* chat_stream_ = nullptr;
  QTextEdit* chat_input_ = nullptr;
  QListWidget* slash_popup_ = nullptr;

  // Preserved logic labels (hidden or refactored as needed)
  QLabel* session_title_label_ = nullptr;
  QLabel* model_chip_label_ = nullptr;
  QLabel* mode_chip_label_ = nullptr;
  QLabel* permission_chip_label_ = nullptr;
  QLabel* trace_chip_label_ = nullptr;
  QLabel* session_chip_label_ = nullptr;
  QLabel* trace_id_label_ = nullptr;
  QLabel* span_id_label_ = nullptr;
  QLabel* trace_status_label_ = nullptr;
  QLabel* trace_export_status_label_ = nullptr;
  QLabel* run_state_chip_label_ = nullptr;
  QLabel* run_queue_status_label_ = nullptr;
  QLabel* run_queue_counts_label_ = nullptr;
  QLabel* run_queue_current_step_label_ = nullptr;
  QLabel* session_status_label_ = nullptr;
  QLabel* policy_decision_label_ = nullptr;
  QLabel* policy_risk_label_ = nullptr;
  QLabel* provider_status_label_ = nullptr;
  QLabel* provider_env_label_ = nullptr;
  QLabel* provider_execution_status_label_ = nullptr;
  QLabel* project_label_ = nullptr;
  QLabel* epoch_label_ = nullptr;
  QLabel* status_label_ = nullptr;
  QLabel* workspace_label_ = nullptr;
  QLabel* diagnostics_label_ = nullptr;
  QLabel* result_state_label_ = nullptr;
  QLabel* task_state_label_ = nullptr;
  QLabel* evidence_label_ = nullptr;
  QLabel* approval_status_label_ = nullptr;
  QLabel* context_usage_label_ = nullptr;
  QFrame* approval_preview_ = nullptr;
  QFrame* proposal_card_ = nullptr;
  QLabel* proposal_summary_label_ = nullptr;
  QListWidget* proposal_changes_list_ = nullptr;
  QPushButton* proposal_approve_button_ = nullptr;
  QPushButton* proposal_revise_button_ = nullptr;
  QPushButton* proposal_reject_button_ = nullptr;
  QCheckBox* proposal_preserve_placement_ = nullptr;
  QCheckBox* proposal_fewer_vias_ = nullptr;
  QCheckBox* proposal_avoid_area_ = nullptr;
  QTextEdit* proposal_revision_input_ = nullptr;
  QPushButton* proposal_submit_revision_button_ = nullptr;
  QPushButton* proposal_details_button_ = nullptr;
  QString proposal_before_snapshot_;
  QLabel* approval_summary_label_ = nullptr;
  QLabel* approval_delta_label_ = nullptr;
  QPushButton* approval_request_button_ = nullptr;
  QPushButton* approval_accept_button_ = nullptr;
  QPushButton* approval_decline_button_ = nullptr;
  QPushButton* approval_cancel_button_ = nullptr;
  QPushButton* approval_clear_button_ = nullptr;

  QLineEdit* action_id_input_ = nullptr;
  QLineEdit* session_path_input_ = nullptr;
  QComboBox* provider_selector_ = nullptr;
  QLineEdit* provider_model_input_ = nullptr;
  QLineEdit* live_method_input_ = nullptr;
  QLineEdit* live_payload_input_ = nullptr;
  QLineEdit* goal_input_ = nullptr;
  QLineEdit* command_input_ = nullptr;
  QLineEdit* approval_request_input_ = nullptr;
  QCheckBox* policy_dry_run_checkbox_ = nullptr;
  QPlainTextEdit* output_ = nullptr;

  UiMapProvider ui_map_provider_;
  SafeActionTrigger safe_action_trigger_;
  LiveQueryProvider live_query_provider_;
  QString staged_goal_;
  QString staged_command_;
  QString run_state_ = "idle";
  QString run_queue_id_ = "local-run-queue-1";
  QString run_queue_status_ = "idle";
  QString run_queue_current_step_ = "Collect evidence";
  int run_queue_depth_ = 3;
  int run_queue_completed_count_ = 0;
  int run_queue_failed_count_ = 0;
  int run_steps_total_ = 3;
  bool run_queue_cancelable_ = true;
  bool backend_ready_ = false;
  bool backend_provider_initialized_ = false;
  QString trace_id_;
  QString span_id_;
  bool tool_result_ack_visible_ = false;
  QString trace_status_ = "local_off";
  QString trace_export_status_ = "export_disabled";
  QString session_file_path_;
  QString durable_session_id_;
  QString durable_thread_id_;
  QString latest_checkpoint_id_;
  bool durable_session_bound_ = false;
  bool session_replayable_ = false;
  int session_checkpoint_count_ = 0;
  QString policy_decision_ = "not_classified";
  QString policy_risk_level_ = "none";
  QString policy_approval_reason_;
  QString policy_command_;
  QStringList policy_args_;
  bool policy_approval_required_ = false;
  bool policy_read_only_ = false;
  bool policy_mutates_project_ = false;
  bool policy_mutates_files_ = false;
  bool policy_dry_run_ = false;
  bool policy_would_execute_ = false;
  QString provider_status_ = "env_unchecked";
  QString provider_model_ = "Auto";
  bool provider_env_present_ = false;
  QHash<QString, QString> provider_secrets_;
  QVector<ActivityEvent> activity_events_;
  QVector<EvidenceCard> evidence_cards_;
  QString last_chat_role_;
  QString last_chat_text_;
  int activity_sequence_ = 0;
  int evidence_sequence_ = 0;
  int trace_sequence_ = 0;
  QString pending_approval_request_;
  QString pending_tool_name_;
  QString pending_tool_args_;
  QString pending_tool_call_id_;
  QString pending_approval_token_;
  QString approval_last_decision_ = "none";

  QProcess* python_process_ = nullptr;
  std::unique_ptr<ccad::AgentOrchestrator> orchestrator_;
  ContextProvider context_provider_;
  ConfigStateCallback config_state_cb_;
  ProviderStateCallback provider_state_cb_;
  MarketplaceCatalogCallback marketplace_catalog_cb_;
  ModelCatalogCallback model_catalog_cb_;
  McpStatusCallback mcp_status_cb_;
  ComponentWizardCallback component_wizard_cb_;
  GridSettingsCallback grid_settings_cb_;
};
