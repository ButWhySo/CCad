#include "ccad_gui/board_canvas_view.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
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
}
