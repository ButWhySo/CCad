#include "ccad_gui/spatial_index.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QGraphicsRectItem>
#include <QGraphicsScene>

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  ccad_gui::CanvasSpatialIndex index(50.0);

  QGraphicsScene scene;
  QGraphicsRectItem* item1 = new QGraphicsRectItem(0, 0, 10, 10);   // Center (5,5)
  QGraphicsRectItem* item2 = new QGraphicsRectItem(300, 300, 10, 10); // Center (305,305)

  scene.addItem(item1);
  scene.addItem(item2);

  index.insert(item1);
  index.insert(item2);

  // Query nearest to (5,5)
  std::vector<QGraphicsItem*> nearest1 = index.queryNearest(QPointF(5, 5), 20.0);
  require(nearest1.size() == 1, "Should find item1 near (5,5)");
  require(nearest1[0] == item1, "Found item must be item1");

  // Query nearest to (305,305)
  std::vector<QGraphicsItem*> nearest2 = index.queryNearest(QPointF(305, 305), 20.0);
  require(nearest2.size() == 1, "Should find item2 near (305,305)");
  require(nearest2[0] == item2, "Found item must be item2");

  // Query in between with a large radius to find both
  std::vector<QGraphicsItem*> nearestBoth = index.queryNearest(QPointF(150, 150), 300.0);
  require(nearestBoth.size() == 2, "Should find both items with large search radius");
}
