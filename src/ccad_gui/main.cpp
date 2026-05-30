#include "review_window.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>

#include <QEventLoop>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  if (argc == 4 && std::string(argv[1]) == "--screenshot") {
    const std::filesystem::path project_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(2000, &window, [&window, screenshot_path, screenshot_arg]() {
      const QPixmap screenshot = window.grab();
      if (!screenshot.save(screenshot_path)) {
        std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
      } else {
        std::cout << "screenshot saved: " << screenshot_arg << '\n';
        std::cout.flush();
        QCoreApplication::exit(0);
      }
    });

    return QApplication::exec();
  } else {
    ReviewWindow window;
    window.show();
    if (argc > 1) {
      const std::filesystem::path project_path(argv[1]);
      QTimer::singleShot(0, &window, [&window, project_path]() { window.loadProjectPath(project_path); });
    }
    const int exit_code = QApplication::exec();
    if (std::cout.good()) {
      std::cout.flush();
    }
    return exit_code;
  }
}
