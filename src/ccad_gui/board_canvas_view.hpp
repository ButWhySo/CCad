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
#include <optional>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>

enum class ToolMode { Select, Measure };

class BoardCanvasView final : public QGraphicsView {
 public:
  using QGraphicsView::QGraphicsView;
  static constexpr double kMinZoomFactor = 0.05;
  static constexpr double kMaxZoomFactor = 40.0;

  void zoomToFit() {
    if (scene() == nullptr || scene()->itemsBoundingRect().isEmpty()) {
      return;
    }
    const QRectF content = scene()->itemsBoundingRect();
    const double pad_x = std::max(24.0, content.width() * 0.06);
    const double pad_y = std::max(24.0, content.height() * 0.06);
    fitInView(content.adjusted(-pad_x, -pad_y, pad_x, pad_y), Qt::KeepAspectRatio);
    zoom_factor_ = std::clamp(transform().m11(), kMinZoomFactor, kMaxZoomFactor);
    user_view_ = false;
    notifyViewportChanged();
  }

  double zoomFactor() const { return zoom_factor_; }
  void setCoordinateCallback(std::function<void(QPointF, double)> callback) {
    coordinate_callback_ = std::move(callback);
  }
  void setPanModeCallback(std::function<void(bool, bool)> callback) {
    pan_mode_callback_ = std::move(callback);
    notifyPanModeChanged();
  }
  void zoomIn() { zoomBy(1.18); }
  void zoomOut() { zoomBy(1.0 / 1.18); }
  void resetZoom() {
    resetTransform();
    zoom_factor_ = 1.0;
    user_view_ = true;
    notifyViewportChanged();
  }

  void setToolMode(ToolMode mode) {
    active_tool_ = mode;
    clearMeasurement();
  }

  ToolMode getToolMode() const { return active_tool_; }

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
    
    if (active_tool_ == ToolMode::Measure && measure_start_pos_.has_value()) {
      updateMeasurement(mapToScene(event->pos()));
    }

    QGraphicsView::mouseMoveEvent(event);
    notifyViewportChanged(event->pos());
  }

  void mousePressEvent(QMouseEvent* event) override {
    const bool pan_gesture = event->button() == Qt::MiddleButton ||
                             event->button() == Qt::RightButton ||
                             (event->button() == Qt::LeftButton && space_pan_mode_) ||
                             (event->button() == Qt::LeftButton &&
                              (event->modifiers() & Qt::KeyboardModifier::ShiftModifier));
    if (pan_gesture) {
      panning_ = true;
      pan_last_pos_ = event->pos();
      viewport()->setCursor(Qt::ClosedHandCursor);
      user_view_ = true;
      notifyPanModeChanged();
      event->accept();
      return;
    }
    
    if (active_tool_ == ToolMode::Measure && event->button() == Qt::LeftButton) {
      if (!measure_start_pos_.has_value()) {
        measure_start_pos_ = mapToScene(event->pos());
        createMeasurementOverlay();
      } else {
        clearMeasurement();
      }
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
      if (space_pan_mode_) {
        viewport()->setCursor(Qt::OpenHandCursor);
      }
      notifyPanModeChanged();
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
      case Qt::Key::Key_Home:
        zoomToFit();
        event->accept();
        return;
      case Qt::Key::Key_Left:
      case Qt::Key::Key_A:
        panByPixels(-pan_step_pixels, 0);
        event->accept();
        return;
      case Qt::Key::Key_Right:
      case Qt::Key::Key_D:
        panByPixels(pan_step_pixels, 0);
        event->accept();
        return;
      case Qt::Key::Key_Up:
      case Qt::Key::Key_W:
        panByPixels(0, -pan_step_pixels);
        event->accept();
        return;
      case Qt::Key::Key_Down:
      case Qt::Key::Key_S:
        panByPixels(0, pan_step_pixels);
        event->accept();
        return;
      case Qt::Key::Key_Space:
        space_pan_mode_ = true;
        if (!panning_) {
          viewport()->setCursor(Qt::OpenHandCursor);
        }
        notifyPanModeChanged();
        event->accept();
        return;
      default:
        break;
    }
    QGraphicsView::keyPressEvent(event);
  }

  void keyReleaseEvent(QKeyEvent* event) override {
    if (event->key() == Qt::Key::Key_Space) {
      space_pan_mode_ = false;
      if (!panning_) {
        viewport()->unsetCursor();
      }
      notifyPanModeChanged();
      event->accept();
      return;
    }
    QGraphicsView::keyReleaseEvent(event);
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

  void notifyPanModeChanged() {
    if (pan_mode_callback_) {
      pan_mode_callback_(space_pan_mode_, panning_);
    }
  }

  void createMeasurementOverlay() {
    if (!scene() || !measure_start_pos_.has_value()) return;
    measure_line_ = scene()->addLine(QLineF(*measure_start_pos_, *measure_start_pos_), QPen(Qt::cyan, 0));
    measure_line_->setZValue(1000);
    measure_text_ = scene()->addText("");
    measure_text_->setDefaultTextColor(Qt::cyan);
    QFont font = measure_text_->font();
    font.setPixelSize(14);
    measure_text_->setFont(font);
    measure_text_->setZValue(1000);
    // Add background to text
    measure_text_bg_ = scene()->addRect(measure_text_->boundingRect(), QPen(Qt::NoPen), QBrush(QColor(0, 0, 0, 180)));
    measure_text_bg_->setZValue(999);
    measure_text_->setFlag(QGraphicsItem::ItemIgnoresTransformations);
    measure_text_bg_->setFlag(QGraphicsItem::ItemIgnoresTransformations);
  }

  void updateMeasurement(const QPointF& current_pos) {
    if (!measure_line_ || !measure_text_ || !measure_text_bg_ || !measure_start_pos_.has_value()) return;
    measure_line_->setLine(QLineF(*measure_start_pos_, current_pos));
    
    const double dx = current_pos.x() - measure_start_pos_->x();
    const double dy = current_pos.y() - measure_start_pos_->y();
    const double dist = std::hypot(dx, dy);
    
    measure_text_->setPlainText(QString("dx: %1 mm\ndy: %2 mm\ndist: %3 mm")
                                    .arg(dx, 0, 'f', 3)
                                    .arg(-dy, 0, 'f', 3) // Canvas Y is inverted relative to physical Cartesian
                                    .arg(dist, 0, 'f', 3));
                                    
    // Position text near cursor
    measure_text_->setPos(current_pos + QPointF(10, 10));
    measure_text_bg_->setRect(measure_text_->boundingRect());
    measure_text_bg_->setPos(measure_text_->pos());
  }

  void clearMeasurement() {
    if (scene()) {
      if (measure_line_) scene()->removeItem(measure_line_);
      if (measure_text_) scene()->removeItem(measure_text_);
      if (measure_text_bg_) scene()->removeItem(measure_text_bg_);
    }
    delete measure_line_;
    delete measure_text_;
    delete measure_text_bg_;
    measure_line_ = nullptr;
    measure_text_ = nullptr;
    measure_text_bg_ = nullptr;
    measure_start_pos_.reset();
  }

  ToolMode active_tool_ = ToolMode::Select;
  std::optional<QPointF> measure_start_pos_;
  QGraphicsLineItem* measure_line_ = nullptr;
  QGraphicsTextItem* measure_text_ = nullptr;
  QGraphicsRectItem* measure_text_bg_ = nullptr;

  double zoom_factor_ = 1.0;
  bool user_view_ = false;
  bool panning_ = false;
  bool space_pan_mode_ = false;
  QPoint pan_last_pos_;
  std::function<void(QPointF, double)> coordinate_callback_;
  std::function<void(bool, bool)> pan_mode_callback_;
};
