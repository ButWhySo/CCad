#include "selection_inspector_panel.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <cmath>

namespace {

QString formatLength(ccad::Length length) {
  double mm = static_cast<double>(length.nanometers) / 1000000.0;
  double mil = static_cast<double>(length.nanometers) / 25400.0;
  return QString::number(mm, 'f', 2) + " mm (" + QString::number(mil, 'f', 2) + " mil)";
}

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
  clearExtraRows();
  setRow("Type", "--");
  setRow("ID", "--");
}

void SelectionInspectorPanel::renderSelection(const QString& type, const QString& id) {
  title_->setText(displayType(type) + " " + id);
  detail_->setText("Selected canvas object identity.");
  clearExtraRows();
  setRow("Type", type);
  setRow("ID", id);
}

void SelectionInspectorPanel::renderSelection(const std::optional<ccad::Board>& board, const QString& type, const QString& id) {
  renderSelection(type, id);

  if (!board.has_value()) {
    return;
  }

  if (type == "pad") {
    for (const auto& pad : board->pads) {
      if (QString::fromStdString(pad.id) == id) {
        setRow("Position X", formatLength(pad.position.x));
        setRow("Position Y", formatLength(pad.position.y));
        setRow("Width", formatLength(pad.size.width));
        setRow("Height", formatLength(pad.size.height));
        setRow("Rotation", QString::number(pad.rotation_degrees, 'f', 1) + "°");
        setRow("Net", pad.net_id.empty() ? "--" : QString::fromStdString(pad.net_id));
        setRow("Layer", QString::fromStdString(pad.layer_id));
        setRow("Component ID", QString::fromStdString(pad.component_id));
        setRow("Pin Name", QString::fromStdString(pad.pin_name));
        break;
      }
    }
  } else if (type == "via") {
    for (const auto& via : board->vias) {
      if (QString::fromStdString(via.id) == id) {
        setRow("Position X", formatLength(via.position.x));
        setRow("Position Y", formatLength(via.position.y));
        setRow("Diameter", formatLength(via.diameter));
        setRow("Drill", formatLength(via.drill));
        setRow("Net", via.net_id.empty() ? "--" : QString::fromStdString(via.net_id));
        break;
      }
    }
  } else if (type == "track") {
    for (const auto& track : board->tracks) {
      if (QString::fromStdString(track.id) == id) {
        setRow("Start X", formatLength(track.start.x));
        setRow("Start Y", formatLength(track.start.y));
        setRow("End X", formatLength(track.end.x));
        setRow("End Y", formatLength(track.end.y));
        setRow("Width", formatLength(track.width));
        double dx = static_cast<double>(track.end.x.nanometers - track.start.x.nanometers);
        double dy = static_cast<double>(track.end.y.nanometers - track.start.y.nanometers);
        double length_val = std::hypot(dx, dy);
        setRow("Length", formatLength(ccad::nanometers(static_cast<int64_t>(length_val))));
        setRow("Net", track.net_id.empty() ? "--" : QString::fromStdString(track.net_id));
        setRow("Layer", QString::fromStdString(track.layer_id));
        setRow("Source Route Request", track.source_route_request_id.empty() ? "--" : QString::fromStdString(track.source_route_request_id));
        break;
      }
    }
  } else if (type == "keepout") {
    for (const auto& keepout : board->keepouts) {
      if (QString::fromStdString(keepout.id) == id) {
        setRow("Origin X", formatLength(keepout.area.origin.x));
        setRow("Origin Y", formatLength(keepout.area.origin.y));
        setRow("Width", formatLength(keepout.area.size.width));
        setRow("Height", formatLength(keepout.area.size.height));
        setRow("Kind", QString::fromStdString(keepout.kind));
        break;
      }
    }
  } else if (type == "placement_region") {
    for (const auto& pr : board->placement_regions) {
      if (QString::fromStdString(pr.id) == id) {
        setRow("Origin X", formatLength(pr.area.origin.x));
        setRow("Origin Y", formatLength(pr.area.origin.y));
        setRow("Width", formatLength(pr.area.size.width));
        setRow("Height", formatLength(pr.area.size.height));
        setRow("Kind", QString::fromStdString(pr.kind));
        break;
      }
    }
  }
}

void SelectionInspectorPanel::renderCanvasItem() {
  title_->setText("Canvas item");
  detail_->setText("Selected item has no stable CCad object identity.");
  clearExtraRows();
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

void SelectionInspectorPanel::clearExtraRows() {
  if (form_ != nullptr) {
    QLayoutItem* item;
    while ((item = form_->takeAt(0)) != nullptr) {
      if (item->widget() != nullptr) {
        delete item->widget();
      }
      delete item;
    }
  }
  rows_.clear();
}
