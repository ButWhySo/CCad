#include "ccad_gui/object_browser_panel.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QListWidget>
#include <QMetaObject>

namespace {

ccad::CanvasScene browserScene() {
  ccad::CanvasScene scene;
  scene.has_board = true;
  scene.layers.push_back(
      ccad::CanvasLayer{.id = "F.Cu", .name = "Front copper", .kind = "signal", .visible = true});
  scene.layers.push_back(
      ccad::CanvasLayer{.id = "B.Cu", .name = "Back copper", .kind = "signal", .visible = false});
  scene.keepouts.push_back(ccad::CanvasKeepout{.id = "K1", .kind = "placement"});
  scene.pads.push_back(ccad::CanvasPad{.id = "P1", .net_id = "N1", .layers = {"F.Cu"}, .type = "smd", .shape = "rect"});
  scene.vias.push_back(ccad::CanvasVia{.id = "V1", .net_id = "N1"});
  scene.tracks.push_back(ccad::CanvasTrack{
      .id = "T1", .net_id = "N1", .layer_id = "F.Cu", .source_route_request_id = "RR1"});
  scene.route_requests.push_back(ccad::CanvasRouteRequest{.id = "RR1",
                                                          .net_id = "N1",
                                                          .from_object_id = "P1",
                                                          .to_object_id = "V1",
                                                          .preferred_layer_id = "F.Cu",
                                                          .policy = "shortest_safe",
                                                          .width_nm = 250000,
                                                          .routed_segment_count = 1});
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
  require(panel.itemCount() == 12, "browser has layer, net, route, and object rows");
  require(panel.itemText(0) == "Layers (2)", "layer section row");
  require(panel.itemText(1) == "F.Cu - Front copper [signal, visible]", "front layer row");
  require(panel.itemText(2) == "B.Cu - Back copper [signal, hidden]", "hidden layer row");
  require(panel.itemText(3) == "Nets (1)", "net section row");
  require(panel.itemText(4) == "net N1  objects 3", "net row summarizes member count");
  require(panel.itemText(5) == "Route Requests (1)", "route request section row");
  require(panel.itemText(6) ==
              "route RR1  net N1  P1 -> V1  layer F.Cu  partial 1 segment(s)",
          "route request row summarizes routing intent");
  require(panel.itemText(7) == "Objects (4)", "object section row");
  require(panel.itemText(8) == "pad P1  net N1  layer F.Cu", "pad object row");
  require(panel.itemText(9) == "via V1  net N1", "via object row");
  require(panel.itemText(10) == "track T1  net N1  layer F.Cu  route RR1",
          "track object row includes route provenance");
  require(panel.itemText(11) == "keepout K1  kind placement", "keepout object row");
  require(panel.objectIdForRow(0).isEmpty(), "section rows do not expose object ids");
  require(panel.objectIdForRow(1).isEmpty(), "layer rows do not expose object ids");
  require(panel.objectIdForRow(4).isEmpty(), "net rows do not expose object ids");
  require(panel.netIdForRow(4) == "N1", "net row exposes net id");
  require(panel.objectIdForRow(6).isEmpty(), "route request row does not fake object id");
  require(panel.objectIdForRow(8) == "P1", "pad row exposes object id");
  require(panel.objectIdForRow(9) == "V1", "via row exposes object id");
  require(panel.objectIdForRow(10) == "T1", "track row exposes object id");
  require(panel.objectIdForRow(11) == "K1", "keepout row exposes object id");
  require(panel.objectIdForRow(99).isEmpty(), "out of range rows do not expose object ids");
  require(panel.netIdForRow(99).isEmpty(), "out of range rows do not expose net ids");

  QString activated_id;
  QString activated_net_id;
  QString activated_route_id;
  panel.setObjectActivatedCallback([&activated_id](const QString& object_id) {
    activated_id = object_id;
  });
  panel.setNetActivatedCallback([&activated_net_id](const QString& net_id) {
    activated_net_id = net_id;
  });
  panel.setRouteActivatedCallback([&activated_route_id](const QString& route_id) {
    activated_route_id = route_id;
  });
  auto* list = panel.findChild<QListWidget*>("objectBrowserPanel");
  require(list != nullptr, "browser list is discoverable for interaction tests");
  QMetaObject::invokeMethod(list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, list->item(8)));
  require(activated_id == "P1", "clicking object row activates object id");
  QMetaObject::invokeMethod(list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, list->item(4)));
  require(activated_net_id == "N1", "clicking net row activates net id");
  QMetaObject::invokeMethod(list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, list->item(6)));
  require(activated_route_id == "RR1", "clicking route row activates route id");
  QMetaObject::invokeMethod(list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, list->item(1)));
  require(activated_id == "P1", "clicking non-object row does not activate object id");
  require(activated_net_id == "N1", "clicking non-net row does not activate net id");

  // Sprint 130 checkable layer and toggling test
  require(list->item(1)->flags() & Qt::ItemIsUserCheckable, "front layer row is checkable");
  require(list->item(2)->flags() & Qt::ItemIsUserCheckable, "back layer row is checkable");
  require(list->item(1)->checkState() == Qt::Checked, "front layer row is checked by default");
  require(list->item(2)->checkState() == Qt::Unchecked, "back layer row is unchecked by default");

  QString toggled_layer_id;
  bool toggled_visible = false;
  panel.setLayerToggledCallback([&toggled_layer_id, &toggled_visible](const QString& layer_id, bool visible) {
    toggled_layer_id = layer_id;
    toggled_visible = visible;
  });

  list->item(1)->setCheckState(Qt::Unchecked);
  require(toggled_layer_id == "F.Cu", "toggling F.Cu checkbox fires callback with layer id");
  require(!toggled_visible, "toggling F.Cu checkbox fires callback with visible = false");
}

