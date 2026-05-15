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
               const QColor& display_color) {
  item.setFlag(QGraphicsItem::ItemIsSelectable, true);
  item.setData(kCanvasObjectTypeRole, type);
  item.setData(kCanvasObjectIdRole, id);
  item.setData(kCanvasShapeSelectionHighlightRole, true);
  item.setData(kCanvasSelectionHighlightColorRole, lighterHighlight(display_color));
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

}  // namespace

void renderBoardCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene) {
  canvas_scene.clear();
  canvas_scene.setBackgroundBrush(QBrush(QColor("#07111f")));
  if (!scene.has_board) {
    auto* text = canvas_scene.addText("No board outline yet");
    text->setDefaultTextColor(QColor("#94a3b8"));
    text->setPos(18, 18);
    canvas_scene.setSceneRect(0, 0, 420, 280);
    return;
  }

  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double width = scene.view_width_units * scale;
  const double height = scene.view_height_units * scale;
  const QRectF board_rect(margin, margin, width, height);
  canvas_scene.setSceneRect(0, 0, width + (2.0 * margin), height + 52.0);

  QPen grid_pen(QColor("#17243a"));
  grid_pen.setWidthF(0.25);
  for (double x = margin; x <= margin + width; x += 5.0 * scale) {
    canvas_scene.addLine(x, margin, x, margin + height, grid_pen);
  }
  for (double y = margin; y <= margin + height; y += 5.0 * scale) {
    canvas_scene.addLine(margin, y, margin + width, y, grid_pen);
  }

  QPen outline_pen(QColor("#38bdf8"));
  outline_pen.setWidthF(1.8);
  auto* board = canvas_scene.addRect(board_rect, outline_pen, QBrush(QColor("#0f1b2d")));
  board->setToolTip("Board outline");

  QPen keepout_pen(QColor("#f97316"));
  keepout_pen.setWidthF(1.2);
  keepout_pen.setStyle(Qt::DashLine);
  QBrush keepout_brush(QColor(249, 115, 22, 48));
  for (const ccad::CanvasKeepout& keepout : scene.keepouts) {
    const QRectF keepout_rect(margin + (keepout.x_units * scale),
                              margin + (keepout.y_units * scale),
                              keepout.width_units * scale, keepout.height_units * scale);
    QPainterPath keepout_path;
    keepout_path.addRect(keepout_rect);
    auto* item = addHighlightPath(canvas_scene, keepout_path, keepout_pen, keepout_brush);
    item->setToolTip("Keepout " + qstr(keepout.id) + " (" + qstr(keepout.kind) + ")");
    tagObject(*item, "keepout", qstr(keepout.id), QColor("#f97316"));
  }

  const QColor track_color("#ef4444");
  QPen track_pen(track_color);
  track_pen.setCapStyle(Qt::RoundCap);
  for (const ccad::CanvasTrack& track : scene.tracks) {
    track_pen.setWidthF(std::max(1.2, track.width_units * scale));
    QPainterPath track_path;
    track_path.moveTo(margin + (track.start_x_units * scale),
                      margin + (track.start_y_units * scale));
    track_path.lineTo(margin + (track.end_x_units * scale), margin + (track.end_y_units * scale));
    auto* item = addHighlightPath(canvas_scene, track_path, track_pen, QBrush(Qt::NoBrush));
    item->setToolTip("Track " + qstr(track.id));
    tagObject(*item, "track", qstr(track.id), track_color);
  }

  const QColor pad_fill_color("#be185d");
  for (const ccad::CanvasPad& pad : scene.pads) {
    const QRectF pad_rect(margin + (pad.x_units * scale) - ((pad.width_units * scale) / 2.0),
                          margin + (pad.y_units * scale) - ((pad.height_units * scale) / 2.0),
                          pad.width_units * scale, pad.height_units * scale);
    const QPointF pad_center(margin + (pad.x_units * scale), margin + (pad.y_units * scale));
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
        addHighlightPath(canvas_scene, pad_path, QPen(QColor("#f472b6"), 0.8), QBrush(pad_fill_color));
    item->setToolTip("Pad " + qstr(pad.id));
    tagObject(*item, "pad", qstr(pad.id), pad_fill_color);
  }

  const QColor via_fill_color("#f59e0b");
  for (const ccad::CanvasVia& via : scene.vias) {
    const double diameter = via.diameter_units * scale;
    const QRectF via_rect(margin + (via.x_units * scale) - (diameter / 2.0),
                          margin + (via.y_units * scale) - (diameter / 2.0), diameter, diameter);
    QPainterPath via_path;
    via_path.addEllipse(via_rect);
    auto* item =
        addHighlightPath(canvas_scene, via_path, QPen(QColor("#fde68a"), 1.0), QBrush(via_fill_color));
    item->setToolTip("Via " + qstr(via.id));
    tagObject(*item, "via", qstr(via.id), via_fill_color);
    const double drill = via.drill_units * scale;
    canvas_scene.addEllipse(margin + (via.x_units * scale) - (drill / 2.0),
                            margin + (via.y_units * scale) - (drill / 2.0), drill, drill,
                            QPen(Qt::NoPen), QBrush(QColor("#07111f")));
  }

  auto* label = canvas_scene.addText(QString::number(scene.view_width_units, 'f', 2) + " mm x " +
                                     QString::number(scene.view_height_units, 'f', 2) + " mm");
  label->setDefaultTextColor(QColor("#cbd5e1"));
  label->setScale(0.9);
  label->setPos(margin, margin + height + 10.0);
}

void addDiagnosticMarkers(QGraphicsScene& canvas_scene,
                          const std::vector<ccad::Diagnostic>& diagnostics) {
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
    const QColor color = diagnostic.severity == "error" ? QColor("#ef4444") : QColor("#f59e0b");
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

QString canvasDiagnosticMarkerObjectId(const QGraphicsItem& item) {
  return item.data(kCanvasDiagnosticMarkerObjectIdRole).toString();
}

QString canvasDiagnosticMarkerSeverity(const QGraphicsItem& item) {
  return item.data(kCanvasDiagnosticMarkerSeverityRole).toString();
}
