#pragma once

#include <QWidget>
#include <QString>

#include <functional>

class QLabel;
class QLineEdit;
class QPlainTextEdit;

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

  void refreshUiMap();
  void triggerSafeAction();
  void runLiveQuery();
  void runHarnessContextPreset();
  void runDiagnosticsPreset();
  void runToolGuidePreset();
  void clearOutput();

  QString projectText() const;
  QString epochText() const;
  QString statusText() const;
  QString workspaceText() const;
  QString diagnosticsText() const;
  QString resultStateText() const;
  QString actionIdText() const;
  QString liveMethodText() const;
  QString livePayloadText() const;
  QString outputText() const;

 private:
  QLabel* project_label_ = nullptr;
  QLabel* epoch_label_ = nullptr;
  QLabel* status_label_ = nullptr;
  QLabel* workspace_label_ = nullptr;
  QLabel* diagnostics_label_ = nullptr;
  QLabel* result_state_label_ = nullptr;
  QLineEdit* action_id_input_ = nullptr;
  QLineEdit* live_method_input_ = nullptr;
  QLineEdit* live_payload_input_ = nullptr;
  QPlainTextEdit* output_ = nullptr;
  UiMapProvider ui_map_provider_;
  SafeActionTrigger safe_action_trigger_;
  LiveQueryProvider live_query_provider_;
};
