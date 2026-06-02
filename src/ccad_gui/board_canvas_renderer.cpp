#include "board_canvas_renderer.hpp"

#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QTransform>

#include <algorithm>
#include <optional>
#include <set>

namespace {

class ShapeHighlightPathItem final : public QGraphicsPathItem {
 public:
  using QGraphicsPathItem::QGraphicsPathItem;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
    QStyleOptionGraphicsItem clean_option(*option);
    clean_option.state &= ~QStyle::State_Selected;
    QGraphicsPathItem::paint(painter, &clean_option, widget);

    if (!isSelected()) {
      return;
    }

    QPen highlight_pen(canvasSelectionHighlightColor(*this));
    highlight_pen.setCosmetic(false);
    highlight_pen.setWidthF(canvasSelectionHighlightWidth(*this));
    highlight_pen.setJoinStyle(Qt::RoundJoin);
    highlight_pen.setCapStyle(Qt::RoundCap);
    painter->setPen(highlight_pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(path());
  }
};

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QColor lighterHighlight(const QColor& color) {
  return color.lighter(160);
}

void tagObject(QGraphicsItem& item, const QString& type, const QString& id,
               const QColor& display_color, const QString& net_id = {},
               const QString& layer_id = {}, const QString& route_request_id = {}) {
  item.setFlag(QGraphicsItem::ItemIsSelectable, true);
  item.setData(kCanvasObjectTypeRole, type);
  item.setData(kCanvasObjectIdRole, id);
  item.setData(kCanvasShapeSelectionHighlightRole, true);
  item.setData(kCanvasSelectionHighlightColorRole, lighterHighlight(display_color));
  double highlight_width = 2.5;
  if (type == "track") {
    if (auto* path_item = dynamic_cast<QGraphicsPathItem*>(&item)) {
      highlight_width = std::max(3.5, path_item->pen().widthF() + 2.0);
    }
  }
  item.setData(kCanvasSelectionHighlightWidthRole, highlight_width);
  if (!net_id.isEmpty()) {
    item.setData(kCanvasObjectNetIdRole, net_id);
  }
  if (!layer_id.isEmpty()) {
    item.setData(kCanvasObjectLayerIdRole, layer_id);
  }
  if (!route_request_id.isEmpty()) {
    item.setData(kCanvasObjectRouteRequestIdRole, route_request_id);
  }
}

ShapeHighlightPathItem* addHighlightPath(QGraphicsScene& canvas_scene, const QPainterPath& path,
                                         const QPen& pen, const QBrush& brush) {
  auto* item = new ShapeHighlightPathItem();
  item->setPath(path);
  item->setPen(pen);
  item->setBrush(brush);
  canvas_scene.addItem(item);
  return item;
}

QGraphicsItem* findCanvasObjectById(QGraphicsScene& canvas_scene, const QString& id) {
  if (id.isEmpty()) {
    return nullptr;
  }
  for (QGraphicsItem* item : canvas_scene.items()) {
    if (canvasObjectId(*item) == id) {
      return item;
    }
  }
  return nullptr;
}

QPainterPath trapezoidPath(const QRectF& rect) {
  const double inset = std::min(rect.width(), rect.height()) * 0.20;
  QPainterPath path;
  path.moveTo(rect.left() + inset, rect.top());
  path.lineTo(rect.right(), rect.top());
  path.lineTo(rect.right() - inset, rect.bottom());
  path.lineTo(rect.left(), rect.bottom());
  path.closeSubpath();
  return path;
}

QPainterPath chamferedRectPath(const QRectF& rect, const std::optional<double> chamfer_ratio) {
  const double ratio = std::clamp(chamfer_ratio.value_or(0.20), 0.0, 0.5);
  const double chamfer = std::min(rect.width(), rect.height()) * ratio;
  QPainterPath path;
  path.moveTo(rect.left() + chamfer, rect.top());
  path.lineTo(rect.right() - chamfer, rect.top());
  path.lineTo(rect.right(), rect.top() + chamfer);
  path.lineTo(rect.right(), rect.bottom() - chamfer);
  path.lineTo(rect.right() - chamfer, rect.bottom());
  path.lineTo(rect.left() + chamfer, rect.bottom());
  path.lineTo(rect.left(), rect.bottom() - chamfer);
  path.lineTo(rect.left(), rect.top() + chamfer);
  path.closeSubpath();
  return path;
}

QPainterPath padShapePath(const QRectF& pad_rect, const QPointF& pad_center,
                          const std::string& shape, const double rotation_degrees,
                          const std::optional<double> roundrect_rratio,
                          const std::optional<double> chamfer_ratio) {
  QPainterPath pad_path;
  if (shape == "circle") {
    const double diameter = std::min(pad_rect.width(), pad_rect.height());
    pad_path.addEllipse(QRectF(pad_center.x() - (diameter / 2.0),
                               pad_center.y() - (diameter / 2.0), diameter, diameter));
  } else if (shape == "oval") {
    pad_path.addEllipse(pad_rect);
  } else if (shape == "rect") {
    pad_path.addRect(pad_rect);
  } else if (shape == "roundrect" || shape == "rounded_rect") {
    const double ratio = std::clamp(roundrect_rratio.value_or(0.25), 0.0, 0.5);
    const double radius = std::min(pad_rect.width(), pad_rect.height()) * ratio;
    pad_path.addRoundedRect(pad_rect, radius, radius);
  } else if (shape == "trapezoid") {
    pad_path = trapezoidPath(pad_rect);
  } else if (shape == "chamfered_rect") {
    pad_path = chamferedRectPath(pad_rect, chamfer_ratio);
  } else {
    pad_path.addEllipse(pad_rect);
  }

  if (rotation_degrees != 0.0) {
    QTransform transform;
    transform.translate(pad_center.x(), pad_center.y());
    transform.rotate(rotation_degrees);
    transform.translate(-pad_center.x(), -pad_center.y());
    pad_path = transform.map(pad_path);
  }
  return pad_path;
}

std::set<std::string> hiddenLayerIds(const ccad::CanvasScene& scene) {
  std::set<std::string> hidden;
  for (const ccad::CanvasLayer& layer : scene.layers) {
    if (!layer.visible) {
      hidden.insert(layer.id);
    }
  }
  return hidden;
}

bool layerIsVisible(const std::set<std::string>& hidden_layers, const std::string& layer_id) {
  return layer_id.empty() || !hidden_layers.contains(layer_id);
}

bool wildcardLayerClassVisible(const std::set<std::string>& hidden_layers, const std::string& suffix) {
  if (suffix == ".Cu") {
    if (!hidden_layers.contains("F.Cu") || !hidden_layers.contains("B.Cu")) {
      return true;
    }
    for (int index = 1; index <= 30; ++index) {
      if (!hidden_layers.contains("In" + std::to_string(index) + ".Cu")) {
        return true;
      }
    }
    return false;
  }
  if (suffix == ".Mask") {
    return !hidden_layers.contains("F.Mask") || !hidden_layers.contains("B.Mask");
  }
  if (suffix == ".Paste") {
    return !hidden_layers.contains("F.Paste") || !hidden_layers.contains("B.Paste");
  }
  return true;
}

bool padLayerIsVisible(const std::set<std::string>& hidden_layers, const std::string& layer_id) {
  if (layer_id.starts_with("*.")) {
    return wildcardLayerClassVisible(hidden_layers, layer_id.substr(1));
  }
  return layerIsVisible(hidden_layers, layer_id);
}

bool padLayerMatches(const std::string& layer_id, const std::string& exact_layer,
                     const std::string& wildcard_layer) {
  return layer_id == exact_layer || layer_id == wildcard_layer;
}

std::optional<std::string> visiblePadCopperLayer(const std::set<std::string>& hidden_layers,
                                                 const ccad::CanvasPad& pad) {
  for (const std::string& layer : pad.layers) {
    if ((layer.ends_with(".Cu") || layer == "*.Cu") && padLayerIsVisible(hidden_layers, layer)) {
      return layer == "*.Cu" ? std::optional<std::string>("F.Cu") : std::optional<std::string>(layer);
    }
  }
  return std::nullopt;
}

void tagLayerOverlay(QGraphicsItem& item, const QString& type, const QString& id,
                     const QString& layer_id) {
  item.setData(kCanvasObjectTypeRole, type);
  item.setData(kCanvasObjectIdRole, id);
  item.setData(kCanvasObjectLayerIdRole, layer_id);
}

double sceneX(const ccad::CanvasScene& scene, const double board_x_units, const double margin,
              const double scale) {
  return margin + ((board_x_units - scene.board_origin_x_units) * scale);
}

double sceneY(const ccad::CanvasScene& scene, const double board_y_units, const double margin,
              const double scale) {
  return margin + ((board_y_units - scene.board_origin_y_units) * scale);
}

}  // namespace

void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene) {
  renderBoardCanvas(canvas_scene, scene, CanvasRenderTheme{});
}

void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene,
                       const CanvasRenderTheme& theme) {
  canvas_scene.clear();
  canvas_scene.setBackgroundBrush(QBrush(theme.background_color));
  if (!scene.has_board && scene.lines.empty() && scene.components.empty() && scene.wires.empty()) {
    auto* text = canvas_scene.addText("No board or schematic to display");
    text->setDefaultTextColor(theme.empty_text_color);
    text->setPos(18, 18);
    canvas_scene.setSceneRect(0, 0, 420, 280);
    return;
  }

  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double width = scene.view_width_units * scale;
  const double height = scene.view_height_units * scale;
  const QRectF bounds_rect(margin, margin, width, height);
  const double horizontal_padding = std::max(width * 6.0, 4800.0);
  const double vertical_padding = std::max(height * 6.0, 3600.0);
  canvas_scene.setSceneRect(bounds_rect.adjusted(-horizontal_padding, -vertical_padding,
                                                 horizontal_padding, vertical_padding + 52.0));

  QPen grid_pen(theme.grid_color);
  grid_pen.setWidthF(0.25);
  for (double x = margin; x <= margin + width; x += 5.0 * scale) {
    canvas_scene.addLine(x, margin, x, margin + height, grid_pen);
  }
  for (double y = margin; y <= margin + height; y += 5.0 * scale) {
    canvas_scene.addLine(margin, y, margin + width, y, grid_pen);
  }

  if (scene.has_board) {
    QPen outline_pen(theme.board_outline_color);
    outline_pen.setWidthF(1.8);
    auto* board = canvas_scene.addRect(bounds_rect, outline_pen, QBrush(theme.board_fill_color));
    board->setToolTip("Board outline");
  }

  const std::set<std::string> hidden_layers = hiddenLayerIds(scene);

  QPen placement_region_pen(theme.placement_region_color);
  placement_region_pen.setWidthF(1.2);
  placement_region_pen.setStyle(Qt::DotLine);
  QBrush placement_region_brush(QColor(theme.placement_region_color.red(),
                                       theme.placement_region_color.green(),
                                       theme.placement_region_color.blue(), 36));
  for (const ccad::CanvasPlacementRegion& region : scene.placement_regions) {
    const QRectF region_rect(sceneX(scene, region.x_units, margin, scale),
                             sceneY(scene, region.y_units, margin, scale),
                             region.width_units * scale, region.height_units * scale);
    QPainterPath region_path;
    region_path.addRect(region_rect);
    auto* item =
        addHighlightPath(canvas_scene, region_path, placement_region_pen, placement_region_brush);
    item->setToolTip("Placement region " + qstr(region.id) + " (" + qstr(region.kind) + ")");
    tagObject(*item, "placement-region", qstr(region.id), theme.placement_region_color);
  }

  QPen keepout_pen(theme.keepout_color);
  keepout_pen.setWidthF(1.2);
  keepout_pen.setStyle(Qt::DashLine);
  QBrush keepout_brush(
      QColor(theme.keepout_color.red(), theme.keepout_color.green(), theme.keepout_color.blue(), 48));
  for (const ccad::CanvasKeepout& keepout : scene.keepouts) {
    const QRectF keepout_rect(sceneX(scene, keepout.x_units, margin, scale),
                              sceneY(scene, keepout.y_units, margin, scale),
                              keepout.width_units * scale, keepout.height_units * scale);
    QPainterPath keepout_path;
    keepout_path.addRect(keepout_rect);
    auto* item = addHighlightPath(canvas_scene, keepout_path, keepout_pen, keepout_brush);
    item->setToolTip("Keepout " + qstr(keepout.id) + " (" + qstr(keepout.kind) + ")");
    tagObject(*item, "keepout", qstr(keepout.id), theme.keepout_color);
  }

  QPen track_pen(theme.track_color);
  track_pen.setCapStyle(Qt::RoundCap);
  for (const ccad::CanvasTrack& track : scene.tracks) {
    if (!layerIsVisible(hidden_layers, track.layer_id)) {
      continue;
    }
    const QColor track_color = colorForKiCadLayer(theme, track.layer_id);
    track_pen.setColor(track_color);
    track_pen.setWidthF(std::max(1.2, track.width_units * scale));
    QPainterPath track_path;
    track_path.moveTo(sceneX(scene, track.start_x_units, margin, scale),
                      sceneY(scene, track.start_y_units, margin, scale));
    track_path.lineTo(sceneX(scene, track.end_x_units, margin, scale),
                      sceneY(scene, track.end_y_units, margin, scale));
    auto* item = addHighlightPath(canvas_scene, track_path, track_pen, QBrush(Qt::NoBrush));
    item->setToolTip("Track " + qstr(track.id));
    tagObject(*item, "track", qstr(track.id), track_color, qstr(track.net_id),
              qstr(track.layer_id), qstr(track.source_route_request_id));
  }

  for (const ccad::CanvasPad& pad : scene.pads) {
    const QRectF pad_rect(sceneX(scene, pad.x_units, margin, scale) -
                              ((pad.width_units * scale) / 2.0),
                          sceneY(scene, pad.y_units, margin, scale) -
                              ((pad.height_units * scale) / 2.0),
                          pad.width_units * scale, pad.height_units * scale);
    const QPointF pad_center(sceneX(scene, pad.x_units, margin, scale),
                             sceneY(scene, pad.y_units, margin, scale));
    QPainterPath pad_path = padShapePath(pad_rect, pad_center, pad.shape, pad.rotation_degrees,
                                         pad.roundrect_rratio, pad.chamfer_ratio);
    if (const std::optional<std::string> copper_layer = visiblePadCopperLayer(hidden_layers, pad)) {
      const QColor pad_color = colorForKiCadLayer(theme, *copper_layer);
      auto* item =
          addHighlightPath(canvas_scene, pad_path, QPen(pad_color.lighter(130), 0.8),
                           QBrush(pad_color));
      item->setToolTip("Pad " + qstr(pad.id) + " (" + qstr(pad.type) + ")");

      QString layers_str;
      for (const auto& l : pad.layers) layers_str += qstr(l) + ",";
      if (!layers_str.isEmpty()) layers_str.chop(1);

      tagObject(*item, "pad", qstr(pad.id), pad_color, qstr(pad.net_id), layers_str);
    }

    const auto addPadLayerAperture = [&](const QString& type, const std::string& layer_id,
                                         const QColor& layer_color, double inflate,
                                         Qt::PenStyle style) {
      const QRectF aperture_rect = pad_rect.adjusted(-inflate, -inflate, inflate, inflate);
      QPainterPath aperture_path = padShapePath(aperture_rect, aperture_rect.center(), pad.shape,
                                                pad.rotation_degrees, pad.roundrect_rratio,
                                                pad.chamfer_ratio);
      QPen aperture_pen(layer_color, 0.9);
      aperture_pen.setStyle(style);
      aperture_pen.setJoinStyle(Qt::RoundJoin);
      aperture_pen.setCapStyle(Qt::RoundCap);
      auto* aperture = addHighlightPath(canvas_scene, aperture_path, aperture_pen,
                                        QBrush(QColor(layer_color.red(), layer_color.green(),
                                                      layer_color.blue(), 42)));
      aperture->setFlag(QGraphicsItem::ItemIsSelectable, false);
      aperture->setToolTip(type + " " + qstr(pad.id) + " " + qstr(layer_id));
      tagLayerOverlay(*aperture, type, qstr(pad.id) + ":" + qstr(layer_id), qstr(layer_id));
    };

    for (const std::string& layer : pad.layers) {
      if (!padLayerIsVisible(hidden_layers, layer)) {
        continue;
      }
      if (padLayerMatches(layer, "F.Mask", "*.Mask")) {
        addPadLayerAperture("pad-mask", "F.Mask", colorForKiCadLayer(theme, "F.Mask"), 1.2,
                            Qt::DashLine);
      } else if (layer == "B.Mask") {
        addPadLayerAperture("pad-mask", "B.Mask", colorForKiCadLayer(theme, "B.Mask"), 1.2,
                            Qt::DashLine);
      } else if (padLayerMatches(layer, "F.Paste", "*.Paste")) {
        addPadLayerAperture("pad-paste", "F.Paste", colorForKiCadLayer(theme, "F.Paste"), 0.5,
                            Qt::SolidLine);
      } else if (layer == "B.Paste") {
        addPadLayerAperture("pad-paste", "B.Paste", colorForKiCadLayer(theme, "B.Paste"), 0.5,
                            Qt::SolidLine);
      }
    }

    if (pad.drill_units > 0.0) {
      const double drill = pad.drill_units * scale;
      auto* drill_item = canvas_scene.addEllipse(pad_center.x() - (drill / 2.0),
                                                 pad_center.y() - (drill / 2.0), drill,
                                                 drill, QPen(Qt::NoPen),
                                                 QBrush(theme.background_color));
      drill_item->setData(kCanvasObjectTypeRole, "pad-drill");
      drill_item->setData(kCanvasObjectIdRole, qstr(pad.id) + ".drill");
    }
  }

  for (const ccad::CanvasVia& via : scene.vias) {
    const double diameter = via.diameter_units * scale;
    const QRectF via_rect(sceneX(scene, via.x_units, margin, scale) - (diameter / 2.0),
                          sceneY(scene, via.y_units, margin, scale) - (diameter / 2.0),
                          diameter, diameter);
    QPainterPath via_path;
    via_path.addEllipse(via_rect);
    auto* item =
        addHighlightPath(canvas_scene, via_path, QPen(theme.via_outline_color, 1.0),
                         QBrush(theme.via_fill_color));
    item->setToolTip("Via " + qstr(via.id));
    tagObject(*item, "via", qstr(via.id), theme.via_fill_color, qstr(via.net_id));
    const double drill = via.drill_units * scale;
    canvas_scene.addEllipse(sceneX(scene, via.x_units, margin, scale) - (drill / 2.0),
                            sceneY(scene, via.y_units, margin, scale) - (drill / 2.0), drill,
                            drill, QPen(Qt::NoPen), QBrush(theme.background_color));
  }

  QPen shape_pen(theme.track_color); // Base color for component graphics
  shape_pen.setCapStyle(Qt::RoundCap);
  shape_pen.setJoinStyle(Qt::RoundJoin);

  for (const ccad::CanvasLine& line : scene.lines) {
    if (!layerIsVisible(hidden_layers, line.layer_id)) continue;
    const QColor layer_color = colorForKiCadLayer(theme, line.layer_id);
    shape_pen.setColor(layer_color);
    shape_pen.setWidthF(std::max(0.2, line.width_units * scale));
    QPainterPath path;
    path.moveTo(sceneX(scene, line.start_x_units, margin, scale), sceneY(scene, line.start_y_units, margin, scale));
    path.lineTo(sceneX(scene, line.end_x_units, margin, scale), sceneY(scene, line.end_y_units, margin, scale));
    auto* item = addHighlightPath(canvas_scene, path, shape_pen, QBrush(Qt::NoBrush));
    tagObject(*item, "line", qstr(line.id), layer_color, "", qstr(line.layer_id));
  }

  for (const ccad::CanvasArc& arc : scene.arcs) {
    if (!layerIsVisible(hidden_layers, arc.layer_id)) continue;
    const QColor layer_color = colorForKiCadLayer(theme, arc.layer_id);
    shape_pen.setColor(layer_color);
    shape_pen.setWidthF(std::max(0.2, arc.width_units * scale));
    QPainterPath path;
    // We have start, mid, end. To draw an arc through 3 points accurately, we approximate or use QPainterPath::arcTo. 
    // QPainterPath doesn't natively have a 3-point arc easily without calculating the bounding rect. 
    // We can use a rough polyline or bezier curve for now to ensure rendering.
    path.moveTo(sceneX(scene, arc.start_x_units, margin, scale), sceneY(scene, arc.start_y_units, margin, scale));
    path.quadTo(sceneX(scene, arc.mid_x_units, margin, scale), sceneY(scene, arc.mid_y_units, margin, scale),
                sceneX(scene, arc.end_x_units, margin, scale), sceneY(scene, arc.end_y_units, margin, scale));
    auto* item = addHighlightPath(canvas_scene, path, shape_pen, QBrush(Qt::NoBrush));
    tagObject(*item, "arc", qstr(arc.id), layer_color, "", qstr(arc.layer_id));
  }

  for (const ccad::CanvasCircle& circle : scene.circles) {
    if (!layerIsVisible(hidden_layers, circle.layer_id)) continue;
    const QColor layer_color = colorForKiCadLayer(theme, circle.layer_id);
    shape_pen.setColor(layer_color);
    shape_pen.setWidthF(std::max(0.2, circle.width_units * scale));
    const double radius = circle.radius_units * scale;
    const QRectF rect(sceneX(scene, circle.center_x_units, margin, scale) - radius,
                      sceneY(scene, circle.center_y_units, margin, scale) - radius,
                      radius * 2.0, radius * 2.0);
    QPainterPath path;
    path.addEllipse(rect);
    
    QBrush brush = Qt::NoBrush;
    if (circle.fill_type == "background") brush = QBrush(theme.background_color);
    else if (circle.fill_type == "solid") brush = QBrush(layer_color);

    auto* item = addHighlightPath(canvas_scene, path, shape_pen, brush);
    tagObject(*item, "circle", qstr(circle.id), layer_color, "", qstr(circle.layer_id));
  }

  for (const ccad::CanvasPolygon& poly : scene.polygons) {
    if (!layerIsVisible(hidden_layers, poly.layer_id)) continue;
    const QColor layer_color = colorForKiCadLayer(theme, poly.layer_id);
    shape_pen.setColor(layer_color);
    shape_pen.setWidthF(std::max(0.2, poly.width_units * scale));
    QPolygonF qpoly;
    for (size_t i = 0; i < poly.pts_x_units.size(); ++i) {
      qpoly << QPointF(sceneX(scene, poly.pts_x_units[i], margin, scale), 
                       sceneY(scene, poly.pts_y_units[i], margin, scale));
    }
    QPainterPath path;
    path.addPolygon(qpoly);
    
    QBrush brush = Qt::NoBrush;
    if (poly.fill_type == "background") brush = QBrush(theme.background_color);
    else if (poly.fill_type == "solid") brush = QBrush(layer_color);

    auto* item = addHighlightPath(canvas_scene, path, shape_pen, brush);
    tagObject(*item, "polygon", qstr(poly.id), layer_color, "", qstr(poly.layer_id));
  }

  for (const ccad::CanvasText& text_item : scene.texts) {
    if (hidden_layers.count(text_item.layer_id)) continue;
    
    QGraphicsTextItem* text = canvas_scene.addText(QString::fromStdString(text_item.text));
    text->setDefaultTextColor(colorForKiCadLayer(theme, text_item.layer_id));
    text->setPos(sceneX(scene, text_item.x_units, margin, scale), 
                 sceneY(scene, text_item.y_units, margin, scale));
    text->setRotation(text_item.rotation_degrees);
  }

  // Render Schematic Components
  QPen component_pen(theme.board_outline_color);
  component_pen.setWidthF(1.5);
  for (const ccad::CanvasComponent& comp : scene.components) {
    const double cx = sceneX(scene, comp.x_units, margin, scale);
    const double cy = sceneY(scene, comp.y_units, margin, scale);
    
    auto* rect = canvas_scene.addRect(cx - 15.0, cy - 15.0, 30.0, 30.0, component_pen);
    rect->setData(kCanvasObjectIdRole, QString::fromStdString(comp.id));
    rect->setData(kCanvasObjectTypeRole, "Component");
    rect->setToolTip(QString::fromStdString(comp.id + " (" + comp.part + ")"));
    
    auto* label = canvas_scene.addText(QString::fromStdString(comp.id));
    label->setDefaultTextColor(theme.board_label_color);
    label->setPos(cx - 15.0, cy - 35.0);
  }

  // Render Schematic Wires
  QPen wire_pen(theme.track_color);
  wire_pen.setWidthF(2.0);
  for (std::size_t i = 0; i < scene.wires.size(); ++i) {
    const ccad::CanvasWire& wire = scene.wires[i];
    const double sx = sceneX(scene, wire.start_x_units, margin, scale);
    const double sy = sceneY(scene, wire.start_y_units, margin, scale);
    const double ex = sceneX(scene, wire.end_x_units, margin, scale);
    const double ey = sceneY(scene, wire.end_y_units, margin, scale);
    
    auto* line = canvas_scene.addLine(sx, sy, ex, ey, wire_pen);
    line->setData(kCanvasObjectIdRole, QString("wire_%1").arg(i));
    line->setData(kCanvasObjectTypeRole, "Wire");
    line->setData(kCanvasObjectNetIdRole, QString::fromStdString(wire.net_id));
    line->setToolTip(QString::fromStdString(wire.net_id));
  }

  auto* label = canvas_scene.addText(QString::number(scene.view_width_units, 'f', 2) + " mm x " +
                                     QString::number(scene.view_height_units, 'f', 2) + " mm");
  label->setDefaultTextColor(theme.board_label_color);
  label->setScale(0.9);
  label->setPos(margin, margin + height + 10.0);
}

void addDiagnosticMarkers(QGraphicsScene& canvas_scene,
                          const std::vector<ccad::Diagnostic>& diagnostics) {
  addDiagnosticMarkers(canvas_scene, diagnostics, CanvasRenderTheme{});
}

void addDiagnosticMarkers(QGraphicsScene& canvas_scene,
                          const std::vector<ccad::Diagnostic>& diagnostics,
                          const CanvasRenderTheme& theme) {
  for (QGraphicsItem* item : canvas_scene.items()) {
    if (!canvasDiagnosticMarkerObjectId(*item).isEmpty()) {
      canvas_scene.removeItem(item);
      delete item;
    }
  }

  for (const ccad::Diagnostic& diagnostic : diagnostics) {
    const QString object_id = qstr(diagnostic.object_id);
    QGraphicsItem* target = findCanvasObjectById(canvas_scene, object_id);
    if (target == nullptr) {
      continue;
    }

    const QRectF bounds = target->sceneBoundingRect();
    const QPointF center = bounds.center();
    constexpr double radius = 5.0;
    const QColor color =
        diagnostic.severity == "error" ? theme.error_marker_color : theme.warning_marker_color;
    auto* marker =
        canvas_scene.addEllipse(center.x() - radius, center.y() - radius, radius * 2.0,
                                radius * 2.0, QPen(color, 1.8), QBrush(QColor(color.red(),
                                                                              color.green(),
                                                                              color.blue(), 80)));
    marker->setZValue(1000.0);
    marker->setToolTip(qstr(diagnostic.code) + ": " + qstr(diagnostic.message));
    marker->setData(kCanvasDiagnosticMarkerObjectIdRole, object_id);
    marker->setData(kCanvasDiagnosticMarkerSeverityRole, qstr(diagnostic.severity));
  }
}

QString canvasObjectId(const QGraphicsItem& item) {
  return item.data(kCanvasObjectIdRole).toString();
}

QString canvasObjectType(const QGraphicsItem& item) {
  return item.data(kCanvasObjectTypeRole).toString();
}

QString canvasObjectNetId(const QGraphicsItem& item) {
  return item.data(kCanvasObjectNetIdRole).toString();
}

QString canvasObjectLayerId(const QGraphicsItem& item) {
  return item.data(kCanvasObjectLayerIdRole).toString();
}

QString canvasObjectRouteRequestId(const QGraphicsItem& item) {
  return item.data(kCanvasObjectRouteRequestIdRole).toString();
}

bool canvasUsesShapeSelectionHighlight(const QGraphicsItem& item) {
  return item.data(kCanvasShapeSelectionHighlightRole).toBool();
}

QColor canvasSelectionHighlightColor(const QGraphicsItem& item) {
  const QVariant value = item.data(kCanvasSelectionHighlightColorRole);
  if (!value.canConvert<QColor>()) {
    return QColor("#60a5fa");
  }
  return value.value<QColor>();
}

double canvasSelectionHighlightWidth(const QGraphicsItem& item) {
  const QVariant value = item.data(kCanvasSelectionHighlightWidthRole);
  if (!value.isValid()) {
    return 2.5;
  }
  return value.toDouble();
}

bool selectCanvasObjectById(QGraphicsScene& canvas_scene, const QString& id) {
  canvas_scene.clearSelection();
  QGraphicsItem* item = findCanvasObjectById(canvas_scene, id);
  if (item != nullptr) {
    item->setSelected(true);
    return true;
  }
  return false;
}

int selectCanvasObjectsByNetId(QGraphicsScene& canvas_scene, const QString& net_id) {
  canvas_scene.clearSelection();
  if (net_id.isEmpty()) {
    return 0;
  }

  int selected_count = 0;
  for (QGraphicsItem* item : canvas_scene.items()) {
    if (canvasObjectNetId(*item) == net_id) {
      item->setSelected(true);
      ++selected_count;
    }
  }
  return selected_count;
}

int selectCanvasObjectsByRouteRequestId(QGraphicsScene& canvas_scene,
                                        const QString& route_request_id) {
  canvas_scene.clearSelection();
  if (route_request_id.isEmpty()) {
    return 0;
  }

  int selected_count = 0;
  for (QGraphicsItem* item : canvas_scene.items()) {
    if (canvasObjectRouteRequestId(*item) == route_request_id) {
      item->setSelected(true);
      ++selected_count;
    }
  }
  return selected_count;
}

QString canvasDiagnosticMarkerObjectId(const QGraphicsItem& item) {
  return item.data(kCanvasDiagnosticMarkerObjectIdRole).toString();
}

QString canvasDiagnosticMarkerSeverity(const QGraphicsItem& item) {
  return item.data(kCanvasDiagnosticMarkerSeverityRole).toString();
}
