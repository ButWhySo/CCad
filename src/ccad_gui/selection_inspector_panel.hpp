#pragma once

#include "ccad_core/model.hpp"

#include <QLabel>
#include <QMap>
#include <QString>
#include <QWidget>

#include <optional>

class QFormLayout;

class SelectionInspectorPanel final : public QWidget {
 public:
  explicit SelectionInspectorPanel(QWidget* parent = nullptr);

  void clearSelection();
  void renderSelection(const QString& type, const QString& id);
  void renderSelection(const std::optional<ccad::Board>& board, const QString& type, const QString& id);
  void renderCanvasItem();

  QString titleText() const;
  QString detailText() const;
  QString rowText(const QString& label) const;

 private:
  void setRow(const QString& label, const QString& value);
  void clearExtraRows();

  QLabel* title_ = nullptr;
  QLabel* detail_ = nullptr;
  QFormLayout* form_ = nullptr;
  QMap<QString, QLabel*> rows_;
};
