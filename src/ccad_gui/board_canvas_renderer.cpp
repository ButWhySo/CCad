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
    highlight_pen.setCosmetic(true);
    highlight_pen.setWidthF(2.5);
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
  if (!scene.has_board) {
    auto* text = canvas_scene.addText("No board outline yet");
    text->setDefaultTextColor(theme.empty_text_color);
    text->setPos(18, 18);
    canvas_scene.setSceneRect(0, 0, 420, 280);
    return;
  }

  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double width = scene.view_width_units * scale;
  const double height = scene.view_height_units * scale;
  const QRectF board_rect(margin, margin, width, height);
  const double horizontal_padding = std::max(width * 6.0, 4800.0);
  const double vertical_padding = std::max(height * 6.0, 3600.0);
  canvas_scene.setSceneRect(board_rect.adjusted(-horizontal_padding, -vertical_padding,
                                                horizontal_padding, vertical_padding + 52.0));

  QPen grid_pen(theme.grid_color);
  grid_pen.setWidthF(0.25);
  for (double x = margin; x <= margin + width; x += 5.0 * scale) {
    canvas_scene.addLine(x, margin, x, margin + height, grid_pen);
  }
  for (double y = margin; y <= margin + height; y += 5.0 * scale) {
    canvas_scene.addLine(margin, y, margin + width, y, grid_pen);
  }

  QPen outline_pen(theme.board_outline_color);
  outline_pen.setWidthF(1.8);
  auto* board = canvas_scene.addRect(board_rect, outline_pen, QBrush(theme.board_fill_color));
  board->setToolTip("Board outline");

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
    track_pen.setWidthF(std::max(1.2, track.width_units * scale));
    QPainterPath track_path;
    track_path.moveTo(sceneX(scene, track.start_x_units, margin, scale),
                      sceneY(scene, track.start_y_units, margin, scale));
    track_path.lineTo(sceneX(scene, track.end_x_units, margin, scale),
                      sceneY(scene, track.end_y_units, margin, scale));
    auto* item = addHighlightPath(canvas_scene, track_path, track_pen, QBrush(Qt::NoBrush));
    item->setToolTip("Track " + qstr(track.id));
    tagObject(*item, "track", qstr(track.id), theme.track_color, qstr(track.net_id),
              qstr(track.layer_id), qstr(track.source_route_request_id));
  }

  for (const ccad::CanvasPad& pad : scene.pads) {
    if (!layerIsVisible(hidden_layers, pad.layer_id)) {
      continue;
    }
    const QRectF pad_rect(sceneX(scene, pad.x_units, margin, scale) -
                              ((pad.width_units * scale) / 2.0),
                          sceneY(scene, pad.y_units, margin, scale) -
                              ((pad.height_units * scale) / 2.0),
                          pad.width_units * scale, pad.height_units * scale);
    const QPointF pad_center(sceneX(scene, pad.x_units, margin, scale),
                             sceneY(scene, pad.y_units, margin, scale));
    QPainterPath pad_path;
    pad_path.addRoundedRect(pad_rect, 2.0, 2.0);
    if (pad.rotation_degrees != 0.0) {
      QTransform transform;
      transform.translate(pad_center.x(), pad_center.y());
      transform.rotate(pad.rotation_degrees);
      transform.translate(-pad_center.x(), -pad_center.y());
      pad_path = transform.map(pad_path);
    }
    auto* item =
        addHighlightPath(canvas_scene, pad_path, QPen(theme.pad_outline_color, 0.8),
                         QBrush(theme.pad_fill_color));
    item->setToolTip("Pad " + qstr(pad.id));
    tagObject(*item, "pad", qstr(pad.id), theme.pad_fill_color, qstr(pad.net_id),
              qstr(pad.layer_id));
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
    shape_pen.setWidthF(std::max(0.2, line.width_units * scale));
    QPainterPath path;
    path.moveTo(sceneX(scene, line.start_x_units, margin, scale), sceneY(scene, line.start_y_units, margin, scale));
    path.lineTo(sceneX(scene, line.end_x_units, margin, scale), sceneY(scene, line.end_y_units, margin, scale));
    auto* item = addHighlightPath(canvas_scene, path, shape_pen, QBrush(Qt::NoBrush));
    tagObject(*item, "line", qstr(line.id), theme.track_color, "", qstr(line.layer_id));
  }

  for (const ccad::CanvasArc& arc : scene.arcs) {
    if (!layerIsVisible(hidden_layers, arc.layer_id)) continue;
    shape_pen.setWidthF(std::max(0.2, arc.width_units * scale));
    QPainterPath path;
    // We have start, mid, end. To draw an arc through 3 points accurately, we approximate or use QPainterPath::arcTo. 
    // QPainterPath doesn't natively have a 3-point arc easily without calculating the bounding rect. 
    // We can use a rough polyline or bezier curve for now to ensure rendering.
    path.moveTo(sceneX(scene, arc.start_x_units, margin, scale), sceneY(scene, arc.start_y_units, margin, scale));
    path.quadTo(sceneX(scene, arc.mid_x_units, margin, scale), sceneY(scene, arc.mid_y_units, margin, scale),
                sceneX(scene, arc.end_x_units, margin, scale), sceneY(scene, arc.end_y_units, margin, scale));
    auto* item = addHighlightPath(canvas_scene, path, shape_pen, QBrush(Qt::NoBrush));
    tagObject(*item, "arc", qstr(arc.id), theme.track_color, "", qstr(arc.layer_id));
  }

  for (const ccad::CanvasCircle& circle : scene.circles) {
    if (!layerIsVisible(hidden_layers, circle.layer_id)) continue;
    shape_pen.setWidthF(std::max(0.2, circle.width_units * scale));
    const double radius = circle.radius_units * scale;
    const QRectF rect(sceneX(scene, circle.center_x_units, margin, scale) - radius,
                      sceneY(scene, circle.center_y_units, margin, scale) - radius,
                      radius * 2.0, radius * 2.0);
    QPainterPath path;
    path.addEllipse(rect);
    
    QBrush brush = Qt::NoBrush;
    if (circle.fill_type == "background") brush = QBrush(theme.background_color);
    else if (circle.fill_type == "solid") brush = QBrush(theme.track_color);

    auto* item = addHighlightPath(canvas_scene, path, shape_pen, brush);
    tagObject(*item, "circle", qstr(circle.id), theme.track_color, "", qstr(circle.layer_id));
  }

  for (const ccad::CanvasPolygon& poly : scene.polygons) {
    if (!layerIsVisible(hidden_layers, poly.layer_id)) continue;
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
    else if (poly.fill_type == "solid") brush = QBrush(theme.track_color);

    auto* item = addHighlightPath(canvas_scene, path, shape_pen, brush);
    tagObject(*item, "polygon", qstr(poly.id), theme.track_color, "", qstr(poly.layer_id));
  }

  for (const ccad::CanvasText& text : scene.texts) {
    if (!layerIsVisible(hidden_layers, text.layer_id)) continue;
    auto* item = canvas_scene.addText(qstr(text.text));
    item->setDefaultTextColor(theme.track_color);
    item->setPos(sceneX(scene, text.x_units, margin, scale), sceneY(scene, text.y_units, margin, scale));
    // Simplistic rotation
    item->setTransformOriginPoint(0, 0);
    item->setRotation(text.rotation_degrees);
    tagObject(*item, "text", qstr(text.id), theme.track_color, "", qstr(text.layer_id));
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
