#include "ccad_core/canvas.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/serialize.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QResizeEvent>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

class BoardCanvasView final : public QGraphicsView {
 public:
  using QGraphicsView::QGraphicsView;

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QGraphicsView::resizeEvent(event);
    if (scene() != nullptr && !scene()->sceneRect().isEmpty()) {
      fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
  }
};

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

class ReviewWindow final : public QMainWindow {
 public:
  ReviewWindow() {
    setWindowTitle("CCad Review");
    resize(1120, 720);
    applyStyle();

    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(20, 18, 20, 16);
    layout->setSpacing(14);

    auto* header = new QFrame(root);
    header->setObjectName("header");
    auto* header_layout = new QHBoxLayout(header);
    header_layout->setContentsMargins(18, 14, 18, 14);
    header_layout->setSpacing(16);

    title_ = new QLabel("CCad Review", header);
    title_->setObjectName("title");
    subtitle_ = new QLabel("Open a project to inspect agent output and ERC state.", header);
    subtitle_->setObjectName("subtitle");
    auto* title_stack = new QVBoxLayout();
    title_stack->setSpacing(2);
    title_stack->addWidget(title_);
    title_stack->addWidget(subtitle_);

    status_chip_ = new QLabel("Ready", header);
    status_chip_->setObjectName("statusChip");
    status_chip_->setAlignment(Qt::AlignCenter);
    status_chip_->setMinimumWidth(120);

    header_layout->addLayout(title_stack, 1);
    header_layout->addWidget(status_chip_);
    layout->addWidget(header);

    auto* cards = new QFrame(root);
    auto* cards_layout = new QGridLayout(cards);
    cards_layout->setContentsMargins(0, 0, 0, 0);
    cards_layout->setHorizontalSpacing(12);
    cards_layout->setVerticalSpacing(12);
    cards_layout->addWidget(makeCard(cards, "Components", &components_value_), 0, 0);
    cards_layout->addWidget(makeCard(cards, "Nets", &nets_value_), 0, 1);
    cards_layout->addWidget(makeCard(cards, "Layers", &layers_value_), 0, 2);
    cards_layout->addWidget(makeCard(cards, "Diagnostics", &diagnostics_value_), 0, 3);
    layout->addWidget(cards);

    diagnostics_ = new QTableWidget(root);
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
    canvas_scene_ = new QGraphicsScene(this);
    canvas_view_ = new BoardCanvasView(canvas_scene_, root);
    canvas_view_->setObjectName("boardCanvas");
    canvas_view_->setRenderHint(QPainter::Antialiasing);
    canvas_view_->setDragMode(QGraphicsView::ScrollHandDrag);
    canvas_view_->setFrameShape(QFrame::NoFrame);
    canvas_view_->setMinimumHeight(360);

    auto* workspace = new QSplitter(Qt::Horizontal, root);
    workspace->addWidget(canvas_view_);
    workspace->addWidget(diagnostics_);
    workspace->setStretchFactor(0, 4);
    workspace->setStretchFactor(1, 3);
    layout->addWidget(workspace, 1);

    setCentralWidget(root);
    statusBar()->showMessage("Ready");

    auto* open_action = new QAction("Open", this);
    auto* reload_action = new QAction("Reload", this);
    auto* quit_action = new QAction("Quit", this);

    connect(open_action, &QAction::triggered, this, [this]() { openProject(); });
    connect(reload_action, &QAction::triggered, this, [this]() { reloadProject(); });
    connect(quit_action, &QAction::triggered, this, [this]() { close(); });

    auto* file_menu = menuBar()->addMenu("File");
    file_menu->addAction(open_action);
    file_menu->addAction(reload_action);
    file_menu->addSeparator();
    file_menu->addAction(quit_action);

    auto* toolbar = addToolBar("Main");
    toolbar->addAction(open_action);
    toolbar->addAction(reload_action);
  }

  void loadProjectPath(const std::filesystem::path& path) {
    current_path_ = path;
    reloadProject();
  }

 private:
  void applyStyle() {
    setStyleSheet(R"(
      QMainWindow {
        background: #f5f7fb;
        color: #172033;
        font-size: 10.5pt;
      }
      QMenuBar, QToolBar {
        background: #ffffff;
        border-bottom: 1px solid #dde3ec;
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
      QFrame#header {
        background: #182235;
        border-radius: 10px;
      }
      QLabel#title {
        color: #ffffff;
        font-size: 18pt;
        font-weight: 700;
      }
      QLabel#subtitle {
        color: #b7c4d8;
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
        border-radius: 9px;
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
      QHeaderView::section {
        background: #eef2f7;
        color: #334155;
        border: none;
        border-bottom: 1px solid #d7dee8;
        padding: 8px;
        font-weight: 700;
      }
      QStatusBar {
        background: #ffffff;
        color: #475569;
        border-top: 1px solid #dde3ec;
      }
    )");
  }

  void openProject() {
    const QString selected = QFileDialog::getOpenFileName(
        this, "Open CCad project", QString(), "CCad projects (*.json *.ccad.json);;All files (*)");
    if (selected.isEmpty()) {
      return;
    }
    current_path_ = selected.toStdString();
    reloadProject();
  }

  void reloadProject() {
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

  void renderReview(const ccad::ProjectReview& review) {
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

  void renderCanvas(const ccad::CanvasScene& scene) {
    canvas_scene_->clear();
    canvas_scene_->setBackgroundBrush(QBrush(QColor("#07111f")));
    if (!scene.has_board) {
      auto* text = canvas_scene_->addText("No board outline yet");
      text->setDefaultTextColor(QColor("#94a3b8"));
      text->setPos(18, 18);
      canvas_scene_->setSceneRect(0, 0, 420, 280);
      canvas_view_->fitInView(canvas_scene_->sceneRect(), Qt::KeepAspectRatio);
      return;
    }

    constexpr double margin = 18.0;
    constexpr double scale = 10.0;
    const double width = scene.view_width_units * scale;
    const double height = scene.view_height_units * scale;
    const QRectF board_rect(margin, margin, width, height);
    canvas_scene_->setSceneRect(0, 0, width + (2.0 * margin), height + 52.0);

    QPen grid_pen(QColor("#17243a"));
    grid_pen.setWidthF(0.25);
    for (double x = margin; x <= margin + width; x += 5.0 * scale) {
      canvas_scene_->addLine(x, margin, x, margin + height, grid_pen);
    }
    for (double y = margin; y <= margin + height; y += 5.0 * scale) {
      canvas_scene_->addLine(margin, y, margin + width, y, grid_pen);
    }

    QPen outline_pen(QColor("#38bdf8"));
    outline_pen.setWidthF(1.8);
    auto* board = canvas_scene_->addRect(board_rect, outline_pen, QBrush(QColor("#0f1b2d")));
    board->setToolTip("Board outline");

    auto* label = canvas_scene_->addText(QString::number(scene.view_width_units, 'f', 2) + " mm x " +
                                         QString::number(scene.view_height_units, 'f', 2) + " mm");
    label->setDefaultTextColor(QColor("#cbd5e1"));
    label->setScale(0.9);
    label->setPos(margin, margin + height + 10.0);

    canvas_view_->fitInView(canvas_scene_->sceneRect(), Qt::KeepAspectRatio);
  }

  void setStatusChip(const QString& text, const QString& color) {
    status_chip_->setText(text);
    status_chip_->setStyleSheet("color: #ffffff; background: " + color +
                                "; border-radius: 13px; padding: 6px 12px; font-weight: 700;");
  }

  QLabel* title_ = nullptr;
  QLabel* subtitle_ = nullptr;
  QLabel* status_chip_ = nullptr;
  QLabel* components_value_ = nullptr;
  QLabel* nets_value_ = nullptr;
  QLabel* layers_value_ = nullptr;
  QLabel* diagnostics_value_ = nullptr;
  QGraphicsScene* canvas_scene_ = nullptr;
  QGraphicsView* canvas_view_ = nullptr;
  QTableWidget* diagnostics_ = nullptr;
  std::filesystem::path current_path_;
  ccad::Project project_cache_;
};

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  ReviewWindow window;
  if (argc > 1) {
    window.loadProjectPath(argv[1]);
  }
  window.show();
  return QApplication::exec();
}

