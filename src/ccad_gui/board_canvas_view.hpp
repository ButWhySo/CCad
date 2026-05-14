#pragma once

#include <QGraphicsView>
#include <QResizeEvent>

class BoardCanvasView final : public QGraphicsView {
 public:
  using QGraphicsView::QGraphicsView;

 protected:
  void resizeEvent(QResizeEvent* event) override {
    QGraphicsView::resizeEvent(event);
    if (scene() != nullptr && !scene()->sceneRect().isEmpty()) {
      fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
  }
};
