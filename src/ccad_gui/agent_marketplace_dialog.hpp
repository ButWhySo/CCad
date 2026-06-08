#pragma once

#include <QDialog>
#include <QNetworkAccessManager>

class QListWidget;
class QNetworkReply;

class AgentMarketplaceDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AgentMarketplaceDialog(QWidget* parent = nullptr);

 private slots:
  void onCatalogFetched(QNetworkReply* reply);

 private:
  QListWidget* list_widget_{nullptr};
  QNetworkAccessManager* network_manager_{nullptr};
};
