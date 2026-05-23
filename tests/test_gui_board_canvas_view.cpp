#include "ccad_gui/board_canvas_view.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>

#include <cmath>

namespace {

bool approxEqual(const double lhs, const double rhs) {
  return std::abs(lhs - rhs) < 0.0001;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QGraphicsScene scene;
  scene.setSceneRect(0.0, 0.0, 5000.0, 5000.0);
  scene.addRect(0.0, 0.0, 5000.0, 5000.0);

  BoardCanvasView view(&scene);
  view.resize(420, 300);
  view.show();
  app.processEvents();

  view.resetZoom();
  require(approxEqual(view.zoomFactor(), 1.0), "reset starts at 100 percent");

  for (int i = 0; i < 120; ++i) {
    view.zoomIn();
  }
  require(view.zoomFactor() <= BoardCanvasView::kMaxZoomFactor + 0.0001,
          "zoom in is clamped to max factor");
  require(view.zoomFactor() >= BoardCanvasView::kMaxZoomFactor - 0.001,
          "zoom in reaches max factor");

  for (int i = 0; i < 240; ++i) {
    view.zoomOut();
  }
  require(view.zoomFactor() >= BoardCanvasView::kMinZoomFactor - 0.0001,
          "zoom out is clamped to min factor");
  require(view.zoomFactor() <= BoardCanvasView::kMinZoomFactor + 0.001,
          "zoom out reaches min factor");

  view.resetZoom();
  require(approxEqual(view.zoomFactor(), 1.0), "reset returns to 100 percent");

  view.setFocus();
  app.processEvents();

  const double zoom_before_keys = view.zoomFactor();
  QKeyEvent plus_key(QEvent::KeyPress, Qt::Key_Plus, Qt::NoModifier);
  QApplication::sendEvent(&view, &plus_key);
  require(view.zoomFactor() > zoom_before_keys, "plus key zooms in");

  QKeyEvent minus_key(QEvent::KeyPress, Qt::Key_Minus, Qt::NoModifier);
  QApplication::sendEvent(&view, &minus_key);
  require(approxEqual(view.zoomFactor(), zoom_before_keys), "minus key zooms out");

  QKeyEvent fit_key(QEvent::KeyPress, Qt::Key_F, Qt::NoModifier);
  QApplication::sendEvent(&view, &fit_key);
  require(view.zoomFactor() > BoardCanvasView::kMinZoomFactor, "fit key applies fit zoom");

  QKeyEvent reset_key(QEvent::KeyPress, Qt::Key_0, Qt::NoModifier);
  QApplication::sendEvent(&view, &reset_key);
  require(approxEqual(view.zoomFactor(), 1.0), "zero key resets zoom");

  view.centerOn(2500.0, 2500.0);
  app.processEvents();
  const int before_x = view.horizontalScrollBar()->value();
  const int before_y = view.verticalScrollBar()->value();

  QMouseEvent press(QEvent::MouseButtonPress, QPointF(120.0, 120.0), QPointF(120.0, 120.0),
                    Qt::RightButton, Qt::RightButton, Qt::NoModifier);
  QApplication::sendEvent(view.viewport(), &press);

  QMouseEvent move(QEvent::MouseMove, QPointF(220.0, 180.0), QPointF(220.0, 180.0),
                   Qt::NoButton, Qt::RightButton, Qt::NoModifier);
  QApplication::sendEvent(view.viewport(), &move);

  QMouseEvent release(QEvent::MouseButtonRelease, QPointF(220.0, 180.0), QPointF(220.0, 180.0),
                      Qt::RightButton, Qt::NoButton, Qt::NoModifier);
  QApplication::sendEvent(view.viewport(), &release);

  const int after_x = view.horizontalScrollBar()->value();
  const int after_y = view.verticalScrollBar()->value();
  require(before_x != after_x || before_y != after_y, "right-drag pan changes scroll position");

  const int before_right = view.horizontalScrollBar()->value();
  QKeyEvent right_key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
  QApplication::sendEvent(&view, &right_key);
  require(view.horizontalScrollBar()->value() > before_right, "right key pans right");

  const int before_left = view.horizontalScrollBar()->value();
  QKeyEvent left_key(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
  QApplication::sendEvent(&view, &left_key);
  require(view.horizontalScrollBar()->value() < before_left, "left key pans left");

  const int before_down = view.verticalScrollBar()->value();
  QKeyEvent down_key(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
  QApplication::sendEvent(&view, &down_key);
  require(view.verticalScrollBar()->value() > before_down, "down key pans down");

  const int before_up = view.verticalScrollBar()->value();
  QKeyEvent up_key(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
  QApplication::sendEvent(&view, &up_key);
  require(view.verticalScrollBar()->value() < before_up, "up key pans up");
}
