#include "object_browser_panel.hpp"

#include <QListWidgetItem>
#include <QVBoxLayout>

namespace {

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QString netText(const std::string& net_id) {
  return net_id.empty() ? "net --" : "net " + qstr(net_id);
}

QString layerText(const std::string& layer_id) {
  return layer_id.empty() ? "layer --" : "layer " + qstr(layer_id);
}

}  // namespace

ObjectBrowserPanel::ObjectBrowserPanel(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  list_ = new QListWidget(this);
  list_->setObjectName("objectBrowserPanel");
  layout->addWidget(list_);

  renderScene(ccad::CanvasScene{});
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

  const int object_count = static_cast<int>(scene.pads.size() + scene.vias.size() +
                                            scene.tracks.size() + scene.keepouts.size());
  addSection("Objects (" + QString::number(object_count) + ")");
  for (const ccad::CanvasPad& pad : scene.pads) {
    addRow("pad " + qstr(pad.id) + "  " + netText(pad.net_id) + "  " + layerText(pad.layer_id));
  }
  for (const ccad::CanvasVia& via : scene.vias) {
    addRow("via " + qstr(via.id) + "  " + netText(via.net_id));
  }
  for (const ccad::CanvasTrack& track : scene.tracks) {
    addRow("track " + qstr(track.id) + "  " + netText(track.net_id) + "  " +
           layerText(track.layer_id));
  }
  for (const ccad::CanvasKeepout& keepout : scene.keepouts) {
    addRow("keepout " + qstr(keepout.id) + "  kind " + qstr(keepout.kind));
  }
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

void ObjectBrowserPanel::addSection(const QString& text) {
  auto* item = new QListWidgetItem(text, list_);
  QFont font = item->font();
  font.setBold(true);
  item->setFont(font);
}

void ObjectBrowserPanel::addRow(const QString& text) {
  list_->addItem(text);
}
