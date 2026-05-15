#include "board_canvas_renderer.hpp"

#include <QBrush>
#include <QColor>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QPen>
#include <QTransform>

#include <algorithm>

namespace {

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

void tagObject(QGraphicsItem& item, const QString& type, const QString& id) {
  item.setFlag(QGraphicsItem::ItemIsSelectable, true);
  item.setData(kCanvasObjectTypeRole, type);
  item.setData(kCanvasObjectIdRole, id);
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
    auto* item = canvas_scene.addRect(keepout_rect, keepout_pen, keepout_brush);
    item->setToolTip("Keepout " + qstr(keepout.id) + " (" + qstr(keepout.kind) + ")");
    tagObject(*item, "keepout", qstr(keepout.id));
  }

  QPen track_pen(QColor("#ef4444"));
  track_pen.setCapStyle(Qt::RoundCap);
  for (const ccad::CanvasTrack& track : scene.tracks) {
    track_pen.setWidthF(std::max(1.2, track.width_units * scale));
    auto* item = canvas_scene.addLine(margin + (track.start_x_units * scale),
                                      margin + (track.start_y_units * scale),
                                      margin + (track.end_x_units * scale),
                                      margin + (track.end_y_units * scale), track_pen);
    item->setToolTip("Track " + qstr(track.id));
    tagObject(*item, "track", qstr(track.id));
  }

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
        canvas_scene.addPath(pad_path, QPen(QColor("#f472b6"), 0.8), QBrush(QColor("#be185d")));
    item->setToolTip("Pad " + qstr(pad.id));
    tagObject(*item, "pad", qstr(pad.id));
  }

  for (const ccad::CanvasVia& via : scene.vias) {
    const double diameter = via.diameter_units * scale;
    const QRectF via_rect(margin + (via.x_units * scale) - (diameter / 2.0),
                          margin + (via.y_units * scale) - (diameter / 2.0), diameter, diameter);
    auto* item =
        canvas_scene.addEllipse(via_rect, QPen(QColor("#fde68a"), 1.0), QBrush(QColor("#f59e0b")));
    item->setToolTip("Via " + qstr(via.id));
    tagObject(*item, "via", qstr(via.id));
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

QString canvasObjectId(const QGraphicsItem& item) {
  return item.data(kCanvasObjectIdRole).toString();
}

QString canvasObjectType(const QGraphicsItem& item) {
  return item.data(kCanvasObjectTypeRole).toString();
}
