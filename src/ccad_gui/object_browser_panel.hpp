#pragma once

#include "ccad_core/canvas.hpp"

#include <QListWidget>
#include <QString>
#include <QWidget>

class ObjectBrowserPanel final : public QWidget {
 public:
  explicit ObjectBrowserPanel(QWidget* parent = nullptr);

  void renderScene(const ccad::CanvasScene& scene);

  int itemCount() const;
  QString itemText(int row) const;

 private:
  void addSection(const QString& text);
  void addRow(const QString& text);

  QListWidget* list_ = nullptr;
};
