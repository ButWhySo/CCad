#include <QApplication>
#include <QTemporaryDir>
#include <QTimer>

#include <fstream>

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_gui/review_window.hpp"

namespace {

bool writeProject(const QString& path, const ccad::Project& project) {
  std::ofstream out(path.toStdString(), std::ios::binary);
  out << ccad::dumpProjectJson(project);
  return static_cast<bool>(out);
}

ccad::Project emptyProject() {
  ccad::Project project;
  project.id = "proj-gui-empty";
  project.name = "gui-empty";
  return project;
}

ccad::Project boardOnlyProject() {
  ccad::Project project;
  project.id = "proj-gui-board-only";
  project.name = "gui-board-only";
  ccad::Board board;
  board.outline.origin = ccad::Point{ccad::millimeters(0.0), ccad::millimeters(0.0)};
  board.outline.size = ccad::Size{ccad::millimeters(20.0), ccad::millimeters(10.0)};
  board.layers.push_back(ccad::Layer{.id = "F.Cu",
                                     .name = "Front copper",
                                     .kind = "copper",
                                     .visible = true});
  board.layers.push_back(ccad::Layer{.id = "B.Cu",
                                     .name = "Back copper",
                                     .kind = "copper",
                                     .visible = true});
  project.boards.push_back(board);
  return project;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QTemporaryDir temp_dir;
  if (!temp_dir.isValid()) {
    return 2;
  }

  const QString empty_path = temp_dir.filePath("empty.ccad.json");
  const QString board_path = temp_dir.filePath("board-only.ccad.json");
  if (!writeProject(empty_path, emptyProject()) ||
      !writeProject(board_path, boardOnlyProject())) {
    return 3;
  }

  ReviewWindow window;
  window.loadProjectPath(empty_path.toStdString());
  const QString empty_map = window.uiMapJson();
  if (!empty_map.contains("\"schema_version\"")) {
    return 4;
  }

  window.loadProjectPath(board_path.toStdString());
  const QString board_map = window.uiMapJson();
  if (!board_map.contains("\"schema_version\"")) {
    return 5;
  }

  window.show();
  QTimer::singleShot(7000, &app, &QCoreApplication::quit);
  return QApplication::exec();
}
