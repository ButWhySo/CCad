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
}
