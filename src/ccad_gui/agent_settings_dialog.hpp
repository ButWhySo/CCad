#pragma once

#include <QDialog>
#include <QJsonObject>
#include <QWidget>

class QListWidget;
class QStackedWidget;
class QVBoxLayout;
class QComboBox;
class QLineEdit;
class QCheckBox;
class QTextEdit;
class QPushButton;
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
  void createPluginsTab(QWidget* parent_widget);
  void createWorkflowsTab(QWidget* parent_widget);

  void loadCurrentSettings();
  void saveAllSettings();

  void applyConfigState(const QJsonObject& config);
  void applyMarketplaceCatalog(const QJsonObject& catalog);

  AgentPanel* agent_panel_;
  QListWidget* category_list_;
  QStackedWidget* stacked_widget_;

  // Config tab
  QComboBox* provider_combo_{nullptr};
  QLineEdit* model_input_{nullptr};
  QLineEdit* api_key_input_{nullptr};
  QCheckBox* sandbox_cb_{nullptr};
  QCheckBox* approval_cb_{nullptr};
  QLineEdit* project_name_{nullptr};
  QLineEdit* project_path_{nullptr};
  QComboBox* trust_level_{nullptr};
  QCheckBox* stm_cb_{nullptr};
  QCheckBox* ltm_cb_{nullptr};
  QCheckBox* episodic_cb_{nullptr};
  QComboBox* hooks_combo_{nullptr};

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

  // Bottom buttons
  QPushButton* save_btn_{nullptr};
  QPushButton* cancel_btn_{nullptr};
};
