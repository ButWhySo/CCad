#include "selection_inspector_panel.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {

QString displayType(const QString& type) {
  if (type.isEmpty()) {
    return "--";
  }
  QString result = type;
  result[0] = result[0].toUpper();
  return result;
}

}  // namespace

SelectionInspectorPanel::SelectionInspectorPanel(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  title_ = new QLabel(this);
  title_->setObjectName("inspectorTitle");

  detail_ = new QLabel(this);
  detail_->setObjectName("inspectorDetail");
  detail_->setWordWrap(true);

  form_ = new QFormLayout();
  form_->setContentsMargins(0, 0, 0, 0);
  form_->setSpacing(6);

  layout->addWidget(title_);
  layout->addWidget(detail_);
  layout->addLayout(form_);
  layout->addStretch(1);

  setRow("Type", "--");
  setRow("ID", "--");
  clearSelection();
}

void SelectionInspectorPanel::clearSelection() {
  title_->setText("No selection");
  detail_->setText("Select a board object to inspect its stable identity.");
  setRow("Type", "--");
  setRow("ID", "--");
}

void SelectionInspectorPanel::renderSelection(const QString& type, const QString& id) {
  title_->setText(displayType(type) + " " + id);
  detail_->setText("Selected canvas object identity.");
  setRow("Type", type);
  setRow("ID", id);
}

void SelectionInspectorPanel::renderCanvasItem() {
  title_->setText("Canvas item");
  detail_->setText("Selected item has no stable CCad object identity.");
  setRow("Type", "--");
  setRow("ID", "--");
}

QString SelectionInspectorPanel::titleText() const {
  return title_->text();
}

QString SelectionInspectorPanel::detailText() const {
  return detail_->text();
}

QString SelectionInspectorPanel::rowText(const QString& label) const {
  const auto it = rows_.find(label);
  if (it == rows_.end()) {
    return {};
  }
  return (*it)->text();
}

void SelectionInspectorPanel::setRow(const QString& label, const QString& value) {
  auto it = rows_.find(label);
  if (it == rows_.end()) {
    auto* value_label = new QLabel(this);
    value_label->setObjectName("inspectorValue");
    form_->addRow(label + ":", value_label);
    it = rows_.insert(label, value_label);
  }
  (*it)->setText(value);
}
