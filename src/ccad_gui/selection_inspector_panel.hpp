#pragma once

#include <QLabel>
#include <QMap>
#include <QString>
#include <QWidget>

class QFormLayout;

class SelectionInspectorPanel final : public QWidget {
 public:
  explicit SelectionInspectorPanel(QWidget* parent = nullptr);

  void clearSelection();
  void renderSelection(const QString& type, const QString& id);
  void renderCanvasItem();

  QString titleText() const;
  QString detailText() const;
  QString rowText(const QString& label) const;

 private:
  void setRow(const QString& label, const QString& value);

  QLabel* title_ = nullptr;
  QLabel* detail_ = nullptr;
  QFormLayout* form_ = nullptr;
  QMap<QString, QLabel*> rows_;
};
