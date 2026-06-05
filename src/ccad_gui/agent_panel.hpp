#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include <functional>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QVBoxLayout;

class AgentPanel final : public QWidget {
 public:
  using UiMapProvider = std::function<QString()>;
  using SafeActionTrigger = std::function<QString(const QString&)>;
  using LiveQueryProvider = std::function<QString(const QString&, const QString&)>;

  explicit AgentPanel(QWidget* parent = nullptr);

  void setUiMapProvider(UiMapProvider provider);
  void setSafeActionTrigger(SafeActionTrigger trigger);
  void setLiveQueryProvider(LiveQueryProvider provider);
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
  void approveNextApproval();
  void declineNextApproval();
  void cancelApproval();
  void clearApprovals();

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
  QString approvalRequestText() const;
  QString approvalStatusText() const;
  int pendingApprovalCount() const;
  QString outputText() const;
  QString workspaceStateJson() const;

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

  QLabel* session_title_label_ = nullptr;
  QLabel* model_chip_label_ = nullptr;
  QLabel* mode_chip_label_ = nullptr;
  QLabel* permission_chip_label_ = nullptr;
  QLabel* project_label_ = nullptr;
  QLabel* epoch_label_ = nullptr;
  QLabel* status_label_ = nullptr;
  QLabel* workspace_label_ = nullptr;
  QLabel* diagnostics_label_ = nullptr;
  QLabel* result_state_label_ = nullptr;
  QLabel* task_state_label_ = nullptr;
  QLabel* evidence_label_ = nullptr;
  QVBoxLayout* activity_events_layout_ = nullptr;
  QVBoxLayout* evidence_cards_layout_ = nullptr;
  QLabel* approval_status_label_ = nullptr;
  QLineEdit* action_id_input_ = nullptr;
  QLineEdit* live_method_input_ = nullptr;
  QLineEdit* live_payload_input_ = nullptr;
  QLineEdit* goal_input_ = nullptr;
  QLineEdit* command_input_ = nullptr;
  QLineEdit* approval_request_input_ = nullptr;
  QPlainTextEdit* output_ = nullptr;
  UiMapProvider ui_map_provider_;
  SafeActionTrigger safe_action_trigger_;
  LiveQueryProvider live_query_provider_;
  QString staged_goal_;
  QString staged_command_;
  QVector<ActivityEvent> activity_events_;
  QVector<EvidenceCard> evidence_cards_;
  int activity_sequence_ = 0;
  int evidence_sequence_ = 0;
  QString pending_approval_request_;
  QString approval_last_decision_ = "none";
};
