#pragma once

#include <QDialog>
class QListWidget;
class QLineEdit;
class QWidget;
class AgentPanel;

class AgentMarketplaceDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AgentMarketplaceDialog(AgentPanel* agent_panel, QWidget* parent = nullptr);

 private slots:
  void toggleSidebar();
  void openSettings();

 private:
  void setupUi();

  AgentPanel* agent_panel_{nullptr};
  QListWidget* list_widget_{nullptr};

  QWidget* sidebar_widget_{nullptr};
  QLineEdit* search_bar_{nullptr};
};
