#include "review_window.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>

#include <QEventLoop>
#include <filesystem>
#include <iostream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

[[noreturn]] void exitScreenshotMode(const int code) {
#ifdef _WIN32
  TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));
#else
  std::_Exit(code);
#endif
  std::_Exit(code);
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  if (argc == 4 && std::string(argv[1]) == "--screenshot") {
    const std::filesystem::path project_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 20000) {
      app.processEvents(QEventLoop::AllEvents, 100);
    }

    const QPixmap screenshot = window.grab();
    if (!screenshot.save(screenshot_path)) {
      std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
      std::cerr.flush();
      exitScreenshotMode(2);
    }
    std::cout << "screenshot saved: " << screenshot_arg << '\n';
    std::cout.flush();
    exitScreenshotMode(0);
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
