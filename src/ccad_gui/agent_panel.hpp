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

  explicit AgentPanel(QWidget* parent = nullptr);

  void setUiMapProvider(UiMapProvider provider);
  void setSafeActionTrigger(SafeActionTrigger trigger);
  void setProjectContext(const QString& project_label, int ui_map_epoch);
  void setActionId(const QString& action_id);

  void refreshUiMap();
  void triggerSafeAction();

  QString projectText() const;
  QString epochText() const;
  QString statusText() const;
  QString actionIdText() const;
  QString outputText() const;

 private:
  QLabel* project_label_ = nullptr;
  QLabel* epoch_label_ = nullptr;
  QLabel* status_label_ = nullptr;
  QLineEdit* action_id_input_ = nullptr;
  QPlainTextEdit* output_ = nullptr;
  UiMapProvider ui_map_provider_;
  SafeActionTrigger safe_action_trigger_;
};
