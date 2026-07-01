#pragma once

#include <QGraphicsItem>
#include <QPointF>
#include <QRectF>
#include <QHash>
#include <QPair>
#include <QVector>
#include <QSet>
#include <cmath>
#include <algorithm>
#include <vector>

namespace ccad_gui {

class CanvasSpatialIndex {
public:
  CanvasSpatialIndex(double cellSize = 100.0) : cell_size_(cellSize) {}

  void clear() {
    grid_.clear();
  }

  void insert(QGraphicsItem* item) {
    if (!item) return;
    QRectF rect = item->sceneBoundingRect();
    int minX = static_cast<int>(std::floor(rect.left() / cell_size_));
    int maxX = static_cast<int>(std::floor(rect.right() / cell_size_));
    int minY = static_cast<int>(std::floor(rect.top() / cell_size_));
    int maxY = static_cast<int>(std::floor(rect.bottom() / cell_size_));

    for (int x = minX; x <= maxX; ++x) {
      for (int y = minY; y <= maxY; ++y) {
        QPair<int, int> cell(x, y);
        grid_[cell].append(item);
      }
    }
  }

  void rebuild(const QList<QGraphicsItem*>& items) {
    clear();
    for (QGraphicsItem* item : items) {
      insert(item);
    }
  }

  std::vector<QGraphicsItem*> queryNearest(QPointF scenePoint, double maxRadius = 1000.0) const {
    std::vector<QGraphicsItem*> result;
    QSet<QGraphicsItem*> visited;

    int cellX = static_cast<int>(std::floor(scenePoint.x() / cell_size_));
    int cellY = static_cast<int>(std::floor(scenePoint.y() / cell_size_));

    int cellRadius = static_cast<int>(std::ceil(maxRadius / cell_size_));

    for (int r = 0; r <= cellRadius; ++r) {
      for (int dx = -r; dx <= r; ++dx) {
        for (int dy = -r; dy <= r; ++dy) {
          if (std::abs(dx) != r && std::abs(dy) != r) continue;
          QPair<int, int> cell(cellX + dx, cellY + dy);
          auto it = grid_.find(cell);
          if (it != grid_.end()) {
            for (QGraphicsItem* item : it.value()) {
              if (!visited.contains(item)) {
                visited.insert(item);
                result.push_back(item);
              }
            }
          }
        }
      }
    }
    return result;
  }

private:
  double cell_size_;
  QHash<QPair<int, int>, QVector<QGraphicsItem*>> grid_;
};

} // namespace ccad_gui
