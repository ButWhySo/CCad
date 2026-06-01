#include "review_window.hpp"
#include "board_canvas_view.hpp"
#include "library_browser_dialog.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>

#include <QEventLoop>
#include <filesystem>
#include <fstream>
#include <iostream>

int screenshotWindow(QWidget& window, const QString& screenshot_path, const char* screenshot_arg) {
  const QPixmap screenshot = window.grab();
  if (!screenshot.save(screenshot_path)) {
    std::cerr << "failed to save GUI screenshot: " << screenshot_arg << '\n';
    std::cerr.flush();
    return 2;
  }
  std::cout << "screenshot saved: " << screenshot_arg << '\n';
  std::cout.flush();
  return 0;
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  if (argc == 4 && (std::string(argv[1]) == "--dump-ui-map" ||
                    std::string(argv[1]) == "--validate-ui-map-targets")) {
    const bool validate_targets = std::string(argv[1]) == "--validate-ui-map-targets";
    const std::filesystem::path project_path(argv[2]);
    const std::filesystem::path output_path(argv[3]);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, output_path, validate_targets]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI map output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QString map = validate_targets ? window.validateUiMapTargetsJson(true)
                                           : window.uiMapJson();
      const QByteArray bytes = map.toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI map output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << (validate_targets ? "ui map target validation saved: " : "ui map saved: ")
                << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--ui-target-id") {
    const std::filesystem::path project_path(argv[2]);
    const QString target_id = QString::fromLocal8Bit(argv[3]);
    const std::filesystem::path output_path(argv[4]);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, target_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.uiTargetJsonById(target_id).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "ui target saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 5 && std::string(argv[1]) == "--ui-trigger-safe") {
    const std::filesystem::path project_path(argv[2]);
    const QString action_id = QString::fromLocal8Bit(argv[3]);
    const std::filesystem::path output_path(argv[4]);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, action_id, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI action output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.triggerSafeUiActionJson(action_id).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI action output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "ui action result saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 6 && std::string(argv[1]) == "--ui-target-board-point") {
    const std::filesystem::path project_path(argv[2]);
    const double x_mm = std::stod(argv[3]);
    const double y_mm = std::stod(argv[4]);
    const std::filesystem::path output_path(argv[5]);
    ReviewWindow window;
    window.loadProjectPath(project_path);
    window.show();

    QTimer::singleShot(500, &window, [&window, x_mm, y_mm, output_path]() {
      std::ofstream output(output_path, std::ios::binary);
      if (!output) {
        std::cerr << "failed to open UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      const QByteArray bytes = window.uiTargetJsonForBoardPoint(x_mm, y_mm).toUtf8();
      output.write(bytes.constData(), bytes.size());
      if (!output) {
        std::cerr << "failed to write UI target output: " << output_path.string() << '\n';
        std::cerr.flush();
        QCoreApplication::exit(2);
        return;
      }
      std::cout << "ui target saved: " << output_path.string() << '\n';
      std::cout.flush();
      QCoreApplication::exit(0);
    });

    return QApplication::exec();
  } else if (argc == 4 && std::string(argv[1]) == "--screenshot") {
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
  } else if (argc == 4 && (std::string(argv[1]) == "--screenshot-chooser-footprint" ||
                           std::string(argv[1]) == "--screenshot-chooser-symbol")) {
    const LibraryType type = std::string(argv[1]) == "--screenshot-chooser-footprint"
                                 ? LibraryType::Footprint
                                 : LibraryType::Symbol;
    const QString cache_root = QString::fromLocal8Bit(argv[2]);
    const char* screenshot_arg = argv[3];
    const QString screenshot_path = QString::fromLocal8Bit(screenshot_arg);
    auto* dialog = new LibraryBrowserDialog(type, cache_root);
    dialog->show();
    QTimer::singleShot(250, dialog, [dialog]() {
      if (!dialog->selectFirstVisibleItemForTest()) {
        std::cerr << "chooser had no visible items to select\n";
        std::cerr.flush();
        QCoreApplication::exit(3);
      }
    });
    QTimer::singleShot(20000, dialog, [dialog, screenshot_path, screenshot_arg]() {
      QCoreApplication::exit(screenshotWindow(*dialog, screenshot_path, screenshot_arg));
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
