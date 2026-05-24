#include "object_browser_panel.hpp"

#include <QListWidgetItem>
#include <QVBoxLayout>

#include <map>
#include <utility>

namespace {

constexpr int kObjectIdRole = Qt::UserRole;
constexpr int kNetIdRole = Qt::UserRole + 1;

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QString netText(const std::string& net_id) {
  return net_id.empty() ? "net --" : "net " + qstr(net_id);
}

QString layerText(const std::string& layer_id) {
  return layer_id.empty() ? "layer --" : "layer " + qstr(layer_id);
}

void countNet(std::map<std::string, int>& net_counts, const std::string& net_id) {
  if (!net_id.empty()) {
    ++net_counts[net_id];
  }
}

}  // namespace

ObjectBrowserPanel::ObjectBrowserPanel(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  list_ = new QListWidget(this);
  list_->setObjectName("objectBrowserPanel");
  layout->addWidget(list_);
  connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
    if (item == nullptr || !object_activated_callback_) {
      return;
    }
    const QString net_id = item->data(kNetIdRole).toString();
    if (!net_id.isEmpty() && net_activated_callback_) {
      net_activated_callback_(net_id);
      return;
    }

    const QString object_id = item->data(kObjectIdRole).toString();
    if (!object_id.isEmpty()) {
      object_activated_callback_(object_id);
    }
  });
}

void ObjectBrowserPanel::renderScene(const ccad::CanvasScene& scene) {
  list_->clear();
  if (!scene.has_board) {
    addRow("No board objects");
    return;
  }

  addSection("Layers (" + QString::number(static_cast<int>(scene.layers.size())) + ")");
  for (const ccad::CanvasLayer& layer : scene.layers) {
    addRow(qstr(layer.id) + " - " + qstr(layer.name) + " [" + qstr(layer.kind) + "]");
  }

  std::map<std::string, int> net_counts;
  for (const ccad::CanvasPad& pad : scene.pads) {
    countNet(net_counts, pad.net_id);
  }
  for (const ccad::CanvasVia& via : scene.vias) {
    countNet(net_counts, via.net_id);
  }
  for (const ccad::CanvasTrack& track : scene.tracks) {
    countNet(net_counts, track.net_id);
  }
  addSection("Nets (" + QString::number(static_cast<int>(net_counts.size())) + ")");
  for (const auto& [net_id, count] : net_counts) {
    addRow("net " + qstr(net_id) + "  objects " + QString::number(count), {}, qstr(net_id));
  }

  const int object_count = static_cast<int>(scene.pads.size() + scene.vias.size() +
                                            scene.tracks.size() + scene.keepouts.size() +
                                            scene.placement_regions.size());
  addSection("Objects (" + QString::number(object_count) + ")");
  for (const ccad::CanvasPad& pad : scene.pads) {
    addRow("pad " + qstr(pad.id) + "  " + netText(pad.net_id) + "  " +
               layerText(pad.layer_id),
           qstr(pad.id));
  }
  for (const ccad::CanvasVia& via : scene.vias) {
    addRow("via " + qstr(via.id) + "  " + netText(via.net_id), qstr(via.id));
  }
  for (const ccad::CanvasTrack& track : scene.tracks) {
    addRow("track " + qstr(track.id) + "  " + netText(track.net_id) + "  " +
               layerText(track.layer_id),
           qstr(track.id));
  }
  for (const ccad::CanvasKeepout& keepout : scene.keepouts) {
    addRow("keepout " + qstr(keepout.id) + "  kind " + qstr(keepout.kind), qstr(keepout.id));
  }
  for (const ccad::CanvasPlacementRegion& region : scene.placement_regions) {
    addRow("placement-region " + qstr(region.id) + "  kind " + qstr(region.kind),
           qstr(region.id));
  }
}

void ObjectBrowserPanel::setObjectActivatedCallback(std::function<void(QString)> callback) {
  object_activated_callback_ = std::move(callback);
}

void ObjectBrowserPanel::setNetActivatedCallback(std::function<void(QString)> callback) {
  net_activated_callback_ = std::move(callback);
}

int ObjectBrowserPanel::itemCount() const {
  return list_->count();
}

QString ObjectBrowserPanel::itemText(const int row) const {
  const QListWidgetItem* item = list_->item(row);
  if (item == nullptr) {
    return {};
  }
  return item->text();
}

QString ObjectBrowserPanel::objectIdForRow(const int row) const {
  const QListWidgetItem* item = list_->item(row);
  if (item == nullptr) {
    return {};
  }
  return item->data(kObjectIdRole).toString();
}

QString ObjectBrowserPanel::netIdForRow(const int row) const {
  const QListWidgetItem* item = list_->item(row);
  if (item == nullptr) {
    return {};
  }
  return item->data(kNetIdRole).toString();
}

void ObjectBrowserPanel::addSection(const QString& text) {
  auto* item = new QListWidgetItem(text, list_);
  QFont font = item->font();
  font.setBold(true);
  item->setFont(font);
}

void ObjectBrowserPanel::addRow(const QString& text, const QString& object_id,
                                const QString& net_id) {
  auto* item = new QListWidgetItem(text, list_);
  if (!object_id.isEmpty()) {
    item->setData(kObjectIdRole, object_id);
  }
  if (!net_id.isEmpty()) {
    item->setData(kNetIdRole, net_id);
  }
}
