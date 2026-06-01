#include "review_window.hpp"

#include "board_canvas_renderer.hpp"
#include "board_canvas_view.hpp"
#include "ccad_core/canvas.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/component_generator.hpp"
#include "ccad_core/component_generator.hpp"
#include "ccad_core/drc.hpp"
#include "ccad_gui/component_wizard_dialog.hpp"
#include "ccad_gui/footprint_placement_dialog.hpp"
#include "ccad_gui/library_browser_dialog.hpp"
#include "ccad_core/kicad_symbol_import.hpp"
#include "ccad_core/kicad_footprint_import.hpp"
#include "ccad_core/placement.hpp"
#include "ccad_core/json.hpp"
#include "symbol_placement_dialog.hpp"
#include "footprint_placement_dialog.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QDockWidget>
#include <QIcon>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QKeySequence>
#include <QScrollArea>
#include <QScreen>
#include <QSize>
#include <QStatusBar>
#include <QStringList>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QTextStream>
#include <QToolButton>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <cstdlib>

QString formatCursorStatus(const std::optional<ccad::Board>& board, const QPointF& scene_position) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  if (!board.has_value()) {
    return "X --  Y --";
  }

  const ccad::Rect outline = board->outline;
  const double origin_x_mm = outline.origin.x.nanometers / 1000000.0;
  const double origin_y_mm = outline.origin.y.nanometers / 1000000.0;
  const double board_width_mm = outline.size.width.nanometers / 1000000.0;
  const double board_height_mm = outline.size.height.nanometers / 1000000.0;
  const double x_mm = origin_x_mm + ((scene_position.x() - margin) / scale);
  const double y_mm = origin_y_mm + ((scene_position.y() - margin) / scale);
  const bool inside_board = x_mm >= origin_x_mm && y_mm >= origin_y_mm &&
                            x_mm <= origin_x_mm + board_width_mm &&
                            y_mm <= origin_y_mm + board_height_mm;

  return (inside_board ? "Board " : "Canvas ") + QString("X ") +
         QString::number(x_mm, 'f', 2) + " mm  Y " + QString::number(y_mm, 'f', 2) + " mm";
}

namespace {

std::string readFile(const std::filesystem::path& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open file: " + path.string());
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void writeFile(const std::filesystem::path& path, const std::string& content) {
  std::ofstream output(path, std::ios::binary);
  if (!output) {
    throw std::runtime_error("failed to open file for writing: " + path.string());
  }
  output.write(content.data(), content.size());
}

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

std::filesystem::path kicadSourceRoot() {
  if (const char* env = std::getenv("CCAD_KICAD_SRC")) {
    return std::filesystem::path(env);
  }
  const std::filesystem::path cwd = std::filesystem::current_path();
  const std::filesystem::path sibling = cwd.parent_path() / "kicad_src";
  if (std::filesystem::exists(sibling)) {
    return sibling;
  }
  return cwd / "kicad_src";
}

QIcon kicadIcon(const std::string& name) {
  const std::filesystem::path path =
      kicadSourceRoot() / "resources" / "bitmaps_png" / "sources" / "light" / (name + ".svg");
  if (std::filesystem::exists(path)) {
    return QIcon(qstr(path.string()));
  }
  return QIcon();
}

QAction* addIconAction(QToolBar& toolbar, const std::string& icon_name, const QString& text) {
  auto* action = toolbar.addAction(kicadIcon(icon_name), text);
  action->setToolTip(text);
  action->setStatusTip(text);
  return action;
}

QPainterPath trapezoidPreviewPath(const QRectF& rect) {
  const double inset = std::min(rect.width(), rect.height()) * 0.20;
  QPainterPath path;
  path.moveTo(rect.left() + inset, rect.top());
  path.lineTo(rect.right(), rect.top());
  path.lineTo(rect.right() - inset, rect.bottom());
  path.lineTo(rect.left(), rect.bottom());
  path.closeSubpath();
  return path;
}

QPainterPath chamferedRectPreviewPath(const QRectF& rect,
                                      const std::optional<double> chamfer_ratio) {
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

QPainterPath padPreviewPath(const double x_mm, const double y_mm, const double width_mm,
                            const double height_mm, const std::string& shape,
                            const double rotation_degrees,
                            const std::optional<double> roundrect_rratio,
                            const std::optional<double> chamfer_ratio) {
  constexpr double scale = 10.0;
  const QPointF center(x_mm * scale, y_mm * scale);
  const QRectF rect(center.x() - ((width_mm * scale) / 2.0),
                    center.y() - ((height_mm * scale) / 2.0), width_mm * scale,
                    height_mm * scale);
  QPainterPath path;
  if (shape == "rect") {
    path.addRect(rect);
  } else if (shape == "roundrect" || shape == "rounded_rect") {
    const double ratio = std::clamp(roundrect_rratio.value_or(0.25), 0.0, 0.5);
    const double radius = std::min(rect.width(), rect.height()) * ratio;
    path.addRoundedRect(rect, radius, radius);
  } else if (shape == "circle") {
    const double diameter = std::min(rect.width(), rect.height());
    path.addEllipse(QRectF(center.x() - (diameter / 2.0), center.y() - (diameter / 2.0),
                           diameter, diameter));
  } else if (shape == "trapezoid") {
    path = trapezoidPreviewPath(rect);
  } else if (shape == "chamfered_rect") {
    path = chamferedRectPreviewPath(rect, chamfer_ratio);
  } else {
    path.addEllipse(rect);
  }
  if (rotation_degrees != 0.0) {
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(rotation_degrees);
    transform.translate(-center.x(), -center.y());
    path = transform.map(path);
  }
  return path;
}

void addPadPreviewItems(QGraphicsScene& scene, std::vector<QGraphicsItem*>& items,
                        const ccad::Footprint& footprint, const QColor& color) {
  QPen pen(color.darker(130), 1.0);
  QBrush brush(QColor(color.red(), color.green(), color.blue(), 150));
  for (const auto& pad : footprint.pads) {
    const double w = pad.size.width.nanometers / 1e6;
    const double h = pad.size.height.nanometers / 1e6;
    const double x = pad.position.x.nanometers / 1e6;
    const double y = pad.position.y.nanometers / 1e6;
    auto* item = scene.addPath(padPreviewPath(x, y, w, h, pad.shape, pad.rotation_degrees,
                                              pad.roundrect_rratio, pad.chamfer_ratio),
                               pen, brush);
    item->setZValue(1000);
    items.push_back(item);
    if (pad.drill.has_value()) {
      constexpr double scale = 10.0;
      const double drill = pad.drill->nanometers / 1e6 * scale;
      auto* drill_item = scene.addEllipse((x * scale) - (drill / 2.0),
                                          (y * scale) - (drill / 2.0), drill, drill,
                                          QPen(Qt::NoPen), QBrush(QColor("#07111f")));
      drill_item->setZValue(1001);
      items.push_back(drill_item);
    }
  }
}

ccad::Point boardPointFromScene(const ccad::Board& board, const QPointF& scene_position) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double origin_x_mm = board.outline.origin.x.nanometers / 1e6;
  const double origin_y_mm = board.outline.origin.y.nanometers / 1e6;
  return {ccad::millimeters(origin_x_mm + ((scene_position.x() - margin) / scale)),
          ccad::millimeters(origin_y_mm + ((scene_position.y() - margin) / scale))};
}

QPointF boardPositionToScene(const ccad::Board& board, const double x_mm, const double y_mm) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double origin_x_mm = board.outline.origin.x.nanometers / 1e6;
  const double origin_y_mm = board.outline.origin.y.nanometers / 1e6;
  return QPointF(margin + ((x_mm - origin_x_mm) * scale),
                 margin + ((y_mm - origin_y_mm) * scale));
}

ccad::Point boardDeltaFromSceneDelta(const QPointF& scene_delta) {
  constexpr double scale = 10.0;
  return {.x = ccad::millimeters(scene_delta.x() / scale),
          .y = ccad::millimeters(scene_delta.y() / scale)};
}

ccad::Point schematicPointFromScene(const QPointF& scene_position) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  return {ccad::millimeters((scene_position.x() - margin) / scale),
          ccad::millimeters((scene_position.y() - margin) / scale)};
}

std::string placementPrefixFromName(const std::string& name) {
  if (name.starts_with("R") || name.starts_with("Resistor")) return "R";
  if (name.starts_with("C") || name.starts_with("Capacitor")) return "C";
  if (name.starts_with("D") || name.starts_with("Diode")) return "D";
  if (name.starts_with("Q")) return "Q";
  if (name.starts_with("L")) return "L";
  if (name.starts_with("J") || name.starts_with("Connector")) return "J";
  return "U";
}

std::string nextComponentId(const ccad::Project& project, const std::string& prefix) {
  int max_num = 0;
  for (const ccad::Component& component : project.components) {
    if (!component.id.starts_with(prefix)) {
      continue;
    }
    try {
      max_num = std::max(max_num, std::stoi(component.id.substr(prefix.size())));
    } catch (...) {
    }
  }
  if (project.board.has_value()) {
    for (const ccad::Pad& pad : project.board->pads) {
      if (!pad.component_id.starts_with(prefix)) {
        continue;
      }
      try {
        max_num = std::max(max_num, std::stoi(pad.component_id.substr(prefix.size())));
      } catch (...) {
      }
    }
  }
  return prefix + std::to_string(max_num + 1);
}

std::string firstCopperLayerId(const ccad::Board& board) {
  for (const ccad::Layer& layer : board.layers) {
    if (layer.kind == "copper" && layer.id == "F.Cu") {
      return layer.id;
    }
  }
  for (const ccad::Layer& layer : board.layers) {
    if (layer.kind == "copper") {
      return layer.id;
    }
  }
  return {};
}

ccad::Footprint loadFootprintSelection(const std::filesystem::path& path) {
  const std::string content = readFile(path);
  const std::string extension = path.extension().string();
  if (extension == ".kicad_mod") {
    return ccad::importKiCadFootprint(content);
  }
  return ccad::loadFootprintJson(content);
}

ccad::Symbol loadSymbolSelection(const std::filesystem::path& path) {
  const std::string content = readFile(path);
  const std::string extension = path.extension().string();
  if (extension == ".kicad_sym") {
    const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(content);
    auto selected = std::find_if(symbols.begin(), symbols.end(), [](const ccad::Symbol& symbol) {
      return !symbol.pins.empty();
    });
    if (selected != symbols.end()) {
      return *selected;
    }
    if (!symbols.empty()) {
      return symbols.front();
    }
    throw std::runtime_error("symbol library has no symbols");
  }
  return ccad::loadSymbolJsonFileWithLocalInheritance(path);
}

QPainterPath sceneLinePath(double sx, double sy, double ex, double ey) {
  QPainterPath path;
  path.moveTo(sx, sy);
  path.lineTo(ex, ey);
  return path;
}

void moveGhostTo(QGraphicsView& view, std::vector<QGraphicsItem*>& items, const QPointF& scene_pos) {
  if (items.empty()) {
    return;
  }
  QRectF bounds;
  for (QGraphicsItem* item : items) {
    bounds = bounds.isNull() ? item->sceneBoundingRect() : bounds.united(item->sceneBoundingRect());
  }
  const QPointF delta = scene_pos - bounds.center();
  for (QGraphicsItem* item : items) {
    item->setPos(item->pos() + delta);
  }
  view.viewport()->setCursor(Qt::CrossCursor);
}

void addSymbolPreviewItems(QGraphicsScene& scene, std::vector<QGraphicsItem*>& items,
                           const ccad::Symbol& symbol, const QColor& color) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const ccad::CanvasScene symbol_scene = ccad::buildCanvasScene(symbol);
  QPen pen(color, 1.2);
  pen.setCapStyle(Qt::RoundCap);
  pen.setJoinStyle(Qt::RoundJoin);
  QBrush translucent(QColor(color.red(), color.green(), color.blue(), 42));

  for (const ccad::CanvasLine& line : symbol_scene.lines) {
    auto* item = scene.addPath(sceneLinePath(margin + (line.start_x_units * scale),
                                             margin + (line.start_y_units * scale),
                                             margin + (line.end_x_units * scale),
                                             margin + (line.end_y_units * scale)),
                               pen, QBrush(Qt::NoBrush));
    item->setZValue(1000);
    items.push_back(item);
  }
  for (const ccad::CanvasCircle& circle : symbol_scene.circles) {
    const double radius = circle.radius_units * scale;
    auto* item = scene.addEllipse(margin + (circle.center_x_units * scale) - radius,
                                  margin + (circle.center_y_units * scale) - radius,
                                  radius * 2.0, radius * 2.0, pen, QBrush(Qt::NoBrush));
    item->setZValue(1000);
    items.push_back(item);
  }
  for (const ccad::CanvasPolygon& poly : symbol_scene.polygons) {
    QPolygonF polygon;
    for (std::size_t i = 0; i < poly.pts_x_units.size() && i < poly.pts_y_units.size(); ++i) {
      polygon << QPointF(margin + (poly.pts_x_units.at(i) * scale),
                         margin + (poly.pts_y_units.at(i) * scale));
    }
    QPainterPath path;
    path.addPolygon(polygon);
    auto* item = scene.addPath(path, pen, poly.fill_type == "solid" ? translucent : QBrush(Qt::NoBrush));
    item->setZValue(1000);
    items.push_back(item);
  }
  for (const ccad::SymbolPin& pin : symbol.pins) {
    const double x = margin + ((pin.position.x.nanometers / 1e6) * scale);
    const double y = margin + ((pin.position.y.nanometers / 1e6) * scale);
    auto* item = scene.addEllipse(x - 2.5, y - 2.5, 5.0, 5.0, pen, translucent);
    item->setZValue(1001);
    items.push_back(item);
  }
  auto* label = scene.addText(QString::fromStdString(symbol.name));
  label->setDefaultTextColor(color);
  label->setPos(margin, margin - 18.0);
  label->setZValue(1001);
  items.push_back(label);
}

}  // namespace

ReviewWindow::ReviewWindow() {
  setWindowTitle("CCad PCB Editor");
  if (const QScreen* screen = QGuiApplication::primaryScreen()) {
    const QRect available = screen->availableGeometry();
    const int width = std::max(1360, (available.width() * 88) / 100);
    const int height = std::max(860, (available.height() * 88) / 100);
    resize(width, height);
  } else {
    resize(1360, 860);
  }
  applyStyle();

  project_summary_ = new ProjectSummaryPanel(this);

  auto* project_dock = new QDockWidget("Project", this);
  project_dock->setObjectName("projectDock");
  project_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  project_dock->setMinimumWidth(300);
  auto* project_scroll = new QScrollArea(project_dock);
  project_scroll->setWidgetResizable(true);
  project_scroll->setFrameShape(QFrame::NoFrame);
  project_scroll->setWidget(project_summary_);
  project_dock->setWidget(project_scroll);
  addDockWidget(Qt::LeftDockWidgetArea, project_dock);

  diagnostics_ = new DiagnosticsPanel(this);
  transaction_timeline_ = new TransactionTimelinePanel(this);

  auto* diagnostics_dock = new QDockWidget("Diagnostics", this);
  diagnostics_dock->setObjectName("diagnosticsDock");
  diagnostics_dock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
  auto* bottom_tabs = new QTabWidget(diagnostics_dock);
  bottom_tabs->setObjectName("bottomReviewTabs");
  bottom_tabs->addTab(diagnostics_, "Diagnostics");
  bottom_tabs->addTab(transaction_timeline_, "Transactions");
  diagnostics_dock->setWidget(bottom_tabs);
  addDockWidget(Qt::BottomDockWidgetArea, diagnostics_dock);

  auto* right_panel = new QWidget(this);
  auto* right_layout = new QVBoxLayout(right_panel);
  right_layout->setContentsMargins(8, 8, 8, 8);
  right_layout->setSpacing(8);
  selection_inspector_ = new SelectionInspectorPanel(right_panel);
  selection_inspector_->setObjectName("selectionInspectorPanel");
  object_browser_ = new ObjectBrowserPanel(right_panel);
  right_layout->addWidget(selection_inspector_);
  right_layout->addWidget(object_browser_, 1);
  auto* layers_dock = new QDockWidget("Layers / Objects", this);
  layers_dock->setObjectName("layersDock");
  layers_dock->setAllowedAreas(Qt::RightDockWidgetArea | Qt::LeftDockWidgetArea);
  layers_dock->setMinimumWidth(280);
  layers_dock->setWidget(right_panel);
  addDockWidget(Qt::RightDockWidgetArea, layers_dock);

  canvas_scene_ = new QGraphicsScene(this);
  auto* board_view = new BoardCanvasView(canvas_scene_, this);
  canvas_view_ = board_view;
  canvas_view_->setObjectName("boardCanvas");
  canvas_view_->setRenderHint(QPainter::Antialiasing);
  canvas_view_->viewport()->installEventFilter(this);
  canvas_view_->setDragMode(QGraphicsView::NoDrag);
  canvas_view_->setFrameShape(QFrame::NoFrame);
  canvas_view_->setMouseTracking(true);
  board_view->setCoordinateCallback(
      [this](const QPointF& scene_position, const double zoom_factor) {
        updateCursorStatus(scene_position, zoom_factor);
      });
  cursor_status_ = new QLabel("X --  Y --", this);
  zoom_status_ = new QLabel("Zoom 100%", this);
  tool_status_ = new QLabel("Tool Select", this);
  layer_status_ = new QLabel("Layer F.Cu", this);
  selection_status_ = new QLabel("Selected --", this);
  statusBar()->addPermanentWidget(cursor_status_);
  statusBar()->addPermanentWidget(zoom_status_);
  statusBar()->addPermanentWidget(selection_status_);
  statusBar()->addPermanentWidget(tool_status_);
  statusBar()->addPermanentWidget(layer_status_);
  statusBar()->showMessage("Ready");

  board_view->setPanModeCallback([this](const bool space_mode, const bool dragging) {
    if (tool_status_ == nullptr) {
      return;
    }
    if (dragging) {
      tool_status_->setText("Tool Pan Drag");
      return;
    }
    if (space_mode) {
      tool_status_->setText("Tool Pan Ready");
      return;
    }
    tool_status_->setText("Tool Select");
  });

  schematic_scene_ = new QGraphicsScene(this);
  auto* schematic_board_view = new BoardCanvasView(schematic_scene_, this);
  schematic_view_ = schematic_board_view;
  schematic_view_->setObjectName("schematicCanvas");
  schematic_view_->setRenderHint(QPainter::Antialiasing);
  schematic_view_->viewport()->installEventFilter(this);
  schematic_view_->setDragMode(QGraphicsView::NoDrag);
  schematic_view_->setFrameShape(QFrame::NoFrame);
  schematic_view_->setMouseTracking(true);
  schematic_board_view->setCoordinateCallback(
      [this](const QPointF& scene_position, const double zoom_factor) {
        updateCursorStatus(scene_position, zoom_factor);
      });
  schematic_board_view->setPanModeCallback([this](const bool space_mode, const bool dragging) {
    if (tool_status_ == nullptr) return;
    if (dragging) { tool_status_->setText("Tool Pan Drag"); return; }
    if (space_mode) { tool_status_->setText("Tool Pan Ready"); return; }
    tool_status_->setText("Tool Select");
  });

  editor_tabs_ = new QTabWidget(this);
  editor_tabs_->setObjectName("editorTabs");
  editor_tabs_->addTab(canvas_view_, "PCB");
  editor_tabs_->addTab(schematic_view_, "Schematic");

  setCentralWidget(editor_tabs_);
  setDockNestingEnabled(true);
  resizeDocks({project_dock, layers_dock}, {360, 320}, Qt::Horizontal);
  resizeDocks({project_dock, diagnostics_dock}, {620, 240}, Qt::Vertical);

  auto* open_action = new QAction(kicadIcon("directory_open"), "Open", this);
  auto* reload_action = new QAction(kicadIcon("reload"), "Reload", this);
  auto* save_action = new QAction(kicadIcon("save"), "Save", this);
  auto* board_setup_action = new QAction(kicadIcon("options_board"), "Board Setup", this);
  undo_action_ = new QAction(kicadIcon("undo"), "Undo", this);
  redo_action_ = new QAction(kicadIcon("redo"), "Redo", this);
  auto* run_drc_action = new QAction(kicadIcon("drc"), "Run DRC", this);
  auto* export_drc_action = new QAction(kicadIcon("export"), "Export DRC Report...", this);
  auto* fit_action = new QAction(kicadIcon("zoom_fit_in_page"), "Fit", this);
  auto* zoom_in_action = new QAction(kicadIcon("zoom_in"), "Zoom In", this);
  auto* zoom_out_action = new QAction(kicadIcon("zoom_out"), "Zoom Out", this);
  auto* zoom_100_action = new QAction("100%", this);
  auto* navigation_help_action = new QAction("Navigation Controls", this);
  auto* quit_action = new QAction("Quit", this);
  save_action->setShortcut(QKeySequence::Save);
  undo_action_->setShortcut(QKeySequence::Undo);
  redo_action_->setShortcut(QKeySequence::Redo);
  fit_action->setShortcut(QKeySequence(Qt::Key_F));
  zoom_in_action->setShortcuts(
      {QKeySequence(Qt::Key_Plus), QKeySequence(Qt::CTRL | Qt::Key_Equal)});
  zoom_out_action->setShortcut(QKeySequence(Qt::Key_Minus));
  zoom_100_action->setShortcut(QKeySequence(Qt::Key_0));
  navigation_help_action->setShortcut(QKeySequence(Qt::Key_F1));

  connect(open_action, &QAction::triggered, this, [this]() { openProject(); });
  connect(reload_action, &QAction::triggered, this, [this]() { reloadProject(); });
  connect(save_action, &QAction::triggered, this, [this]() { saveProject(); });
  connect(board_setup_action, &QAction::triggered, this, [this]() { showBoardSetup(); });
  connect(undo_action_, &QAction::triggered, this, [this]() {
    if (undo_stack_.empty()) return;
    redo_stack_.push_back(project_cache_);
    const ccad::Project snapshot = undo_stack_.back();
    undo_stack_.pop_back();
    restoreProjectSnapshot(snapshot);
  });
  connect(redo_action_, &QAction::triggered, this, [this]() {
    if (redo_stack_.empty()) return;
    undo_stack_.push_back(project_cache_);
    const ccad::Project snapshot = redo_stack_.back();
    redo_stack_.pop_back();
    restoreProjectSnapshot(snapshot);
  });
  connect(run_drc_action, &QAction::triggered, this, [this]() { runDrcFromToolbar(); });
  connect(export_drc_action, &QAction::triggered, this, [this]() { exportDrcReport(); });
  connect(fit_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomToFit();
  });
  connect(zoom_in_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomIn();
  });
  connect(zoom_out_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->zoomOut();
  });
  connect(zoom_100_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) view->resetZoom();
  });
  connect(navigation_help_action, &QAction::triggered, this,
          [this]() { showNavigationHelp(); });
  connect(quit_action, &QAction::triggered, this, [this]() { close(); });

  auto* file_menu = menuBar()->addMenu("File");
  file_menu->addAction(open_action);
  file_menu->addAction(reload_action);
  file_menu->addSeparator();
  file_menu->addAction(quit_action);

  auto* tools_menu = menuBar()->addMenu(tr("&Tools"));
  tools_menu->addAction(tr("Component Wizard..."), this, &ReviewWindow::showComponentWizard);
  tools_menu->addAction(run_drc_action);
  tools_menu->addAction(export_drc_action);

  auto* help_menu = menuBar()->addMenu("Help");
  help_menu->addAction(navigation_help_action);

  auto* top_toolbar = addToolBar("Top Toolbar");
  top_toolbar->setMovable(false);
  top_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  top_toolbar->setIconSize(QSize(24, 24));
  top_toolbar->addAction(open_action);
  top_toolbar->addAction(reload_action);
  top_toolbar->addSeparator();
  top_toolbar->addAction(save_action);
  top_toolbar->addAction(board_setup_action);
  top_toolbar->addSeparator();
  top_toolbar->addAction(undo_action_);
  top_toolbar->addAction(redo_action_);
  top_toolbar->addSeparator();
  top_toolbar->addAction(fit_action);
  top_toolbar->addAction(zoom_in_action);
  top_toolbar->addAction(zoom_out_action);
  top_toolbar->addAction(zoom_100_action);
  top_toolbar->addSeparator();
  top_toolbar->addAction(run_drc_action);
  updateUndoRedoActions();

  auto* left_toolbar = new QToolBar("Left Toolbar", this);
  left_toolbar->setMovable(false);
  left_toolbar->setOrientation(Qt::Vertical);
  left_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  left_toolbar->setIconSize(QSize(24, 24));
  addToolBar(Qt::LeftToolBarArea, left_toolbar);
  addIconAction(*left_toolbar, "grid", "Toggle Grid");
  addIconAction(*left_toolbar, "polar_coord", "Polar Coordinates");
  left_toolbar->addSeparator();
  addIconAction(*left_toolbar, "unit_inch", "Toggle Units");
  left_toolbar->addSeparator();
  addIconAction(*left_toolbar, "cursor_shape", "Crosshair Cursor");
  left_toolbar->addSeparator();
  addIconAction(*left_toolbar, "show_ratsnest", "Show Ratsnest");
  addIconAction(*left_toolbar, "net_highlight", "Net Highlight");
  left_toolbar->addSeparator();
  addIconAction(*left_toolbar, "contrast_mode", "Display Modes");
  left_toolbar->addSeparator();
  addIconAction(*left_toolbar, "layers_manager", "Show Layers");
  addIconAction(*left_toolbar, "part_properties", "Show Properties");

  auto* right_toolbar = new QToolBar("Right Toolbar", this);
  right_toolbar->setMovable(false);
  right_toolbar->setOrientation(Qt::Vertical);
  right_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
  right_toolbar->setIconSize(QSize(24, 24));
  addToolBar(Qt::RightToolBarArea, right_toolbar);
  auto* select_action = addIconAction(*right_toolbar, "cursor", "Select");
  addIconAction(*right_toolbar, "tool_ratsnest", "Local Ratsnest");
  right_toolbar->addSeparator();
  auto* add_footprint_action = addIconAction(*right_toolbar, "new_footprint", "Add Footprint");
  add_footprint_action->setShortcut(QKeySequence(Qt::Key_O));
  auto* add_symbol_action = addIconAction(*right_toolbar, "add_symbol_to_schematic", "Add Symbol");
  add_symbol_action->setShortcut(QKeySequence(Qt::Key_A));
  addIconAction(*right_toolbar, "add_tracks", "Route Track");
  addIconAction(*right_toolbar, "add_via", "Add Via");
  addIconAction(*right_toolbar, "add_zone", "Add Zone");
  addIconAction(*right_toolbar, "add_keepout_area", "Add Keepout");
  right_toolbar->addSeparator();
  addIconAction(*right_toolbar, "add_graphical_segments", "Draw Graphic");
  addIconAction(*right_toolbar, "text", "Place Text");
  right_toolbar->addSeparator();
  addIconAction(*right_toolbar, "delete_cursor", "Delete");
  auto* measure_action = addIconAction(*right_toolbar, "measurement", "Measure");

  connect(add_footprint_action, &QAction::triggered, this, [this]() { placeFromActiveEditor(); });
  connect(add_symbol_action, &QAction::triggered, this, [this]() { placeFromActiveEditor(); });
  connect(editor_tabs_, &QTabWidget::currentChanged, this,
          [add_footprint_action, add_symbol_action](const int index) {
            const bool pcb_tab = index == 0;
            add_footprint_action->setVisible(pcb_tab);
            add_symbol_action->setVisible(!pcb_tab);
          });
  add_footprint_action->setVisible(true);
  add_symbol_action->setVisible(false);

  connect(select_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) {
      view->setToolMode(ToolMode::Select);
      tool_status_->setText("Tool Select");
    }
  });

  connect(measure_action, &QAction::triggered, this, [this]() {
    if (auto* view = dynamic_cast<BoardCanvasView*>(editor_tabs_->currentWidget())) {
      view->setToolMode(ToolMode::Measure);
      tool_status_->setText("Tool Measure");
    }
  });

  connect(canvas_scene_, &QGraphicsScene::selectionChanged, this,
          [this]() { updateSelectionStatus(); });
  connect(diagnostics_, &QTableWidget::cellClicked, this, [this](const int row, int) {
    selectCanvasObjectById(*canvas_scene_, diagnostics_->objectIdForRow(row));
  });
  object_browser_->setObjectActivatedCallback(
      [this](const QString& object_id) { selectCanvasObjectById(*canvas_scene_, object_id); });
  object_browser_->setNetActivatedCallback(
      [this](const QString& net_id) { selectCanvasObjectsByNetId(*canvas_scene_, net_id); });
  object_browser_->setRouteActivatedCallback([this](const QString& route_request_id) {
    selectCanvasObjectsByRouteRequestId(*canvas_scene_, route_request_id);
  });
  object_browser_->setLayerToggledCallback([this](const QString& layer_id, bool visible) {
    if (!project_cache_.board) {
      return;
    }
    pushUndoSnapshot();
    for (auto& layer : project_cache_.board->layers) {
      if (QString::fromStdString(layer.id) == layer_id) {
        layer.visible = visible;
        break;
      }
    }
    renderReview(ccad::buildReview(project_cache_));
  });

  selection_inspector_->setDesignRulesChangedCallback([this](const ccad::DesignRules& rules) {
    if (!project_cache_.board) return;
    pushUndoSnapshot();
    project_cache_.board->design_rules = rules;
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated design rules to project file");
    } catch (const std::exception& e) {
      QMessageBox::warning(this, "Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selection_inspector_->renderBoardRules(project_cache_.board);
  });

  selection_inspector_->setTrackChangedCallback([this](const QString& id, double width_mm) {
    if (!project_cache_.board) return;
    pushUndoSnapshot();
    for (auto& track : project_cache_.board->tracks) {
      if (QString::fromStdString(track.id) == id) {
        track.width = ccad::millimeters(width_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated track segment to project file");
    } catch (const std::exception& e) {
      QMessageBox::warning(this, "Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setViaChangedCallback([this](const QString& id, double diameter_mm, double drill_mm) {
    if (!project_cache_.board) return;
    pushUndoSnapshot();
    for (auto& via : project_cache_.board->vias) {
      if (QString::fromStdString(via.id) == id) {
        via.diameter = ccad::millimeters(diameter_mm);
        via.drill = ccad::millimeters(drill_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated via to project file");
    } catch (const std::exception& e) {
      QMessageBox::warning(this, "Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setPadChangedCallback([this](const QString& id, double width_mm, double height_mm, double rotation_deg) {
    if (!project_cache_.board) return;
    pushUndoSnapshot();
    for (auto& pad : project_cache_.board->pads) {
      if (QString::fromStdString(pad.id) == id) {
        pad.size.width = ccad::millimeters(width_mm);
        pad.size.height = ccad::millimeters(height_mm);
        pad.rotation_degrees = rotation_deg;
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated pad to project file");
    } catch (const std::exception& e) {
      QMessageBox::warning(this, "Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setKeepoutChangedCallback([this](const QString& id, double width_mm, double height_mm) {
    if (!project_cache_.board) return;
    pushUndoSnapshot();
    for (auto& keepout : project_cache_.board->keepouts) {
      if (QString::fromStdString(keepout.id) == id) {
        keepout.area.size.width = ccad::millimeters(width_mm);
        keepout.area.size.height = ccad::millimeters(height_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated keepout to project file");
    } catch (const std::exception& e) {
      QMessageBox::warning(this, "Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  selection_inspector_->setRegionChangedCallback([this](const QString& id, double width_mm, double height_mm) {
    if (!project_cache_.board) return;
    pushUndoSnapshot();
    for (auto& pr : project_cache_.board->placement_regions) {
      if (QString::fromStdString(pr.id) == id) {
        pr.area.size.width = ccad::millimeters(width_mm);
        pr.area.size.height = ccad::millimeters(height_mm);
        break;
      }
    }
    try {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
      statusBar()->showMessage("Saved updated placement region to project file");
    } catch (const std::exception& e) {
      QMessageBox::warning(this, "Save failed", QString::fromStdString(e.what()));
    }
    renderReview(ccad::buildReview(project_cache_));
    selectCanvasObjectById(*canvas_scene_, id);
  });

  transaction_timeline_->renderTransactions({});
}

void ReviewWindow::showNavigationHelp() {
  const QStringList lines{
      "Mouse:",
      "  - Wheel: zoom in/out",
      "  - Middle drag: pan",
      "  - Right drag: pan",
      "  - Shift + Left drag: pan",
      "  - Hold Space + Left drag: hand-pan",
      "",
      "Keyboard:",
      "  - + / -: zoom in/out",
      "  - 0: reset zoom to 100%",
      "  - F or Home: fit board to view",
      "  - Arrow keys: pan",
      "  - W / A / S / D: pan",
      "  - F1: open this help",
  };
  QMessageBox::information(this, "Navigation Controls", lines.join('\n'));
}

void ReviewWindow::pushUndoSnapshot() {
  undo_stack_.push_back(project_cache_);
  redo_stack_.clear();
  updateUndoRedoActions();
}

void ReviewWindow::restoreProjectSnapshot(const ccad::Project& snapshot) {
  project_cache_ = snapshot;
  try {
    if (!current_path_.empty()) {
      writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
    }
    renderReview(ccad::buildReview(project_cache_));
    statusBar()->showMessage("Restored project snapshot");
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Restore failed", QString::fromStdString(e.what()));
  }
  updateUndoRedoActions();
}

void ReviewWindow::updateUndoRedoActions() {
  if (undo_action_ != nullptr) {
    undo_action_->setEnabled(!undo_stack_.empty());
  }
  if (redo_action_ != nullptr) {
    redo_action_->setEnabled(!redo_stack_.empty());
  }
}

void ReviewWindow::saveProject() {
  if (current_path_.empty()) {
    QMessageBox::warning(this, "Save Project", "No project file is loaded.");
    return;
  }
  try {
    writeFile(current_path_, ccad::dumpProjectJson(project_cache_));
    statusBar()->showMessage("Saved project file");
  } catch (const std::exception& e) {
    QMessageBox::critical(this, "Save failed", QString::fromStdString(e.what()));
  }
}

void ReviewWindow::showBoardSetup() {
  if (!project_cache_.board.has_value()) {
    QMessageBox::warning(this, "Board Setup", "Load a project with a board first.");
    return;
  }

  QDialog dialog(this);
  dialog.setWindowTitle("Board Setup");
  auto* layout = new QVBoxLayout(&dialog);
  auto* form = new QFormLayout();
  auto* clearance = new QDoubleSpinBox(&dialog);
  auto* min_track = new QDoubleSpinBox(&dialog);
  auto* min_ring = new QDoubleSpinBox(&dialog);
  for (QDoubleSpinBox* spin : {clearance, min_track, min_ring}) {
    spin->setDecimals(3);
    spin->setRange(0.001, 1000.0);
    spin->setSuffix(" mm");
  }
  clearance->setValue(project_cache_.board->design_rules.copper_clearance.nanometers / 1e6);
  min_track->setValue(project_cache_.board->design_rules.min_track_width.nanometers / 1e6);
  min_ring->setValue(project_cache_.board->design_rules.min_via_annular_ring.nanometers / 1e6);
  form->addRow("Copper clearance:", clearance);
  form->addRow("Minimum track width:", min_track);
  form->addRow("Minimum via annular ring:", min_ring);
  layout->addLayout(form);
  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  pushUndoSnapshot();
  project_cache_.board->design_rules.copper_clearance = ccad::millimeters(clearance->value());
  project_cache_.board->design_rules.min_track_width = ccad::millimeters(min_track->value());
  project_cache_.board->design_rules.min_via_annular_ring = ccad::millimeters(min_ring->value());
  saveProject();
  renderReview(ccad::buildReview(project_cache_));
}

void ReviewWindow::runDrcFromToolbar() {
  const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
  diagnostics_->renderDiagnostics(diagnostics);
  const ccad::CanvasScene pcb_scene = ccad::buildCanvasScene(project_cache_);
  renderBoardCanvas(*canvas_scene_, pcb_scene);
  addDiagnosticMarkers(*canvas_scene_, diagnostics);
  object_browser_->renderScene(pcb_scene);
  statusBar()->showMessage("DRC complete: " + QString::number(diagnostics.size()) + " findings");
}

void ReviewWindow::placeFromActiveEditor() {
  if (editor_tabs_->currentWidget() == schematic_view_) {
    chooseAndPlaceSymbol();
    return;
  }
  chooseAndPlaceFootprint();
}

void ReviewWindow::chooseAndPlaceFootprint() {
  if (!project_cache_.board.has_value()) {
    QMessageBox::warning(this, "No Board", "Load a project with a board before placing footprints.");
    return;
  }
  const std::string layer_id = firstCopperLayerId(*project_cache_.board);
  if (layer_id.empty()) {
    QMessageBox::warning(this, "No Copper Layer", "No copper layer is available for footprint placement.");
    return;
  }
  LibraryBrowserDialog dialog(LibraryType::Footprint, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const std::optional<std::string> selected = dialog.result();
  if (!selected.has_value()) {
    return;
  }
  try {
    const std::filesystem::path path(*selected);
    ccad::Footprint footprint = loadFootprintSelection(path);
    if (footprint.pads.empty()) {
      QMessageBox::warning(this, "Invalid Footprint", "The selected footprint has no pads.");
      return;
    }
    editor_tabs_->setCurrentWidget(canvas_view_);
    const std::string component_id =
        nextComponentId(project_cache_, placementPrefixFromName(path.stem().string()));
    enterPlaceFootprintMode(component_id, footprint, layer_id);
  } catch (const std::exception& e) {
    QMessageBox::critical(this, "Footprint Load Failed", QString::fromUtf8(e.what()));
  }
}

void ReviewWindow::chooseAndPlaceSymbol() {
  LibraryBrowserDialog dialog(LibraryType::Symbol, this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }
  const std::optional<std::string> selected = dialog.result();
  if (!selected.has_value()) {
    return;
  }
  try {
    const std::filesystem::path path(*selected);
    ccad::Symbol symbol = loadSymbolSelection(path);
    if (symbol.pins.empty()) {
      QMessageBox::warning(this, "Invalid Symbol", "The selected symbol has no pins.");
      return;
    }
    editor_tabs_->setCurrentWidget(schematic_view_);
    const std::string component_id =
        nextComponentId(project_cache_, placementPrefixFromName(path.stem().string()));
    enterPlaceSymbolMode(component_id, symbol, 0.0);
  } catch (const std::exception& e) {
    QMessageBox::critical(this, "Symbol Load Failed", QString::fromUtf8(e.what()));
  }
}

void ReviewWindow::loadProjectPath(const std::filesystem::path& path) {
  current_path_ = path;
  undo_stack_.clear();
  redo_stack_.clear();
  updateUndoRedoActions();
  reloadProject();
  if (project_cache_.board.has_value() && !project_cache_.board->pads.empty()) {
    const QString first_pad_id = QString::fromStdString(project_cache_.board->pads.front().id);
    selectCanvasObjectById(*canvas_scene_, first_pad_id);
  }
}

void ReviewWindow::applyStyle() {
  setStyleSheet(R"(
    QMainWindow {
      background: #0f172a;
      color: #172033;
      font-size: 10.5pt;
    }
    QMenuBar, QToolBar, QDockWidget {
      background: #f8fafc;
      color: #111827;
      border-bottom: 1px solid #cbd5e1;
      spacing: 8px;
    }
    QToolBar {
      padding: 6px;
    }
    QToolButton {
      padding: 6px 10px;
      border-radius: 6px;
    }
    QToolButton:hover {
      background: #e8eef7;
    }
    QLabel#title {
      color: #f8fafc;
      font-size: 16pt;
      font-weight: 700;
    }
    QLabel#subtitle {
      color: #94a3b8;
    }
    QLabel#statusChip {
      color: #ffffff;
      background: #64748b;
      border-radius: 13px;
      padding: 6px 12px;
      font-weight: 700;
    }
    QFrame#summaryCard {
      background: #ffffff;
      border: 1px solid #dfe6ef;
      border-radius: 6px;
    }
    QLabel#cardTitle {
      color: #64748b;
      font-size: 9pt;
      font-weight: 700;
      text-transform: uppercase;
    }
    QLabel#cardValue {
      color: #111827;
      font-size: 19pt;
      font-weight: 700;
    }
    QTableWidget#diagnosticsTable {
      background: #ffffff;
      alternate-background-color: #f8fafc;
      border: 1px solid #dfe6ef;
      border-radius: 8px;
      selection-background-color: #dbeafe;
      selection-color: #111827;
    }
    QListWidget#objectBrowserPanel {
      background: #ffffff;
      border: 1px solid #dfe6ef;
      padding: 6px;
    }
    QWidget#selectionInspectorPanel {
      background: #e0f2fe;
      border: 1px solid #38bdf8;
      border-radius: 6px;
      padding: 8px;
    }
    QLabel#inspectorTitle {
      color: #0f172a;
      font-weight: 700;
    }
    QLabel#inspectorDetail {
      color: #334155;
    }
    QLabel#inspectorValue {
      color: #0f172a;
      font-weight: 700;
    }
    QTabWidget::pane {
      border: 1px solid #1e293b;
      background: #07111f;
    }
    QTabBar::tab {
      background: #e2e8f0;
      color: #1e293b;
      padding: 8px 18px;
      border-top-left-radius: 6px;
      border-top-right-radius: 6px;
    }
    QTabBar::tab:selected {
      background: #ffffff;
      color: #0f172a;
      font-weight: 700;
    }
    QHeaderView::section {
      background: #eef2f7;
      color: #334155;
      border: none;
      border-bottom: 1px solid #d7dee8;
      padding: 8px;
      font-weight: 700;
    }
    QStatusBar {
      background: #111827;
      color: #dbeafe;
      border-top: 1px solid #334155;
    }
  )");
}

void ReviewWindow::openProject() {
  const QString selected = QFileDialog::getOpenFileName(
      this, "Open CCad project", QString(), "CCad projects (*.json *.ccad.json);;All files (*)");
  if (selected.isEmpty()) {
    return;
  }
  current_path_ = selected.toStdString();
  undo_stack_.clear();
  redo_stack_.clear();
  updateUndoRedoActions();
  reloadProject();
}

void ReviewWindow::reloadProject() {
  if (current_path_.empty()) {
    statusBar()->showMessage("No project file selected");
    return;
  }

  try {
    const ccad::Project project = ccad::loadProjectJson(readFile(current_path_));
    project_cache_ = project;
    renderReview(ccad::buildReview(project));
    setWindowTitle("CCad Review - " + QFileInfo(qstr(current_path_.string())).fileName());
  } catch (const std::exception& error) {
    diagnostics_->setRowCount(0);
    renderCanvas(ccad::CanvasScene{});
    project_summary_->renderLoadFailure(qstr(current_path_.string()));
    statusBar()->showMessage(qstr(error.what()));
    QMessageBox::warning(this, "Load failed", qstr(error.what()));
  }
}

void ReviewWindow::renderReview(const ccad::ProjectReview& review) {
  project_summary_->renderReview(review);
  diagnostics_->renderDiagnostics(review.diagnostics);
  
  const ccad::CanvasScene pcb_scene = ccad::buildCanvasScene(project_cache_);
  renderBoardCanvas(*canvas_scene_, pcb_scene);
  addDiagnosticMarkers(*canvas_scene_, review.diagnostics);
  object_browser_->renderScene(pcb_scene);
  
  const ccad::CanvasScene schematic_scene = ccad::buildSchematicScene(project_cache_);
  renderBoardCanvas(*schematic_scene_, schematic_scene);

  if (auto* board_view = dynamic_cast<BoardCanvasView*>(canvas_view_)) board_view->zoomToFit();
  if (auto* schem_view = dynamic_cast<BoardCanvasView*>(schematic_view_)) schem_view->zoomToFit();
  statusBar()->showMessage(qstr(review.status));
}

void ReviewWindow::renderCanvas(const ccad::CanvasScene& scene,
                                const std::vector<ccad::Diagnostic>& diagnostics) {
  renderBoardCanvas(*canvas_scene_, scene);
  addDiagnosticMarkers(*canvas_scene_, diagnostics);
  object_browser_->renderScene(scene);
  if (auto* board_view = dynamic_cast<BoardCanvasView*>(canvas_view_)) {
    board_view->zoomToFit();
  }
}

void ReviewWindow::updateCursorStatus(const QPointF& scene_position, const double zoom_factor) {
  cursor_status_->setText(formatCursorStatus(project_cache_.board, scene_position));
  zoom_status_->setText("Zoom " + QString::number(zoom_factor * 100.0, 'f', 0) + "%");
}

void ReviewWindow::updateSelectionStatus() {
  const QList<QGraphicsItem*> selected_items = canvas_scene_->selectedItems();
  if (selected_items.isEmpty()) {
    selection_status_->setText("Selected --");
    selection_inspector_->renderBoardRules(project_cache_.board);
    return;
  }
  const QGraphicsItem* item = selected_items.first();
  const QString type = canvasObjectType(*item);
  const QString id = canvasObjectId(*item);
  if (type.isEmpty() || id.isEmpty()) {
    selection_status_->setText("Selected canvas item");
    selection_inspector_->renderCanvasItem();
    return;
  }
  const QString text = "Selected " + type + " " + id;
  selection_status_->setText(text);
  selection_inspector_->renderSelection(project_cache_.board, type, id);
}

void ReviewWindow::previewFootprint() {
  const QString path = QFileDialog::getOpenFileName(this, "Select KiCad Footprint", "", "KiCad Footprint (*.kicad_mod)");
  if (path.isEmpty()) return;
  try {
    const std::string content = readFile(path.toStdString());
    const ccad::Footprint footprint = ccad::importKiCadFootprint(content);
    const ccad::CanvasScene scene = ccad::buildCanvasScene(footprint);
    renderCanvas(scene);
    statusBar()->showMessage("Previewing footprint: " + qstr(footprint.name));
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Import Failed", qstr(e.what()));
  }
}

void ReviewWindow::previewSymbol() {
  const QString path = QFileDialog::getOpenFileName(this, "Select KiCad Symbol", "", "KiCad Symbol (*.kicad_sym)");
  if (path.isEmpty()) return;
  try {
    const std::string content = readFile(path.toStdString());
    const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(content);
    if (symbols.empty()) {
      QMessageBox::warning(this, "Import Failed", "No symbols found in file.");
      return;
    }
    const ccad::CanvasScene scene = ccad::buildCanvasScene(symbols.front());
    renderCanvas(scene);
    statusBar()->showMessage("Previewing symbol: " + qstr(symbols.front().name));
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Import Failed", qstr(e.what()));
  }
}

void ReviewWindow::loadFootprintPreview(const std::filesystem::path& path) {
  try {
    const std::string content = readFile(path);
    const ccad::Footprint footprint = ccad::importKiCadFootprint(content);
    const ccad::CanvasScene scene = ccad::buildCanvasScene(footprint);
    renderCanvas(scene);
    statusBar()->showMessage("Previewing footprint: " + qstr(footprint.name));
  } catch (const std::exception& e) {
    std::cerr << "Import Failed: " << e.what() << "\n";
  }
}

void ReviewWindow::loadSymbolPreview(const std::filesystem::path& path) {
  try {
    const std::string content = readFile(path);
    const std::vector<ccad::Symbol> symbols = ccad::importKiCadSymbolLibrary(content);
    if (!symbols.empty()) {
      const ccad::CanvasScene scene = ccad::buildCanvasScene(symbols.front());
      renderCanvas(scene);
      statusBar()->showMessage("Previewing symbol: " + qstr(symbols.front().name));
    }
  } catch (const std::exception& e) {
    std::cerr << "Import Failed: " << e.what() << "\n";
  }
}

void ReviewWindow::exportDrcReport() {
  if (current_path_.empty()) {
    QMessageBox::warning(this, "Export DRC Report", "Load a project before exporting DRC.");
    return;
  }
  const QString selected = QFileDialog::getSaveFileName(
      this, "Export DRC Report", qstr(current_path_.stem().string() + ".drc.json"),
      "JSON Files (*.json);;All files (*)");
  if (selected.isEmpty()) {
    return;
  }
  const std::vector<ccad::Diagnostic> diagnostics = ccad::runDrc(project_cache_);
  std::ostringstream out;
  out << "{\n  \"diagnostics\": [\n";
  for (std::size_t i = 0; i < diagnostics.size(); ++i) {
    const ccad::Diagnostic& diagnostic = diagnostics.at(i);
    out << "    {\n";
    out << "      \"severity\": \"" << ccad::escapeJson(diagnostic.severity) << "\",\n";
    out << "      \"code\": \"" << ccad::escapeJson(diagnostic.code) << "\",\n";
    out << "      \"message\": \"" << ccad::escapeJson(diagnostic.message) << "\",\n";
    out << "      \"object_id\": \"" << ccad::escapeJson(diagnostic.object_id) << "\"\n";
    out << "    }" << (i + 1 == diagnostics.size() ? "" : ",") << "\n";
  }
  out << "  ]\n}\n";
  try {
    writeFile(selected.toStdString(), out.str());
    statusBar()->showMessage("Exported DRC report");
  } catch (const std::exception& e) {
    QMessageBox::critical(this, "Export failed", QString::fromStdString(e.what()));
  }
}

void ReviewWindow::showComponentWizard() {
  ComponentWizardDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted) {
    QString type = dialog.getComponentType();
    QString name = dialog.getComponentName();
    int pins = dialog.getPinCount();
    
    if (name.isEmpty()) {
      QMessageBox::warning(this, "Validation Error", "Component name cannot be empty.");
      return;
    }

    QString default_out = name + (type == "symbol" ? ".json" : ".json");
    QString filename = QFileDialog::getSaveFileName(
        this, "Save Component JSON", default_out, "JSON Files (*.json)");
        
    if (filename.isEmpty()) return;
    
    std::string json_data;
    if (type == "symbol") {
      ccad::SymbolParams params;
      params.name = name.toStdString();
      params.pin_count = pins;
      ccad::Symbol sym = ccad::generateParametricSymbol(params);
      json_data = ccad::dumpSymbolsJson({sym});
    } else {
      ccad::FootprintParams params;
      params.name = name.toStdString();
      params.pin_count = pins;
      params.package_type = dialog.getPackageType().toStdString();
      ccad::Footprint fp = ccad::generateParametricFootprint(params);
      json_data = ccad::dumpFootprintJson(fp);
    }
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream out(&file);
      out << QString::fromStdString(json_data);
      QMessageBox::information(this, "Success", "Component saved successfully.");
    } else {
      QMessageBox::critical(this, "Error", "Failed to save file.");
    }
  }
}

void ReviewWindow::cancelInteractionMode() {
  for (QGraphicsItem* item : interaction_ghost_items_) {
    if (item->scene() != nullptr) {
      item->scene()->removeItem(item);
    }
    delete item;
  }
  interaction_ghost_items_.clear();
  interaction_mode_ = InteractionMode::Default;
  interaction_component_id_.clear();
  interaction_layer_id_.clear();
  interaction_rotation_degrees_ = 0.0;
  canvas_view_->viewport()->unsetCursor();
  schematic_view_->viewport()->unsetCursor();
}

void ReviewWindow::enterPlaceFootprintMode(const std::string& component_id, const ccad::Footprint& footprint, const std::string& layer_id) {
  cancelInteractionMode();
  interaction_mode_ = InteractionMode::PlaceFootprint;
  interaction_component_id_ = component_id;
  interaction_footprint_ = footprint;
  interaction_layer_id_ = layer_id;

  try {
    addPadPreviewItems(*canvas_scene_, interaction_ghost_items_, footprint, QColor(100, 255, 100));
    interaction_last_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));
    moveGhostTo(*canvas_view_, interaction_ghost_items_, interaction_last_mouse_pos_);
    tool_status_->setText("Tool Place Footprint");
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Error", "Failed to load footprint for placement preview: " + QString(e.what()));
    cancelInteractionMode();
  }
}

void ReviewWindow::enterPlaceSymbolMode(const std::string& component_id, const ccad::Symbol& symbol, double rotation_degrees) {
  cancelInteractionMode();
  interaction_mode_ = InteractionMode::PlaceSymbol;
  interaction_component_id_ = component_id;
  interaction_symbol_ = symbol;
  interaction_rotation_degrees_ = rotation_degrees;

  try {
    addSymbolPreviewItems(*schematic_scene_, interaction_ghost_items_, symbol, QColor("#008484"));
    interaction_last_mouse_pos_ =
        schematic_view_->mapToScene(schematic_view_->mapFromGlobal(QCursor::pos()));
    moveGhostTo(*schematic_view_, interaction_ghost_items_, interaction_last_mouse_pos_);
    tool_status_->setText("Tool Place Symbol");
  } catch (const std::exception& e) {
    QMessageBox::warning(this, "Error", "Failed to load symbol for placement preview: " + QString(e.what()));
    cancelInteractionMode();
  }
}

void ReviewWindow::enterMoveFootprintMode(const std::string& component_id) {
  cancelInteractionMode();
  if (!project_cache_.board.has_value()) return;
  interaction_mode_ = InteractionMode::MoveFootprint;
  interaction_component_id_ = component_id;
  interaction_start_mouse_pos_ = canvas_view_->mapToScene(canvas_view_->mapFromGlobal(QCursor::pos()));
  interaction_last_mouse_pos_ = interaction_start_mouse_pos_;

  for (const auto& pad : project_cache_.board->pads) {
    if (pad.component_id != component_id) {
      continue;
    }
    const double w = pad.size.width.nanometers / 1e6;
    const double h = pad.size.height.nanometers / 1e6;
    const double x = pad.position.x.nanometers / 1e6;
    const double y = pad.position.y.nanometers / 1e6;
    const QPointF scene_center = boardPositionToScene(*project_cache_.board, x, y);
    auto* item = canvas_scene_->addPath(padPreviewPath(scene_center.x() / 10.0,
                                                       scene_center.y() / 10.0, w, h,
                                                       pad.shape, pad.rotation_degrees,
                                                       pad.roundrect_rratio, pad.chamfer_ratio),
                                        QPen(QColor(100, 180, 80), 1.0),
                                        QBrush(QColor(100, 255, 100, 150)));
    item->setZValue(1000);
    interaction_ghost_items_.push_back(item);
    if (pad.drill.has_value()) {
      constexpr double scale = 10.0;
      const double drill = pad.drill->nanometers / 1e6 * scale;
      auto* drill_item = canvas_scene_->addEllipse(scene_center.x() - (drill / 2.0),
                                                   scene_center.y() - (drill / 2.0), drill, drill,
                                                   QPen(Qt::NoPen), QBrush(QColor("#07111f")));
      drill_item->setZValue(1001);
      interaction_ghost_items_.push_back(drill_item);
    }
  }
}

bool ReviewWindow::eventFilter(QObject* obj, QEvent* event) {
  if (interaction_mode_ != InteractionMode::Default) {
    QGraphicsView* active_view = interaction_mode_ == InteractionMode::PlaceSymbol ? schematic_view_
                                                                                   : canvas_view_;
    if (obj == active_view->viewport()) {
      if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPointF scene_pos = active_view->mapToScene(me->pos());
        QPointF delta = scene_pos - interaction_last_mouse_pos_;

        for (QGraphicsItem* item : interaction_ghost_items_) {
          item->setPos(item->pos() + delta);
        }
        interaction_last_mouse_pos_ = scene_pos;
        return true; // Consume event
      } else if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
          // Finalize placement
          QPointF scene_pos = active_view->mapToScene(me->pos());

          if (interaction_mode_ == InteractionMode::PlaceFootprint) {
            try {
              if (!project_cache_.board.has_value()) {
                throw std::runtime_error("cannot place footprint without a board");
              }
              pushUndoSnapshot();
              ccad::placeFootprint(
                  project_cache_, interaction_footprint_, interaction_component_id_,
                  boardPointFromScene(*project_cache_.board, scene_pos),
                  0.0, interaction_layer_id_);
              // Save the project
              std::ofstream out(current_path_);
              if (out) {
                out << ccad::dumpProjectJson(project_cache_);
                reloadProject();
              } else {
                QMessageBox::critical(this, "Save Error", "Failed to write project file.");
              }
            } catch (const std::exception& e) {
              QMessageBox::critical(this, "Placement Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::PlaceSymbol) {
            try {
              pushUndoSnapshot();
              ccad::placeComponent(project_cache_, interaction_symbol_, interaction_component_id_,
                                   schematicPointFromScene(scene_pos),
                                   interaction_rotation_degrees_);
              std::ofstream out(current_path_);
              if (out) {
                out << ccad::dumpProjectJson(project_cache_);
                reloadProject();
              } else {
                QMessageBox::critical(this, "Save Error", "Failed to write project file.");
              }
            } catch (const std::exception& e) {
              QMessageBox::critical(this, "Placement Error", QString::fromUtf8(e.what()));
            }
          } else if (interaction_mode_ == InteractionMode::MoveFootprint) {
            try {
              QPointF delta = scene_pos - interaction_start_mouse_pos_;
              pushUndoSnapshot();
              ccad::moveFootprint(project_cache_, interaction_component_id_,
                                  boardDeltaFromSceneDelta(delta));
              std::ofstream out(current_path_);
              if (out) {
                out << ccad::dumpProjectJson(project_cache_);
                reloadProject();
              }
            } catch (const std::exception& e) {
              QMessageBox::critical(this, "Move Error", QString::fromUtf8(e.what()));
            }
          }
          cancelInteractionMode();
          return true; // Consume event
        }
      }
    }
  } else {
    // If not in a specific interaction mode, handle general hotkeys if applicable
    if (event->type() == QEvent::KeyPress) {
      auto* ke = static_cast<QKeyEvent*>(event);
      if (ke->key() == Qt::Key_M) {
        auto selected = canvas_scene_->selectedItems();
        if (!selected.empty() && project_cache_.board.has_value()) {
          QString object_id = selected.first()->data(0).toString();
          for (const auto& pad : project_cache_.board->pads) {
            if (pad.id == object_id.toStdString()) {
              enterMoveFootprintMode(pad.component_id);
              return true;
            }
          }
        }
      }
    }
  }

  if (interaction_mode_ != InteractionMode::Default && event->type() == QEvent::KeyPress) {
    auto* ke = static_cast<QKeyEvent*>(event);
    if (ke->key() == Qt::Key_Escape) {
      cancelInteractionMode();
      return true; // Consume event
    }
  }
  return QMainWindow::eventFilter(obj, event);
}
