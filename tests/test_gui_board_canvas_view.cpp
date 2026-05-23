#include "ccad_gui/board_canvas_view.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>

#include <cmath>
#include <vector>

namespace {

bool approxEqual(const double lhs, const double rhs) {
  return std::abs(lhs - rhs) < 0.0001;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  QGraphicsScene fit_scene;
  fit_scene.setSceneRect(-50000.0, -50000.0, 100000.0, 100000.0);
  fit_scene.addRect(0.0, 0.0, 400.0, 300.0);

  BoardCanvasView fit_view(&fit_scene);
  fit_view.resize(420, 300);
  fit_view.show();
  app.processEvents();
  fit_view.zoomToFit();
  require(fit_view.zoomFactor() > 0.4, "fit uses content bounds inside oversized scene rect");

  QGraphicsScene scene;
  scene.setSceneRect(-50000.0, -50000.0, 100000.0, 100000.0);
  scene.addRect(0.0, 0.0, 5000.0, 5000.0);

  BoardCanvasView view(&scene);
  std::vector<std::pair<bool, bool>> pan_states;
  view.setPanModeCallback([&pan_states](const bool space_mode, const bool dragging) {
    pan_states.emplace_back(space_mode, dragging);
  });
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
  const double fit_zoom = view.zoomFactor();
  require(fit_zoom < 1.0, "fit key applies fit zoom");

  QKeyEvent reset_key(QEvent::KeyPress, Qt::Key_0, Qt::NoModifier);
  QApplication::sendEvent(&view, &reset_key);
  require(approxEqual(view.zoomFactor(), 1.0), "zero key resets zoom");

  QKeyEvent home_key(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
  QApplication::sendEvent(&view, &home_key);
  require(approxEqual(view.zoomFactor(), fit_zoom), "home key applies fit zoom");

  view.resetZoom();
  app.processEvents();
  require(approxEqual(view.zoomFactor(), 1.0), "reset before pan key assertions");

  view.centerOn(2500.0, 2500.0);
  app.processEvents();
  const int h_mid =
      (view.horizontalScrollBar()->minimum() + view.horizontalScrollBar()->maximum()) / 2;
  const int v_mid = (view.verticalScrollBar()->minimum() + view.verticalScrollBar()->maximum()) / 2;
  view.horizontalScrollBar()->setValue(h_mid);
  view.verticalScrollBar()->setValue(v_mid);

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

  const int before_d = view.horizontalScrollBar()->value();
  QKeyEvent d_key(QEvent::KeyPress, Qt::Key_D, Qt::NoModifier);
  QApplication::sendEvent(&view, &d_key);
  require(view.horizontalScrollBar()->value() > before_d, "D key pans right");

  const int before_a = view.horizontalScrollBar()->value();
  QKeyEvent a_key(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier);
  QApplication::sendEvent(&view, &a_key);
  require(view.horizontalScrollBar()->value() < before_a, "A key pans left");

  const int before_s = view.verticalScrollBar()->value();
  QKeyEvent s_key(QEvent::KeyPress, Qt::Key_S, Qt::NoModifier);
  QApplication::sendEvent(&view, &s_key);
  require(view.verticalScrollBar()->value() > before_s, "S key pans down");

  const int before_w = view.verticalScrollBar()->value();
  QKeyEvent w_key(QEvent::KeyPress, Qt::Key_W, Qt::NoModifier);
  QApplication::sendEvent(&view, &w_key);
  require(view.verticalScrollBar()->value() < before_w, "W key pans up");

  const int before_space_pan_x = view.horizontalScrollBar()->value();
  const int before_space_pan_y = view.verticalScrollBar()->value();
  QKeyEvent space_press(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
  QApplication::sendEvent(&view, &space_press);
  require(!pan_states.empty(), "pan callback receives initial state");
  require(pan_states.back().first && !pan_states.back().second, "space sets pan-ready state");

  QMouseEvent left_pan_press(QEvent::MouseButtonPress, QPointF(180.0, 170.0), QPointF(180.0, 170.0),
                             Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(view.viewport(), &left_pan_press);
  require(pan_states.back().first && pan_states.back().second, "left press enters pan-drag state");
  QMouseEvent left_pan_move(QEvent::MouseMove, QPointF(120.0, 110.0), QPointF(120.0, 110.0),
                            Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(view.viewport(), &left_pan_move);
  QMouseEvent left_pan_release(QEvent::MouseButtonRelease, QPointF(120.0, 110.0),
                               QPointF(120.0, 110.0), Qt::LeftButton, Qt::NoButton,
                               Qt::NoModifier);
  QApplication::sendEvent(view.viewport(), &left_pan_release);
  require(pan_states.back().first && !pan_states.back().second,
          "left release returns to pan-ready state");

  QKeyEvent space_release(QEvent::KeyRelease, Qt::Key_Space, Qt::NoModifier);
  QApplication::sendEvent(&view, &space_release);
  require(!pan_states.back().first && !pan_states.back().second, "space release clears pan mode");

  require(view.horizontalScrollBar()->value() != before_space_pan_x ||
              view.verticalScrollBar()->value() != before_space_pan_y,
          "space plus left drag pans viewport");
}
