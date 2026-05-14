#include "ccad_core/review.hpp"
#include "ccad_core/serialize.hpp"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <filesystem>
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

class ReviewWindow final : public QMainWindow {
 public:
  ReviewWindow() {
    setWindowTitle("CCad Review");
    resize(920, 620);

    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);

    summary_ = new QLabel("Open a .ccad.json project to review.", root);
    summary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(summary_);

    diagnostics_ = new QTableWidget(root);
    diagnostics_->setColumnCount(4);
    diagnostics_->setHorizontalHeaderLabels({"Severity", "Code", "Object", "Message"});
    diagnostics_->horizontalHeader()->setStretchLastSection(true);
    diagnostics_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    diagnostics_->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(diagnostics_);

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

 private:
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
      renderReview(ccad::buildReview(project));
      setWindowTitle("CCad Review - " + QFileInfo(qstr(current_path_)).fileName());
    } catch (const std::exception& error) {
      diagnostics_->setRowCount(0);
      summary_->setText("Load failed");
      statusBar()->showMessage(qstr(error.what()));
      QMessageBox::warning(this, "Load failed", qstr(error.what()));
    }
  }

  void renderReview(const ccad::ProjectReview& review) {
    summary_->setText(
        "Project: " + qstr(review.project_name) + " (" + qstr(review.project_id) + ")\n" +
        "Components: " + QString::number(review.component_count) + "   Nets: " +
        QString::number(review.net_count) + "   Constraints: " +
        QString::number(review.constraint_count) + "\n" + "Status: " + qstr(review.status));

    diagnostics_->setRowCount(static_cast<int>(review.diagnostics.size()));
    for (int row = 0; row < static_cast<int>(review.diagnostics.size()); ++row) {
      const ccad::Diagnostic& diagnostic = review.diagnostics.at(static_cast<std::size_t>(row));
      diagnostics_->setItem(row, 0, new QTableWidgetItem(qstr(diagnostic.severity)));
      diagnostics_->setItem(row, 1, new QTableWidgetItem(qstr(diagnostic.code)));
      diagnostics_->setItem(row, 2, new QTableWidgetItem(qstr(diagnostic.object_id)));
      diagnostics_->setItem(row, 3, new QTableWidgetItem(qstr(diagnostic.message)));
    }
    diagnostics_->resizeColumnsToContents();
    statusBar()->showMessage(qstr(review.status));
  }

  QLabel* summary_ = nullptr;
  QTableWidget* diagnostics_ = nullptr;
  std::filesystem::path current_path_;
};

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  ReviewWindow window;
  window.show();
  return QApplication::exec();
}

