#pragma once

#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
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
  explicit BoardCanvasView(QWidget* parent = nullptr) : QGraphicsView(parent) {
    setBackgroundBrush(QColor("#07111f"));
    setContextMenuPolicy(Qt::CustomContextMenu);
  }
  explicit BoardCanvasView(QGraphicsScene* scene, QWidget* parent = nullptr) : QGraphicsView(scene, parent) {
    setBackgroundBrush(QColor("#07111f"));
    setContextMenuPolicy(Qt::CustomContextMenu);
  }
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
  void setObjectsMovedCallback(std::function<void(QPointF)> callback) {
    objects_moved_callback_ = std::move(callback);
  }
  void setPanModeCallback(std::function<void(bool, bool)> callback) {
    pan_mode_callback_ = std::move(callback);
    notifyPanModeChanged();
  }
  void setDoubleClickCallback(std::function<void()> callback) {
    double_click_callback_ = std::move(callback);
  }
  void setDeleteRequestedCallback(std::function<void()> callback) {
    delete_requested_callback_ = std::move(callback);
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
  bool gridVisible() const { return grid_visible_; }
  void setGridVisible(const bool visible) {
    if (grid_visible_ == visible) {
      return;
    }
    grid_visible_ = visible;
    viewport()->update();
  }
  bool crosshairVisible() const { return crosshair_visible_; }
  void setCrosshairVisible(const bool visible) {
    if (crosshair_visible_ == visible) {
      return;
    }
    crosshair_visible_ = visible;
    viewport()->update();
  }

 protected:
  void drawBackground(QPainter* painter, const QRectF& rect) override {
    QGraphicsView::drawBackground(painter, rect);
    if (!grid_visible_ || painter == nullptr) {
      return;
    }
    constexpr double grid_step = 10.0;
    QPen grid_pen(QColor("#20304a"));
    grid_pen.setCosmetic(true);
    painter->setPen(grid_pen);
    const double left = std::floor(rect.left() / grid_step) * grid_step;
    const double top = std::floor(rect.top() / grid_step) * grid_step;
    for (double x = left; x <= rect.right(); x += grid_step) {
      painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    }
    for (double y = top; y <= rect.bottom(); y += grid_step) {
      painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
    }
  }

  void drawForeground(QPainter* painter, const QRectF& rect) override {
    QGraphicsView::drawForeground(painter, rect);
    if (!crosshair_visible_ || painter == nullptr) {
      return;
    }
    const QPointF cursor_scene =
        last_cursor_scene_pos_.value_or(mapToScene(viewport()->rect().center()));
    QPen crosshair_pen(QColor("#d6e4ff"));
    crosshair_pen.setCosmetic(true);
    crosshair_pen.setStyle(Qt::DashLine);
    painter->setPen(crosshair_pen);
    painter->drawLine(QLineF(rect.left(), cursor_scene.y(), rect.right(), cursor_scene.y()));
    painter->drawLine(QLineF(cursor_scene.x(), rect.top(), cursor_scene.x(), rect.bottom()));
  }

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
    last_cursor_scene_pos_ = mapToScene(event->pos());
    if (crosshair_visible_) {
      viewport()->update();
    }
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

    if (dragging_objects_ && drag_start_scene_pos_.has_value()) {
      const QPointF delta = mapToScene(event->pos()) - drag_start_scene_pos_.value();
      for (const auto& pair : dragged_items_initial_pos_) {
        if (pair.first != nullptr) pair.first->setPos(pair.second + delta);
      }
      event->accept();
      return;
    }

    QGraphicsView::mouseMoveEvent(event);
    notifyViewportChanged(event->pos());
  }

  void mousePressEvent(QMouseEvent* event) override {
    const bool pan_gesture = event->button() == Qt::MiddleButton ||
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

    if (active_tool_ == ToolMode::Select && event->button() == Qt::LeftButton && scene() != nullptr) {
      QGraphicsItem* item = scene()->itemAt(mapToScene(event->pos()), transform());
      if (item != nullptr && item->isSelected()) {
        dragging_objects_ = true;
        drag_start_scene_pos_ = mapToScene(event->pos());
        dragged_items_initial_pos_.clear();
        for (QGraphicsItem* selected : scene()->selectedItems()) {
          dragged_items_initial_pos_.push_back({selected, selected->pos()});
        }
        event->accept();
        return;
      }
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

    if (dragging_objects_ && event->button() == Qt::LeftButton) {
      dragging_objects_ = false;
      if (drag_start_scene_pos_.has_value()) {
        const QPointF delta = mapToScene(event->pos()) - drag_start_scene_pos_.value();
        for (const auto& pair : dragged_items_initial_pos_) {
          if (pair.first != nullptr) pair.first->setPos(pair.second);
        }
        if (objects_moved_callback_ && (std::abs(delta.x()) > 0.01 || std::abs(delta.y()) > 0.01)) {
          objects_moved_callback_(delta);
        }
      }
      dragged_items_initial_pos_.clear();
      event->accept();
      return;
    }

    QGraphicsView::mouseReleaseEvent(event);
  }

  void mouseDoubleClickEvent(QMouseEvent* event) override {
    if (double_click_callback_) {
      double_click_callback_();
    }
    QGraphicsView::mouseDoubleClickEvent(event);
  }

  void keyPressEvent(QKeyEvent* event) override {
    constexpr int pan_step_pixels = 48;
    switch (event->key()) {
      case Qt::Key_Delete:
      case Qt::Key_Backspace:
        if (delete_requested_callback_) delete_requested_callback_();
        event->accept();
        return;
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
  bool dragging_objects_ = false;
  std::optional<QPointF> drag_start_scene_pos_;
  std::vector<std::pair<QGraphicsItem*, QPointF>> dragged_items_initial_pos_;
  QGraphicsLineItem* measure_line_ = nullptr;
  QGraphicsTextItem* measure_text_ = nullptr;
  QGraphicsRectItem* measure_text_bg_ = nullptr;

  double zoom_factor_ = 1.0;
  bool user_view_ = false;
  bool panning_ = false;
  bool space_pan_mode_ = false;
  bool grid_visible_ = true;
  bool crosshair_visible_ = false;
  QPoint pan_last_pos_;
  std::optional<QPointF> last_cursor_scene_pos_;
  std::function<void(QPointF, double)> coordinate_callback_;
  std::function<void(QPointF)> objects_moved_callback_;
  std::function<void(bool, bool)> pan_mode_callback_;
  std::function<void()> double_click_callback_;
  std::function<void()> delete_requested_callback_;
};
