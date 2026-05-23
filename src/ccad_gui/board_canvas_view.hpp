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
    notifyViewportChanged();
  }

  double zoomFactor() const { return zoom_factor_; }
  void setCoordinateCallback(std::function<void(QPointF, double)> callback) {
    coordinate_callback_ = std::move(callback);
  }
  void zoomIn() { zoomBy(1.18); }
  void zoomOut() { zoomBy(1.0 / 1.18); }
  void resetZoom() {
    resetTransform();
    zoom_factor_ = 1.0;
    user_view_ = true;
    notifyViewportChanged();
  }

 protected:
  void wheelEvent(QWheelEvent* event) override {
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    zoomBy(event->angleDelta().y() > 0 ? 1.18 : 1.0 / 1.18);
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
    notifyViewportChanged(event->pos());
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
  void zoomBy(const double factor) {
    scale(factor, factor);
    zoom_factor_ = transform().m11();
    user_view_ = true;
    notifyViewportChanged();
  }

  void notifyViewportChanged() {
    notifyViewportChanged(viewport()->rect().center());
  }

  void notifyViewportChanged(const QPoint& viewport_position) {
    if (coordinate_callback_) {
      coordinate_callback_(mapToScene(viewport_position), zoom_factor_);
    }
  }

  double zoom_factor_ = 1.0;
  bool user_view_ = false;
  std::function<void(QPointF, double)> coordinate_callback_;
};
