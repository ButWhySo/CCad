#include "agent_marketplace_dialog.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>

AgentMarketplaceDialog::AgentMarketplaceDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle("Agent Marketplace (Live)");
  resize(500, 400);
  
  auto* layout = new QVBoxLayout(this);
  list_widget_ = new QListWidget(this);
  layout->addWidget(list_widget_);

  list_widget_->addItem("Loading catalog from live URL...");

  network_manager_ = new QNetworkAccessManager(this);
  connect(network_manager_, &QNetworkAccessManager::finished, this, &AgentMarketplaceDialog::onCatalogFetched);

  // Fallback to a mock JSON URL or a real one if available
  QNetworkRequest request(QUrl("https://raw.githubusercontent.com/antigravity-ide/ccad-marketplace/main/catalog.json"));
  network_manager_->get(request);
}

void AgentMarketplaceDialog::onCatalogFetched(QNetworkReply* reply) {
  list_widget_->clear();
  if (reply->error() == QNetworkReply::NoError) {
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isArray()) {
      QJsonArray array = doc.array();
      for (const QJsonValue& val : array) {
        if (val.isObject()) {
          QJsonObject obj = val.toObject();
          QString name = obj.value("name").toString();
          QString desc = obj.value("description").toString();
          list_widget_->addItem(name + " - " + desc);
        }
      }
    } else {
      list_widget_->addItem("Failed to parse catalog JSON.");
    }
  } else {
    list_widget_->addItem("Network error: " + reply->errorString());
    list_widget_->addItem("Fallback mock plugin: 'Kicad BOM Exporter' - Exports BOMs");
    list_widget_->addItem("Fallback mock plugin: 'Auto-Router' - Basic net routing");
  }
  reply->deleteLater();
}
