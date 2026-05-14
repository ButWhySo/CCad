#include "review_window.hpp"

#include <QApplication>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  ReviewWindow window;
  if (argc > 1) {
    window.loadProjectPath(argv[1]);
  }
  window.show();
  return QApplication::exec();
}
