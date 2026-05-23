#pragma once

#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <functional>

class BoardCanvasView final : public QGraphicsView {
 public:
  using QGraphicsView::QGraphicsView;
  static constexpr double kMinZoomFactor = 0.05;
  static constexpr double kMaxZoomFactor = 40.0;

  void zoomToFit() {
    if (scene() == nullptr || scene()->sceneRect().isEmpty()) {
      return;
    }
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    zoom_factor_ = std::clamp(transform().m11(), kMinZoomFactor, kMaxZoomFactor);
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
    if (panning_) {
      const QPoint delta = event->pos() - pan_last_pos_;
      if (horizontalScrollBar() != nullptr) {
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
      }
      if (verticalScrollBar() != nullptr) {
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
      }
      pan_last_pos_ = event->pos();
      user_view_ = true;
      notifyViewportChanged(event->pos());
      event->accept();
      return;
    }
    QGraphicsView::mouseMoveEvent(event);
    notifyViewportChanged(event->pos());
  }

  void mousePressEvent(QMouseEvent* event) override {
    const bool pan_gesture = event->button() == Qt::MiddleButton ||
                             event->button() == Qt::RightButton ||
                             (event->button() == Qt::LeftButton &&
                              (event->modifiers() & Qt::KeyboardModifier::ShiftModifier));
    if (pan_gesture) {
      panning_ = true;
      pan_last_pos_ = event->pos();
      viewport()->setCursor(Qt::ClosedHandCursor);
      user_view_ = true;
      event->accept();
      return;
    }
    QGraphicsView::mousePressEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent* event) override {
    if (panning_ && (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton ||
                     event->button() == Qt::LeftButton)) {
      panning_ = false;
      viewport()->unsetCursor();
      notifyViewportChanged(event->pos());
      event->accept();
      return;
    }
    QGraphicsView::mouseReleaseEvent(event);
  }

  void keyPressEvent(QKeyEvent* event) override {
    constexpr int pan_step_pixels = 48;
    switch (event->key()) {
      case Qt::Key::Key_Plus:
      case Qt::Key::Key_Equal:
        zoomIn();
        event->accept();
        return;
      case Qt::Key::Key_Minus:
      case Qt::Key::Key_Underscore:
        zoomOut();
        event->accept();
        return;
      case Qt::Key::Key_0:
        resetZoom();
        event->accept();
        return;
      case Qt::Key::Key_F:
        zoomToFit();
        event->accept();
        return;
      case Qt::Key::Key_Left:
        panByPixels(-pan_step_pixels, 0);
        event->accept();
        return;
      case Qt::Key::Key_Right:
        panByPixels(pan_step_pixels, 0);
        event->accept();
        return;
      case Qt::Key::Key_Up:
        panByPixels(0, -pan_step_pixels);
        event->accept();
        return;
      case Qt::Key::Key_Down:
        panByPixels(0, pan_step_pixels);
        event->accept();
        return;
      default:
        break;
    }
    QGraphicsView::keyPressEvent(event);
  }

 private:
  void panByPixels(const int dx, const int dy) {
    if (horizontalScrollBar() != nullptr) {
      horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
    }
    if (verticalScrollBar() != nullptr) {
      verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
    }
    user_view_ = true;
    notifyViewportChanged();
  }

  void zoomBy(const double factor) {
    const double target = std::clamp(zoom_factor_ * factor, kMinZoomFactor, kMaxZoomFactor);
    if (zoom_factor_ <= 0.0) {
      zoom_factor_ = transform().m11();
    }
    const double relative = target / zoom_factor_;
    if (std::abs(relative - 1.0) < 0.0001) {
      return;
    }
    scale(relative, relative);
    zoom_factor_ = target;
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
  bool panning_ = false;
  QPoint pan_last_pos_;
  std::function<void(QPointF, double)> coordinate_callback_;
};
