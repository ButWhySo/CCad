#pragma once

#include <QDialog>
#include <QNetworkAccessManager>

class QListWidget;
class QNetworkReply;
class QLineEdit;
class QWidget;
class AgentPanel;

class AgentMarketplaceDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AgentMarketplaceDialog(AgentPanel* agent_panel, QWidget* parent = nullptr);

 private slots:
  void onCatalogFetched(QNetworkReply* reply);
  void toggleSidebar();
  void openSettings();

 private:
  void setupUi();

  AgentPanel* agent_panel_{nullptr};
  QListWidget* list_widget_{nullptr};
  QNetworkAccessManager* network_manager_{nullptr};

  QWidget* sidebar_widget_{nullptr};
  QLineEdit* search_bar_{nullptr};
};
