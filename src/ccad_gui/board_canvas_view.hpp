#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWheelEvent>

#include <functional>

class BoardCanvasView final : public QGraphicsView {
 public:
  using QGraphicsView::QGraphicsView;

  void zoomToFit() {
    if (scene() == nullptr || scene()->sceneRect().isEmpty()) {
      return;
    }
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    zoom_factor_ = transform().m11();
    user_view_ = false;
  }

  double zoomFactor() const { return zoom_factor_; }
  void setCoordinateCallback(std::function<void(QPointF, double)> callback) {
    coordinate_callback_ = std::move(callback);
  }

 protected:
  void wheelEvent(QWheelEvent* event) override {
    constexpr double zoom_in = 1.18;
    constexpr double zoom_out = 1.0 / zoom_in;
    const double factor = event->angleDelta().y() > 0 ? zoom_in : zoom_out;
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    scale(factor, factor);
    zoom_factor_ = transform().m11();
    user_view_ = true;
    event->accept();
  }

  void resizeEvent(QResizeEvent* event) override {
    QGraphicsView::resizeEvent(event);
    if (!user_view_) {
      zoomToFit();
    }
  }

  void mouseMoveEvent(QMouseEvent* event) override {
    QGraphicsView::mouseMoveEvent(event);
    if (coordinate_callback_) {
      coordinate_callback_(mapToScene(event->pos()), zoom_factor_);
    }
  }

  void mousePressEvent(QMouseEvent* event) override {
    if (event->button() == Qt::MiddleButton) {
      setDragMode(QGraphicsView::ScrollHandDrag);
      auto left_event = QMouseEvent(event->type(), event->position(), event->scenePosition(),
                                    event->globalPosition(), Qt::LeftButton, Qt::LeftButton,
                                    event->modifiers());
      QGraphicsView::mousePressEvent(&left_event);
      user_view_ = true;
      return;
    }
    QGraphicsView::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent* event) override {
    if (event->button() == Qt::MiddleButton) {
      auto left_event = QMouseEvent(event->type(), event->position(), event->scenePosition(),
                                    event->globalPosition(), Qt::LeftButton, Qt::NoButton,
                                    event->modifiers());
      QGraphicsView::mouseReleaseEvent(&left_event);
      setDragMode(QGraphicsView::NoDrag);
      return;
    }
    QGraphicsView::mouseReleaseEvent(event);
  }

 private:
  double zoom_factor_ = 1.0;
  bool user_view_ = false;
  std::function<void(QPointF, double)> coordinate_callback_;
};
