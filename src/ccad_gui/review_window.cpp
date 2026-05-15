#include "review_window.hpp"

#include "board_canvas_renderer.hpp"
#include "board_canvas_view.hpp"
#include "ccad_core/canvas.hpp"
#include "ccad_core/serialize.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QDockWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QStatusBar>
#include <QTabWidget>
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

}  // namespace

ReviewWindow::ReviewWindow() {
  setWindowTitle("CCad PCB Editor");
  resize(1360, 860);
  applyStyle();

  project_summary_ = new ProjectSummaryPanel(this);

  auto* project_dock = new QDockWidget("Project", this);
  project_dock->setObjectName("projectDock");
  project_dock->setWidget(project_summary_);
  addDockWidget(Qt::LeftDockWidgetArea, project_dock);

  diagnostics_ = new DiagnosticsPanel(this);
  transaction_timeline_ = new TransactionTimelinePanel(this);

  auto* diagnostics_dock = new QDockWidget("Diagnostics", this);
  diagnostics_dock->setObjectName("diagnosticsDock");
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
  layers_dock->setWidget(right_panel);
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
  selection_status_ = new QLabel("Selected --", this);
  statusBar()->addPermanentWidget(cursor_status_);
  statusBar()->addPermanentWidget(zoom_status_);
  statusBar()->addPermanentWidget(selection_status_);
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

  connect(canvas_scene_, &QGraphicsScene::selectionChanged, this,
          [this]() { updateSelectionStatus(); });
  connect(diagnostics_, &QTableWidget::cellClicked, this, [this](const int row, int) {
    selectCanvasObjectById(*canvas_scene_, diagnostics_->objectIdForRow(row));
  });
  object_browser_->setObjectActivatedCallback(
      [this](const QString& object_id) { selectCanvasObjectById(*canvas_scene_, object_id); });
  object_browser_->setNetActivatedCallback(
      [this](const QString& net_id) { selectCanvasObjectsByNetId(*canvas_scene_, net_id); });
  transaction_timeline_->renderTransactions({});
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
  renderCanvas(ccad::buildCanvasScene(project_cache_), review.diagnostics);
  statusBar()->showMessage(qstr(review.status));
}

void ReviewWindow::renderCanvas(const ccad::CanvasScene& scene,
                                const std::vector<ccad::Diagnostic>& diagnostics) {
  renderBoardCanvas(*canvas_scene_, scene);
  addDiagnosticMarkers(*canvas_scene_, diagnostics);
  object_browser_->renderScene(scene);
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

void ReviewWindow::updateSelectionStatus() {
  const QList<QGraphicsItem*> selected_items = canvas_scene_->selectedItems();
  if (selected_items.isEmpty()) {
    selection_status_->setText("Selected --");
    selection_inspector_->clearSelection();
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
  selection_inspector_->renderSelection(type, id);
}

