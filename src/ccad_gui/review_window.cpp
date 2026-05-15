#include "review_window.hpp"

#include "board_canvas_renderer.hpp"
#include "board_canvas_view.hpp"
#include "ccad_core/canvas.hpp"
#include "ccad_core/serialize.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QDockWidget>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <fstream>
#include <sstream>
#include <string>

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

QString qstr(const std::string& value) {
  return QString::fromStdString(value);
}

QFrame* makeCard(QWidget* parent, const QString& label, QLabel** value_label) {
  auto* card = new QFrame(parent);
  card->setObjectName("summaryCard");
  auto* layout = new QVBoxLayout(card);
  layout->setContentsMargins(14, 10, 14, 10);
  layout->setSpacing(4);

  auto* title = new QLabel(label, card);
  title->setObjectName("cardTitle");
  *value_label = new QLabel("0", card);
  (*value_label)->setObjectName("cardValue");

  layout->addWidget(title);
  layout->addWidget(*value_label);
  return card;
}

QTableWidgetItem* makeItem(const QString& text) {
  auto* item = new QTableWidgetItem(text);
  item->setFlags(item->flags() & ~Qt::ItemIsEditable);
  return item;
}

}  // namespace

ReviewWindow::ReviewWindow() {
  setWindowTitle("CCad PCB Editor");
  resize(1360, 860);
  applyStyle();

  auto* project_panel = new QWidget(this);
  auto* project_layout = new QVBoxLayout(project_panel);
  project_layout->setContentsMargins(14, 12, 14, 12);
  project_layout->setSpacing(10);

  title_ = new QLabel("No project loaded", project_panel);
  title_->setObjectName("title");
  subtitle_ = new QLabel("Open a .ccad.json project to review board state.", project_panel);
  subtitle_->setObjectName("subtitle");
  status_chip_ = new QLabel("Ready", project_panel);
  status_chip_->setObjectName("statusChip");
  status_chip_->setAlignment(Qt::AlignCenter);
  project_layout->addWidget(title_);
  project_layout->addWidget(subtitle_);
  project_layout->addWidget(status_chip_);
  project_layout->addWidget(makeCard(project_panel, "Components", &components_value_));
  project_layout->addWidget(makeCard(project_panel, "Nets", &nets_value_));
  project_layout->addWidget(makeCard(project_panel, "Layers", &layers_value_));
  project_layout->addWidget(makeCard(project_panel, "Diagnostics", &diagnostics_value_));
  project_layout->addStretch(1);

  auto* project_dock = new QDockWidget("Project", this);
  project_dock->setObjectName("projectDock");
  project_dock->setWidget(project_panel);
  addDockWidget(Qt::LeftDockWidgetArea, project_dock);

  diagnostics_ = new QTableWidget(this);
  diagnostics_->setObjectName("diagnosticsTable");
  diagnostics_->setColumnCount(4);
  diagnostics_->setHorizontalHeaderLabels({"Severity", "Code", "Object", "Message"});
  diagnostics_->horizontalHeader()->setStretchLastSection(true);
  diagnostics_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  diagnostics_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  diagnostics_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
  diagnostics_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  diagnostics_->setSelectionBehavior(QAbstractItemView::SelectRows);
  diagnostics_->setAlternatingRowColors(true);
  diagnostics_->verticalHeader()->setVisible(false);
  diagnostics_->setShowGrid(false);

  auto* diagnostics_dock = new QDockWidget("Diagnostics", this);
  diagnostics_dock->setObjectName("diagnosticsDock");
  diagnostics_dock->setWidget(diagnostics_);
  addDockWidget(Qt::BottomDockWidgetArea, diagnostics_dock);

  auto* layer_panel = new QListWidget(this);
  layer_panel->setObjectName("layersPanel");
  layer_panel->addItem("F.Cu");
  layer_panel->addItem("B.Cu");
  layer_panel->addItem("Edge.Cuts");
  layer_panel->addItem("Keepouts");
  auto* layers_dock = new QDockWidget("Layers / Objects", this);
  layers_dock->setObjectName("layersDock");
  layers_dock->setWidget(layer_panel);
  addDockWidget(Qt::RightDockWidgetArea, layers_dock);

  canvas_scene_ = new QGraphicsScene(this);
  auto* board_view = new BoardCanvasView(canvas_scene_, this);
  canvas_view_ = board_view;
  canvas_view_->setObjectName("boardCanvas");
  canvas_view_->setRenderHint(QPainter::Antialiasing);
  canvas_view_->setDragMode(QGraphicsView::NoDrag);
  canvas_view_->setFrameShape(QFrame::NoFrame);
  canvas_view_->setMouseTracking(true);
  board_view->setCoordinateCallback(
      [this](const QPointF& scene_position, const double zoom_factor) {
        updateCursorStatus(scene_position, zoom_factor);
      });

  auto* tabs = new QTabWidget(this);
  tabs->setObjectName("editorTabs");
  tabs->addTab(canvas_view_, "PCB");
  auto* schematic_placeholder = new QLabel("Schematic editor will share this shell.", tabs);
  schematic_placeholder->setAlignment(Qt::AlignCenter);
  tabs->addTab(schematic_placeholder, "Schematic");
  tabs->setTabEnabled(1, false);

  setCentralWidget(tabs);
  cursor_status_ = new QLabel("X --  Y --", this);
  zoom_status_ = new QLabel("Zoom 100%", this);
  tool_status_ = new QLabel("Tool Select", this);
  layer_status_ = new QLabel("Layer F.Cu", this);
  statusBar()->addPermanentWidget(cursor_status_);
  statusBar()->addPermanentWidget(zoom_status_);
  statusBar()->addPermanentWidget(tool_status_);
  statusBar()->addPermanentWidget(layer_status_);
  statusBar()->showMessage("Ready");

  auto* open_action = new QAction("Open", this);
  auto* reload_action = new QAction("Reload", this);
  auto* fit_action = new QAction("Fit", this);
  auto* quit_action = new QAction("Quit", this);

  connect(open_action, &QAction::triggered, this, [this]() { openProject(); });
  connect(reload_action, &QAction::triggered, this, [this]() { reloadProject(); });
  connect(fit_action, &QAction::triggered, this, [board_view]() { board_view->zoomToFit(); });
  connect(quit_action, &QAction::triggered, this, [this]() { close(); });

  auto* file_menu = menuBar()->addMenu("File");
  file_menu->addAction(open_action);
  file_menu->addAction(reload_action);
  file_menu->addSeparator();
  file_menu->addAction(quit_action);

  auto* toolbar = addToolBar("Main");
  toolbar->addAction(open_action);
  toolbar->addAction(reload_action);
  toolbar->addSeparator();
  toolbar->addAction(fit_action);
}

void ReviewWindow::loadProjectPath(const std::filesystem::path& path) {
  current_path_ = path;
  reloadProject();
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
    QListWidget#layersPanel {
      background: #ffffff;
      border: 1px solid #dfe6ef;
      padding: 6px;
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
    title_->setText("Load failed");
    subtitle_->setText(qstr(current_path_.string()));
    components_value_->setText("0");
    nets_value_->setText("0");
    layers_value_->setText("0");
    diagnostics_value_->setText("0");
    setStatusChip("Load failed", "#dc2626");
    statusBar()->showMessage(qstr(error.what()));
    QMessageBox::warning(this, "Load failed", qstr(error.what()));
  }
}

void ReviewWindow::renderReview(const ccad::ProjectReview& review) {
  title_->setText(qstr(review.project_name));
  QString board_text = "No board";
  if (review.has_board) {
    board_text = "Board: " + QString::number(review.board_width_nm / 1000000.0, 'f', 2) +
                 " mm x " + QString::number(review.board_height_nm / 1000000.0, 'f', 2) + " mm";
  }
  subtitle_->setText("Project ID: " + qstr(review.project_id) + "   " + board_text);
  components_value_->setText(QString::number(review.component_count));
  nets_value_->setText(QString::number(review.net_count));
  layers_value_->setText(QString::number(review.layer_count));
  diagnostics_value_->setText(QString::number(review.diagnostics.size()));

  if (review.error_count > 0) {
    setStatusChip("Errors", "#dc2626");
  } else if (review.warning_count > 0) {
    setStatusChip("Warnings", "#d97706");
  } else {
    setStatusChip("Clean", "#059669");
  }

  diagnostics_->setRowCount(static_cast<int>(review.diagnostics.size()));
  for (int row = 0; row < static_cast<int>(review.diagnostics.size()); ++row) {
    const ccad::Diagnostic& diagnostic = review.diagnostics.at(static_cast<std::size_t>(row));
    auto* severity = makeItem(qstr(diagnostic.severity));
    if (diagnostic.severity == "error") {
      severity->setBackground(QColor("#fee2e2"));
      severity->setForeground(QColor("#991b1b"));
    } else if (diagnostic.severity == "warning") {
      severity->setBackground(QColor("#fef3c7"));
      severity->setForeground(QColor("#92400e"));
    }
    diagnostics_->setItem(row, 0, severity);
    diagnostics_->setItem(row, 1, makeItem(qstr(diagnostic.code)));
    diagnostics_->setItem(row, 2, makeItem(qstr(diagnostic.object_id)));
    diagnostics_->setItem(row, 3, makeItem(qstr(diagnostic.message)));
  }
  diagnostics_->resizeColumnsToContents();
  renderCanvas(ccad::buildCanvasScene(project_cache_));
  statusBar()->showMessage(qstr(review.status));
}

void ReviewWindow::renderCanvas(const ccad::CanvasScene& scene) {
  renderBoardCanvas(*canvas_scene_, scene);
  auto* board_view = dynamic_cast<BoardCanvasView*>(canvas_view_);
  if (board_view != nullptr) {
    board_view->zoomToFit();
  }
}

void ReviewWindow::updateCursorStatus(const QPointF& scene_position, const double zoom_factor) {
  constexpr double margin = 18.0;
  constexpr double scale = 10.0;
  const double x_mm = (scene_position.x() - margin) / scale;
  const double y_mm = (scene_position.y() - margin) / scale;
  cursor_status_->setText("X " + QString::number(x_mm, 'f', 2) + " mm  Y " +
                          QString::number(y_mm, 'f', 2) + " mm");
  zoom_status_->setText("Zoom " + QString::number(zoom_factor * 100.0, 'f', 0) + "%");
}

void ReviewWindow::setStatusChip(const QString& text, const QString& color) {
  status_chip_->setText(text);
  status_chip_->setStyleSheet("color: #ffffff; background: " + color +
                              "; border-radius: 13px; padding: 6px 12px; font-weight: 700;");
}
