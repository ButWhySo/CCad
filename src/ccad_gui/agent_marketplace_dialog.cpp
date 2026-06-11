#include "agent_marketplace_dialog.hpp"
#include "agent_settings_dialog.hpp"
#include "agent_panel.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QLineEdit>
#include <QToolButton>
#include <QSplitter>

AgentMarketplaceDialog::AgentMarketplaceDialog(AgentPanel* agent_panel, QWidget* parent) 
    : QDialog(parent), agent_panel_(agent_panel) {
  setWindowTitle("Agent Marketplace (Live)");
  resize(800, 500);

  setStyleSheet(R"(
    QDialog {
      background-color: #0d1117;
      color: #e6edf3;
      font-family: 'Inter', sans-serif;
    }
    QListWidget {
      background-color: #0d1117;
      border: 1px solid #30363d;
      border-radius: 6px;
      color: #e6edf3;
      padding: 4px;
      font-size: 13px;
    }
    QListWidget::item {
      padding: 10px;
      border-bottom: 1px solid #21262d;
    }
    QListWidget::item:selected {
      background-color: rgba(138, 43, 226, 0.15);
      border-left: 3px solid #8a2be2;
    }
    QLabel {
      color: #e6edf3;
      font-size: 13px;
    }
    QLineEdit {
      background-color: #010409;
      border: 1px solid #30363d;
      border-radius: 6px;
      padding: 8px;
      color: #e6edf3;
    }
    QToolButton {
      background-color: #21262d;
      border: 1px solid #30363d;
      border-radius: 6px;
      color: #e6edf3;
      padding: 8px;
    }
    QToolButton:hover {
      background-color: #30363d;
      border-color: #8a2be2;
    }
    QWidget#sidebarWidget {
      background-color: #0d1117;
      border-right: 1px solid #30363d;
    }
  )");

  setupUi();

  network_manager_ = new QNetworkAccessManager(this);
  connect(network_manager_, &QNetworkAccessManager::finished, this, &AgentMarketplaceDialog::onCatalogFetched);

  QNetworkRequest request(QUrl("https://raw.githubusercontent.com/antigravity-ide/ccad-marketplace/main/catalog.json"));
  network_manager_->get(request);
}

void AgentMarketplaceDialog::setupUi() {
  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(0, 0, 0, 0);
  main_layout->setSpacing(0);

  // Top Bar
  auto* top_bar = new QWidget(this);
  auto* top_bar_layout = new QHBoxLayout(top_bar);
  top_bar_layout->setContentsMargins(10, 10, 10, 10);

  auto* burger_btn = new QToolButton(top_bar);
  burger_btn->setObjectName("action:burgerMenu");
  burger_btn->setText("≡");
  connect(burger_btn, &QToolButton::clicked, this, &AgentMarketplaceDialog::toggleSidebar);

  search_bar_ = new QLineEdit(top_bar);
  search_bar_->setObjectName("control:searchBar");
  search_bar_->setPlaceholderText("Search workflows, developer prompts, user prompts, hooks...");

  auto* settings_btn = new QToolButton(top_bar);
  settings_btn->setObjectName("action:settings");
  settings_btn->setText("⚙");
  connect(settings_btn, &QToolButton::clicked, this, &AgentMarketplaceDialog::openSettings);

  top_bar_layout->addWidget(burger_btn);
  top_bar_layout->addWidget(search_bar_);
  top_bar_layout->addWidget(settings_btn);

  main_layout->addWidget(top_bar);

  // Splitter for Sidebar and Main Content
  auto* splitter = new QSplitter(Qt::Horizontal, this);
  main_layout->addWidget(splitter);

  // Sidebar
  sidebar_widget_ = new QWidget(splitter);
  sidebar_widget_->setObjectName("sidebarWidget");
  auto* sidebar_layout = new QVBoxLayout(sidebar_widget_);
  sidebar_layout->setContentsMargins(0, 0, 0, 0);

  auto* sidebar_list = new QListWidget(sidebar_widget_);
  sidebar_list->addItem("Workflows");
  sidebar_list->addItem("Dev Prompts");
  sidebar_list->addItem("Tools");
  sidebar_list->addItem("Hooks");
  sidebar_list->addItem("Home");
  sidebar_layout->addWidget(sidebar_list);

  // Initially hide the sidebar
  sidebar_widget_->hide();
  splitter->addWidget(sidebar_widget_);

  // Main Content
  auto* content_widget = new QWidget(splitter);
  auto* content_layout = new QVBoxLayout(content_widget);

  auto* top_used_label = new QLabel("<b>Top Used Plugins & Hooks</b>", content_widget);
  content_layout->addWidget(top_used_label);

  list_widget_ = new QListWidget(content_widget);
  list_widget_->addItem("Loading catalog from live URL...");

  // Removed hardcoded parity stubs, fetching dynamically.

  content_layout->addWidget(list_widget_);

  splitter->addWidget(content_widget);

  // Set sizes
  splitter->setSizes({200, 600});
}

void AgentMarketplaceDialog::toggleSidebar() {
  if (sidebar_widget_) {
    sidebar_widget_->setVisible(!sidebar_widget_->isVisible());
  }
}

void AgentMarketplaceDialog::openSettings() {
  AgentSettingsDialog settings_dialog(agent_panel_, this);
  settings_dialog.exec();
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
  }
  reply->deleteLater();
}
