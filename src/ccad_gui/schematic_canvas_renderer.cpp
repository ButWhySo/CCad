#include "schematic_canvas_renderer.hpp"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QPainterPath>
#include <QPen>

namespace {

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

double sceneX(const ccad::CanvasScene& /*scene*/, const double x_units, const double margin,
              const double scale) {
  return margin + (x_units * scale);
}

double sceneY(const ccad::CanvasScene& /*scene*/, const double y_units, const double margin,
              const double scale) {
  return margin + (y_units * scale);
}

}  // namespace

void renderSchematicCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene) {
  renderSchematicCanvas(canvas_scene, scene, CanvasRenderTheme{});
}

void renderSchematicCanvas(QGraphicsScene& canvas_scene, const ccad::CanvasScene& scene,
                           const CanvasRenderTheme& theme) {
  canvas_scene.clear();
  canvas_scene.setBackgroundBrush(QBrush(theme.background_color));

  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double width = scene.view_width_units * scale;
  const double height = scene.view_height_units * scale;
  const QRectF bounds_rect(margin, margin, width > 0 ? width : 4000.0, height > 0 ? height : 3000.0);
  canvas_scene.setSceneRect(bounds_rect.adjusted(-2000, -2000, 2000, 2000));

  // Draw basic grid
  QPen grid_pen(theme.grid_color);
  grid_pen.setWidthF(0.25);
  for (double x = bounds_rect.left(); x <= bounds_rect.right(); x += 5.0 * scale) {
    canvas_scene.addLine(x, bounds_rect.top(), x, bounds_rect.bottom(), grid_pen);
  }
  for (double y = bounds_rect.top(); y <= bounds_rect.bottom(); y += 5.0 * scale) {
    canvas_scene.addLine(bounds_rect.left(), y, bounds_rect.right(), y, grid_pen);
  }

  // Wires
  QPen wire_pen(theme.symbol_pin_color);
  wire_pen.setWidthF(2.0);
  wire_pen.setCapStyle(Qt::RoundCap);
  wire_pen.setJoinStyle(Qt::RoundJoin);
  for (std::size_t i = 0; i < scene.wires.size(); ++i) {
    const ccad::CanvasWire& wire = scene.wires[i];
    const double sx = sceneX(scene, wire.start_x_units, margin, scale);
    const double sy = sceneY(scene, wire.start_y_units, margin, scale);
    const double ex = sceneX(scene, wire.end_x_units, margin, scale);
    const double ey = sceneY(scene, wire.end_y_units, margin, scale);
    
    auto* line = canvas_scene.addLine(sx, sy, ex, ey, wire_pen);
    line->setData(kCanvasObjectIdRole, QString("wire_%1").arg(i));
    line->setData(kCanvasObjectTypeRole, "Wire");
    line->setData(kCanvasObjectNetIdRole, qstr(wire.net_id));
    line->setToolTip(qstr(wire.net_id));
    line->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }

  // Buses
  QPen bus_pen(QColor(0, 0, 130)); // Dark blue for buses
  bus_pen.setWidthF(4.0); // Thicker than wires
  bus_pen.setCapStyle(Qt::RoundCap);
  bus_pen.setJoinStyle(Qt::RoundJoin);
  for (std::size_t i = 0; i < scene.bus_segments.size(); ++i) {
    const ccad::CanvasSchBus& bus = scene.bus_segments[i];
    const double sx = sceneX(scene, bus.start_x_units, margin, scale);
    const double sy = sceneY(scene, bus.start_y_units, margin, scale);
    const double ex = sceneX(scene, bus.end_x_units, margin, scale);
    const double ey = sceneY(scene, bus.end_y_units, margin, scale);
    
    auto* line = canvas_scene.addLine(sx, sy, ex, ey, bus_pen);
    line->setData(kCanvasObjectIdRole, QString("bus_%1").arg(i));
    line->setData(kCanvasObjectTypeRole, "Bus");
    line->setData(kCanvasObjectNetIdRole, qstr(bus.bus_id));
    line->setToolTip(QString("Bus: ") + qstr(bus.bus_id));
    line->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }

  // Components
  QPen component_pen(theme.symbol_body_color);
  component_pen.setWidthF(1.5);
  QBrush component_brush(theme.background_color);
  
  for (const ccad::CanvasComponent& comp : scene.symbols) {
    const double cx = sceneX(scene, comp.x_units, margin, scale);
    const double cy = sceneY(scene, comp.y_units, margin, scale);

    // If no symbol graphics, draw a fallback rectangle
    auto* rect = canvas_scene.addRect(cx - 20.0, cy - 20.0, 40.0, 40.0, component_pen, component_brush);
    rect->setData(kCanvasObjectIdRole, qstr(comp.id));
    rect->setData(kCanvasObjectTypeRole, "Component");
    rect->setToolTip(qstr(comp.id) + " (" + qstr(comp.part) + ")");
    rect->setFlag(QGraphicsItem::ItemIsSelectable, true);
    
    auto* label = canvas_scene.addText(qstr(comp.id));
    label->setDefaultTextColor(theme.board_label_color);
    label->setPos(cx - 20.0, cy - 45.0);
    
    auto* part_label = canvas_scene.addText(qstr(comp.part));
    part_label->setDefaultTextColor(theme.empty_text_color);
    part_label->setPos(cx - 20.0, cy + 25.0);
  }

  // Labels
  for (const ccad::CanvasLabel& clabel : scene.labels) {
    const double x = sceneX(scene, clabel.x_units, margin, scale);
    const double y = sceneY(scene, clabel.y_units, margin, scale);
    auto* text = canvas_scene.addText(qstr(clabel.text));
    text->setDefaultTextColor(theme.board_label_color);
    text->setPos(x, y);
    text->setRotation(clabel.rotation_degrees);
    text->setData(kCanvasObjectIdRole, qstr(clabel.id));
    text->setData(kCanvasObjectTypeRole, clabel.global ? "GlobalLabel" : "Label");
    text->setData(kCanvasObjectNetIdRole, qstr(clabel.net_id));
    text->setToolTip(qstr(clabel.id) + " (" + qstr(clabel.net_id) + ")");
    text->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }

  // Power Symbols
  QPen power_pen(theme.symbol_body_color);
  power_pen.setWidthF(1.5);
  for (const ccad::CanvasPowerSymbol& ps : scene.power_symbols) {
    const double x = sceneX(scene, ps.x_units, margin, scale);
    const double y = sceneY(scene, ps.y_units, margin, scale);
    
    auto* text = canvas_scene.addText(qstr(ps.value));
    text->setDefaultTextColor(theme.symbol_body_color);
    text->setPos(x, y - 25.0);
    text->setRotation(ps.rotation_degrees);
    text->setData(kCanvasObjectIdRole, qstr(ps.id));
    text->setData(kCanvasObjectTypeRole, "PowerSymbol");
    text->setData(kCanvasObjectNetIdRole, qstr(ps.net_id));
    text->setToolTip(qstr(ps.id) + " (" + qstr(ps.value) + ")");
    text->setFlag(QGraphicsItem::ItemIsSelectable, true);
    
    canvas_scene.addLine(x, y, x, y - 10.0, power_pen);
    canvas_scene.addLine(x - 5.0, y - 10.0, x + 5.0, y - 10.0, power_pen);
  }

  // Graphic Lines
  QPen graphic_pen(theme.drawing_color);
  graphic_pen.setCapStyle(Qt::RoundCap);
  graphic_pen.setJoinStyle(Qt::RoundJoin);
  for (const ccad::CanvasLine& line : scene.lines) {
    graphic_pen.setWidthF(std::max(0.2, line.width_units * scale));
    const double sx = sceneX(scene, line.start_x_units, margin, scale);
    const double sy = sceneY(scene, line.start_y_units, margin, scale);
    const double ex = sceneX(scene, line.end_x_units, margin, scale);
    const double ey = sceneY(scene, line.end_y_units, margin, scale);
    
    auto* item = canvas_scene.addLine(sx, sy, ex, ey, graphic_pen);
    item->setData(kCanvasObjectIdRole, qstr(line.id));
    item->setData(kCanvasObjectTypeRole, "GraphicLine");
    item->setToolTip(qstr(line.id));
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }

  // Graphic Arcs
  for (const ccad::CanvasArc& arc : scene.arcs) {
    graphic_pen.setWidthF(std::max(0.2, arc.width_units * scale));
    const double sx = sceneX(scene, arc.start_x_units, margin, scale);
    const double sy = sceneY(scene, arc.start_y_units, margin, scale);
    const double ex = sceneX(scene, arc.end_x_units, margin, scale);
    const double ey = sceneY(scene, arc.end_y_units, margin, scale);
    QPainterPath path;
    path.moveTo(sx, sy);
    if (arc.mid_x_units != 0.0 || arc.mid_y_units != 0.0) {
      path.quadTo(sceneX(scene, arc.mid_x_units, margin, scale), sceneY(scene, arc.mid_y_units, margin, scale), ex, ey);
    } else {
      path.lineTo(ex, ey);
    }
    auto* item = canvas_scene.addPath(path, graphic_pen);
    item->setData(kCanvasObjectIdRole, qstr(arc.id));
    item->setData(kCanvasObjectTypeRole, "GraphicArc");
    item->setToolTip(qstr(arc.id));
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }

  // Graphic Circles (Junctions, etc)
  for (const ccad::CanvasCircle& circle : scene.circles) {
    graphic_pen.setWidthF(std::max(0.2, circle.width_units * scale));
    const double radius = circle.radius_units * scale;
    const double cx = sceneX(scene, circle.center_x_units, margin, scale);
    const double cy = sceneY(scene, circle.center_y_units, margin, scale);
    
    QBrush brush = Qt::NoBrush;
    if (circle.fill_type == "background") brush = QBrush(theme.background_color);
    else if (circle.fill_type == "solid") brush = QBrush(theme.symbol_pin_color);

    auto* item = canvas_scene.addEllipse(cx - radius, cy - radius, radius * 2.0, radius * 2.0, graphic_pen, brush);
    item->setData(kCanvasObjectIdRole, qstr(circle.id));
    item->setData(kCanvasObjectTypeRole, "GraphicCircle");
    item->setToolTip(qstr(circle.id));
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }

  // Graphic Polygons
  for (const ccad::CanvasPolygon& poly : scene.polygons) {
    graphic_pen.setWidthF(std::max(0.2, poly.width_units * scale));
    QPolygonF qpoly;
    for (std::size_t j = 0; j < poly.pts_x_units.size() && j < poly.pts_y_units.size(); ++j) {
      qpoly.append(QPointF(sceneX(scene, poly.pts_x_units[j], margin, scale),
                           sceneY(scene, poly.pts_y_units[j], margin, scale)));
    }
    QBrush brush = Qt::NoBrush;
    if (poly.fill_type == "background") brush = QBrush(theme.background_color);
    else if (poly.fill_type == "solid") brush = QBrush(theme.drawing_color);
    
    auto* item = canvas_scene.addPolygon(qpoly, graphic_pen, brush);
    item->setData(kCanvasObjectIdRole, qstr(poly.id));
    item->setData(kCanvasObjectTypeRole, "GraphicPolygon");
    item->setToolTip(qstr(poly.id));
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }

  // Graphic Texts
  for (const ccad::CanvasText& text : scene.texts) {
    const double x = sceneX(scene, text.x_units, margin, scale);
    const double y = sceneY(scene, text.y_units, margin, scale);
    auto* item = canvas_scene.addText(qstr(text.text));
    item->setDefaultTextColor(theme.drawing_color);
    item->setPos(x, y);
    item->setRotation(text.rotation_degrees);
    item->setData(kCanvasObjectIdRole, qstr(text.id));
    item->setData(kCanvasObjectTypeRole, "GraphicText");
    item->setToolTip(qstr(text.id));
    item->setFlag(QGraphicsItem::ItemIsSelectable, true);
  }
}
