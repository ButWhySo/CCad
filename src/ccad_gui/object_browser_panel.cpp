#include "object_browser_panel.hpp"
#include "board_canvas_renderer.hpp"

#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QPixmap>
#include <QIcon>
#include <QPainter>

#include <map>
#include <utility>

namespace {

constexpr int kObjectIdRole = Qt::UserRole;
constexpr int kNetIdRole = Qt::UserRole + 1;
constexpr int kRouteRequestIdRole = Qt::UserRole + 2;
constexpr int kLayerIdRole = Qt::UserRole + 3;

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

QIcon createColorSwatchIcon(const QColor& color) {
  QPixmap pixmap(12, 12);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setBrush(color);
  painter.setPen(Qt::NoPen);
  painter.drawRoundedRect(0, 0, 12, 12, 2, 2);
  return QIcon(pixmap);
}

}  // namespace

ObjectBrowserPanel::ObjectBrowserPanel(QWidget* parent) : QWidget(parent) {
  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  tabs_ = new QTabWidget(this);
  tabs_->setObjectName("appearanceTabs");
  layout->addWidget(tabs_);

  layers_list_ = new QListWidget(this);
  layers_list_->setObjectName("layersList");
  tabs_->addTab(layers_list_, "Layers");

  objects_list_ = new QListWidget(this);
  objects_list_->setObjectName("objectsList");
  tabs_->addTab(objects_list_, "Objects");

  nets_list_ = new QListWidget(this);
  nets_list_->setObjectName("netsList");
  tabs_->addTab(nets_list_, "Nets");

  auto setup_list = [this](QListWidget* list) {
    connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
      if (item == nullptr) return;
      
      const QString layer_id = item->data(kLayerIdRole).toString();
      if (!layer_id.isEmpty() && layer_activated_callback_) {
        layer_activated_callback_(layer_id);
      }

      const QString net_id = item->data(kNetIdRole).toString();
      if (!net_id.isEmpty() && net_activated_callback_) {
        net_activated_callback_(net_id);
        return;
      }
      const QString route_request_id = item->data(kRouteRequestIdRole).toString();
      if (!route_request_id.isEmpty() && route_activated_callback_) {
        route_activated_callback_(route_request_id);
        return;
      }

      const QString object_id = item->data(kObjectIdRole).toString();
      if (!object_id.isEmpty() && object_activated_callback_) {
        object_activated_callback_(object_id);
      }
    });

    connect(list, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) {
      if (item == nullptr || !layer_toggled_callback_) return;
      
      const QString layer_id = item->data(kLayerIdRole).toString();
      if (!layer_id.isEmpty()) {
        const bool visible = (item->checkState() == Qt::Checked);
        layer_toggled_callback_(layer_id, visible);
      }
    });
  };

  setup_list(layers_list_);
  setup_list(objects_list_);
  setup_list(nets_list_);
}

void ObjectBrowserPanel::renderScene(const ccad::CanvasScene& scene) {
  layers_list_->blockSignals(true);
  objects_list_->blockSignals(true);
  nets_list_->blockSignals(true);
  
  layers_list_->clear();
  objects_list_->clear();
  nets_list_->clear();
  
  if (!scene.has_board) {
    addRow(layers_list_, "No board objects");
    layers_list_->blockSignals(false);
    objects_list_->blockSignals(false);
    nets_list_->blockSignals(false);
    return;
  }

  CanvasRenderTheme theme;

  addSection(layers_list_, "Layers (" + QString::number(static_cast<int>(scene.layers.size())) + ")");
  for (const ccad::CanvasLayer& layer : scene.layers) {
    auto* item = addRow(layers_list_, qstr(layer.name) + " [" + qstr(layer.kind) + "]");
    item->setData(kLayerIdRole, qstr(layer.id));
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setCheckState(layer.visible ? Qt::Checked : Qt::Unchecked);
    item->setIcon(createColorSwatchIcon(colorForKiCadLayer(theme, layer.id)));
  }

  std::map<std::string, int> net_counts;
  for (const ccad::CanvasPad& pad : scene.pads) countNet(net_counts, pad.net_id);
  for (const ccad::CanvasVia& via : scene.vias) countNet(net_counts, via.net_id);
  for (const ccad::CanvasTrack& track : scene.tracks) countNet(net_counts, track.net_id);
  for (const ccad::CanvasZone& zone : scene.zones) countNet(net_counts, zone.net_id);
  
  addSection(nets_list_, "Nets (" + QString::number(static_cast<int>(net_counts.size())) + ")");
  for (const auto& [net_id, count] : net_counts) {
    auto* item = addRow(nets_list_, "net " + qstr(net_id) + "  objects " + QString::number(count), {}, qstr(net_id));
    // Could generate consistent random color per net, but for now we skip net colors or use track color
    item->setIcon(createColorSwatchIcon(theme.track_color));
  }

  addSection(nets_list_, "Route Requests (" + QString::number(static_cast<int>(scene.route_requests.size())) + ")");
  for (const ccad::CanvasRouteRequest& request : scene.route_requests) {
    const QString progress = request.routed_segment_count > 0 ? "partial" : "open";
    addRow(nets_list_, "route " + qstr(request.id) + "  net " + qstr(request.net_id) + "  " + progress, {}, {}, qstr(request.id));
  }

  const int object_count = static_cast<int>(scene.pads.size() + scene.vias.size() +
                                            scene.tracks.size() + scene.keepouts.size() +
                                            scene.placement_regions.size() + scene.lines.size() +
                                            scene.texts.size() + scene.zones.size());
  addSection(objects_list_, "Objects (" + QString::number(object_count) + ")");
  for (const ccad::CanvasPad& pad : scene.pads) {
    addRow(objects_list_, "pad " + qstr(pad.id) + "  " + netText(pad.net_id), qstr(pad.id));
  }
  for (const ccad::CanvasVia& via : scene.vias) {
    addRow(objects_list_, "via " + qstr(via.id) + "  " + netText(via.net_id), qstr(via.id));
  }
  for (const ccad::CanvasTrack& track : scene.tracks) {
    addRow(objects_list_, "track " + qstr(track.id) + "  " + netText(track.net_id) + "  " + layerText(track.layer_id), qstr(track.id));
  }
  for (const ccad::CanvasLine& line : scene.lines) {
    addRow(objects_list_, "graphic " + qstr(line.id) + "  " + layerText(line.layer_id), qstr(line.id));
  }
  for (const ccad::CanvasText& text : scene.texts) {
    addRow(objects_list_, "text " + qstr(text.id) + "  " + layerText(text.layer_id) + "  " + qstr(text.text), qstr(text.id));
  }
  for (const ccad::CanvasZone& zone : scene.zones) {
    addRow(objects_list_, "zone " + qstr(zone.id) + "  " + netText(zone.net_id), qstr(zone.id));
  }
  for (const ccad::CanvasKeepout& keepout : scene.keepouts) {
    addRow(objects_list_, "keepout " + qstr(keepout.id) + "  kind " + qstr(keepout.kind), qstr(keepout.id));
  }
  for (const ccad::CanvasPlacementRegion& region : scene.placement_regions) {
    addRow(objects_list_, "placement-region " + qstr(region.id) + "  kind " + qstr(region.kind), qstr(region.id));
  }
  
  layers_list_->blockSignals(false);
  objects_list_->blockSignals(false);
  nets_list_->blockSignals(false);
}

void ObjectBrowserPanel::setObjectActivatedCallback(std::function<void(QString)> callback) {
  object_activated_callback_ = std::move(callback);
}

void ObjectBrowserPanel::setNetActivatedCallback(std::function<void(QString)> callback) {
  net_activated_callback_ = std::move(callback);
}

void ObjectBrowserPanel::setRouteActivatedCallback(std::function<void(QString)> callback) {
  route_activated_callback_ = std::move(callback);
}

void ObjectBrowserPanel::setLayerToggledCallback(std::function<void(QString, bool)> callback) {
  layer_toggled_callback_ = std::move(callback);
}

void ObjectBrowserPanel::setLayerActivatedCallback(std::function<void(QString)> callback) {
  layer_activated_callback_ = std::move(callback);
}

int ObjectBrowserPanel::itemCount() const {
  return layers_list_->count() + objects_list_->count() + nets_list_->count();
}

QString ObjectBrowserPanel::itemText(const int /*row*/) const {
  return {}; // Not robust across multiple lists, left empty or implement list finding
}

QString ObjectBrowserPanel::objectIdForRow(const int /*row*/) const {
  return {}; // Not robust across multiple lists
}

QString ObjectBrowserPanel::netIdForRow(const int /*row*/) const {
  return {}; // Not robust across multiple lists
}

void ObjectBrowserPanel::addSection(QListWidget* list, const QString& text) {
  auto* item = new QListWidgetItem(text, list);
  QFont font = item->font();
  font.setBold(true);
  item->setFont(font);
  item->setFlags(Qt::NoItemFlags); // sections are not selectable
}

QListWidgetItem* ObjectBrowserPanel::addRow(QListWidget* list, const QString& text, const QString& object_id,
                                            const QString& net_id, const QString& route_request_id) {
  auto* item = new QListWidgetItem(text, list);
  if (!object_id.isEmpty()) {
    item->setData(kObjectIdRole, object_id);
  }
  if (!net_id.isEmpty()) {
    item->setData(kNetIdRole, net_id);
  }
  if (!route_request_id.isEmpty()) {
    item->setData(kRouteRequestIdRole, route_request_id);
  }
  return item;
}
