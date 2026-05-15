#pragma once

#include "ccad_core/canvas.hpp"

#include <QListWidget>
#include <QString>
#include <QWidget>

#include <functional>

class ObjectBrowserPanel final : public QWidget {
 public:
  explicit ObjectBrowserPanel(QWidget* parent = nullptr);

  void renderScene(const ccad::CanvasScene& scene);
  void setObjectActivatedCallback(std::function<void(QString)> callback);

  int itemCount() const;
  QString itemText(int row) const;
  QString objectIdForRow(int row) const;

 private:
  void addSection(const QString& text);
  void addRow(const QString& text, const QString& object_id = {});

  QListWidget* list_ = nullptr;
  std::function<void(QString)> object_activated_callback_;
};
