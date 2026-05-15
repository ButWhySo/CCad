#include "ccad_gui/object_browser_panel.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QListWidget>
#include <QMetaObject>

namespace {

ccad::CanvasScene browserScene() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.layers.push_back(ccad::CanvasLayer{.id = "F.Cu", .name = "Front copper", .kind = "signal"});
  scene.layers.push_back(ccad::CanvasLayer{.id = "B.Cu", .name = "Back copper", .kind = "signal"});
  scene.keepouts.push_back(ccad::CanvasKeepout{.id = "K1", .kind = "placement"});
  scene.pads.push_back(ccad::CanvasPad{.id = "P1", .net_id = "N1", .layer_id = "F.Cu"});
  scene.vias.push_back(ccad::CanvasVia{.id = "V1", .net_id = "N1"});
  scene.tracks.push_back(ccad::CanvasTrack{.id = "T1", .net_id = "N1", .layer_id = "F.Cu"});
  return scene;
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  ObjectBrowserPanel panel;
  panel.renderScene(ccad::CanvasScene{});
  require(panel.itemCount() == 1, "empty browser has one status row");
  require(panel.itemText(0) == "No board objects", "empty browser status text");

  panel.renderScene(browserScene());
  require(panel.itemCount() == 8, "browser has layer and object rows");
  require(panel.itemText(0) == "Layers (2)", "layer section row");
  require(panel.itemText(1) == "F.Cu - Front copper [signal]", "front layer row");
  require(panel.itemText(3) == "Objects (4)", "object section row");
  require(panel.itemText(4) == "pad P1  net N1  layer F.Cu", "pad object row");
  require(panel.itemText(5) == "via V1  net N1", "via object row");
  require(panel.itemText(6) == "track T1  net N1  layer F.Cu", "track object row");
  require(panel.itemText(7) == "keepout K1  kind placement", "keepout object row");
  require(panel.objectIdForRow(0).isEmpty(), "section rows do not expose object ids");
  require(panel.objectIdForRow(1).isEmpty(), "layer rows do not expose object ids");
  require(panel.objectIdForRow(4) == "P1", "pad row exposes object id");
  require(panel.objectIdForRow(5) == "V1", "via row exposes object id");
  require(panel.objectIdForRow(6) == "T1", "track row exposes object id");
  require(panel.objectIdForRow(7) == "K1", "keepout row exposes object id");
  require(panel.objectIdForRow(99).isEmpty(), "out of range rows do not expose object ids");

  QString activated_id;
  panel.setObjectActivatedCallback([&activated_id](const QString& object_id) {
    activated_id = object_id;
  });
  auto* list = panel.findChild<QListWidget*>("objectBrowserPanel");
  require(list != nullptr, "browser list is discoverable for interaction tests");
  QMetaObject::invokeMethod(list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, list->item(4)));
  require(activated_id == "P1", "clicking object row activates object id");
  QMetaObject::invokeMethod(list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, list->item(1)));
  require(activated_id == "P1", "clicking non-object row does not activate object id");
}
