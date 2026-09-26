#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QPointer>
#include <QWidget>
#include <QString>

class QListWidget;
class QTableWidget;
class QDialog;
class QStackedWidget;
class QVBoxLayout;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QTextEdit;
class QPushButton;
class QLabel;
class AgentPanel;

class AgentSettingsDialog : public QDialog {
  Q_OBJECT

public:
  explicit AgentSettingsDialog(AgentPanel* agent_panel, QWidget* parent = nullptr);
  ~AgentSettingsDialog() override;

private:
  void setupUi();
  void createGeneralTab(QWidget* parent_widget);
  void createConfigurationTab(QWidget* parent_widget);
  void createPersonalisationTab(QWidget* parent_widget);
  void createMCPTab(QWidget* parent_widget);
  void createAPIProvidersTab(QWidget* parent_widget);
  void createObservabilityTab(QWidget* parent_widget);
  void createPluginsTab(QWidget* parent_widget);
  void createWorkflowsTab(QWidget* parent_widget);

  void loadCurrentSettings();
  void saveAllSettings();

  void applyConfigState(const QJsonObject& config);
  void applyMarketplaceCatalog(const QJsonObject& catalog);
  void applyModelCatalog(const QJsonObject& catalog);
  void openMemoryManager();
  void applyMemoryState(const QJsonObject& state);
  void applyMemoryOperation(const QString& method, const QJsonObject& result);

  AgentPanel* agent_panel_;
  QListWidget* category_list_;
  QStackedWidget* stacked_widget_;

  // Config tab
  QComboBox* provider_combo_{nullptr};
  QComboBox* model_combo_{nullptr};
  QLineEdit* model_input_{nullptr};
  QLabel* model_details_{nullptr};
  QTextEdit* resolved_config_preview_{nullptr};
  QLineEdit* api_key_input_{nullptr};
  QLabel* provider_target_label_{nullptr};
  QLabel* provider_status_label_{nullptr};
  QCheckBox* langfuse_enabled_cb_{nullptr};
  QLineEdit* langfuse_public_key_input_{nullptr};
  QLineEdit* langfuse_secret_key_input_{nullptr};
  QLineEdit* langfuse_base_url_input_{nullptr};
  QLineEdit* langfuse_environment_input_{nullptr};
  QLineEdit* langfuse_service_name_input_{nullptr};
  QLabel* langfuse_status_label_{nullptr};
  QLabel* mcp_runtime_status_label_{nullptr};
  QCheckBox* sandbox_cb_{nullptr};
  QCheckBox* approval_cb_{nullptr};
  QLineEdit* project_name_{nullptr};
  QLineEdit* project_path_{nullptr};
  QComboBox* trust_level_{nullptr};
  QCheckBox* stm_cb_{nullptr};
  QCheckBox* ltm_cb_{nullptr};
  QCheckBox* episodic_cb_{nullptr};
  QCheckBox* semantic_memory_enabled_cb_{nullptr};
  QLineEdit* semantic_memory_endpoint_{nullptr};
  QLineEdit* semantic_memory_model_{nullptr};
  QLabel* memory_status_label_{nullptr};
  QLabel* semantic_memory_status_label_{nullptr};
  QLabel* memory_operation_status_label_{nullptr};
  QLabel* memory_manager_status_label_{nullptr};
  QPushButton* memory_reset_button_{nullptr};
  QPushButton* memory_save_button_{nullptr};
  QPushButton* memory_delete_button_{nullptr};
  QPointer<QDialog> memory_dialog_;
  QListWidget* memory_entries_{nullptr};
  QTextEdit* memory_content_{nullptr};
  QLineEdit* memory_title_{nullptr};
  QLineEdit* memory_scope_{nullptr};
  QComboBox* memory_tier_{nullptr};
  QComboBox* memory_kind_{nullptr};
  QComboBox* memory_importance_{nullptr};
  QComboBox* hooks_combo_{nullptr};

  // General settings
  QComboBox* theme_combo_{nullptr};
  QComboBox* grid_combo_{nullptr};
  QCheckBox* autosave_cb_{nullptr};
  QCheckBox* restore_session_cb_{nullptr};
  bool grid_user_modified_{false};
  QString last_loaded_grid_;

  // Personalisation tab
  QLineEdit* follow_up_{nullptr};
  QCheckBox* context_window_{nullptr};
  QComboBox* inline_detached_{nullptr};
  QComboBox* agent_personality_{nullptr};
  QTextEdit* custom_instructions_{nullptr};

  // Workflows tab
  QListWidget* workflows_list_{nullptr};
  QTextEdit* system_prompt_{nullptr};
  QTextEdit* dev_prompt_{nullptr};

  // Plugins tab
  QListWidget* plugins_list_{nullptr};
  QTableWidget* mcp_servers_table_{nullptr};

  // Bottom buttons
  QPushButton* save_btn_{nullptr};
  QPushButton* cancel_btn_{nullptr};
};
