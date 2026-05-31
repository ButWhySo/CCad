#include "review_window.hpp"
#include "board_canvas_view.hpp"

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
  } else if (argc == 4 && (std::string(argv[1]) == "--screenshot-footprint" || std::string(argv[1]) == "--screenshot-symbol")) {
    const std::string mode = argv[1];
    const std::filesystem::path target_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    
    ReviewWindow window;
    if (mode == "--screenshot-footprint") {
      window.loadFootprintPreview(target_path);
    } else {
      window.loadSymbolPreview(target_path);
    }
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
  } else if (argc == 4 && std::string(argv[1]) == "--screenshot-measure") {
    const std::filesystem::path project_path(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window]() {
      // Simulate switching to measure tool
      if (auto* tabs = window.findChild<QTabWidget*>("editorTabs")) {
        if (auto* view = dynamic_cast<BoardCanvasView*>(tabs->currentWidget())) {
          view->setToolMode(ToolMode::Measure);
          // Simulate drag from (10,10) to (50,50) in viewport
          QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
          QApplication::sendEvent(view->viewport(), &press);
          QMouseEvent move(QEvent::MouseMove, QPointF(400, 300), QPointF(400, 300), Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
          QApplication::sendEvent(view->viewport(), &move);
        }
      }
    });

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
