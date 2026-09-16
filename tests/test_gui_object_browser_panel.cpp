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
  scene.pads.push_back(ccad::CanvasPad{.id = "P1", .net_id = "N1", .layers = {"F.Cu"}, .type = "smd", .shape = "rect", .pin_type = ""});
  scene.vias.push_back(ccad::CanvasVia{.id = "V1", .net_id = "N1"});
  scene.tracks.push_back(ccad::CanvasTrack{
      .id = "T1", .net_id = "N1", .layer_id = "F.Cu", .source_route_request_id = "RR1"});
  scene.lines.push_back(ccad::CanvasLine{.id = "G1", .layer_id = "Dwgs.User"});
  scene.texts.push_back(ccad::CanvasText{.id = "BT1", .layer_id = "F.SilkS", .text = "RECTIFIER"});
  scene.zones.push_back(ccad::CanvasZone{
      .id = "Z1",
      .name = "GND copper",
      .net_id = "N1",
      .layer_ids = {"F.Cu"},
      .pts_x_units = {2.0, 18.0, 18.0, 2.0},
      .pts_y_units = {2.0, 2.0, 12.0, 12.0},
      .pad_connection = "thermal",
      .thermal_spokes = {}});
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
  auto* layers_list = panel.findChild<QListWidget*>("layersList");
  auto* objects_list = panel.findChild<QListWidget*>("objectBrowserPanel");
  auto* nets_list = panel.findChild<QListWidget*>("netsList");
  require(layers_list != nullptr, "layers list is discoverable for interaction tests");
  require(objects_list != nullptr, "objects list is discoverable for interaction tests");
  require(nets_list != nullptr, "nets list is discoverable for interaction tests");
  require(layers_list->item(0)->text() == "No board objects", "empty browser status text");

  panel.renderScene(browserScene());
  require(panel.itemCount() == 15, "browser has layer, net, route, and object rows");
  require(layers_list->item(0)->text() == "Layers (2)", "layer section row");
  require(layers_list->item(1)->text() == "F.Cu - Front copper [signal, visible]", "front layer row");
  require(layers_list->item(2)->text() == "B.Cu - Back copper [signal, hidden]", "hidden layer row");
  require(nets_list->item(0)->text() == "Nets (1)", "net section row");
  require(nets_list->item(1)->text() == "net N1  objects 4", "net row summarizes member count");
  require(nets_list->item(2)->text() == "Route Requests (1)", "route request section row");
  require(nets_list->item(3)->text() ==
              "route RR1  net N1  P1 -> V1  layer F.Cu  partial 1 segment(s)",
          "route request row summarizes routing intent");
  require(objects_list->item(0)->text() == "Objects (7)", "object section row");
  require(objects_list->item(1)->text() == "pad P1  net N1  layer F.Cu", "pad object row");
  require(objects_list->item(2)->text() == "via V1  net N1", "via object row");
  require(objects_list->item(3)->text() == "track T1  net N1  layer F.Cu",
          "track object row includes route provenance");
  require(objects_list->item(4)->text() == "graphic G1  layer Dwgs.User", "graphic object row");
  require(objects_list->item(5)->text() == "text BT1  layer F.SilkS  RECTIFIER", "text object row");
  require(objects_list->item(6)->text() == "zone Z1  net N1  layers F.Cu", "zone object row");
  require(objects_list->item(7)->text() == "keepout K1  kind placement", "keepout object row");

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
  QMetaObject::invokeMethod(objects_list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, objects_list->item(1)));
  require(activated_id == "P1", "clicking object row activates object id");
  QMetaObject::invokeMethod(nets_list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, nets_list->item(1)));
  require(activated_net_id == "N1", "clicking net row activates net id");
  QMetaObject::invokeMethod(nets_list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, nets_list->item(3)));
  require(activated_route_id == "RR1", "clicking route row activates route id");
  QMetaObject::invokeMethod(layers_list, "itemClicked", Qt::DirectConnection,
                            Q_ARG(QListWidgetItem*, layers_list->item(1)));
  require(activated_id == "P1", "clicking non-object row does not activate object id");
  require(activated_net_id == "N1", "clicking non-net row does not activate net id");

  // Sprint 130 checkable layer and toggling test
  require(layers_list->item(1)->flags() & Qt::ItemIsUserCheckable, "front layer row is checkable");
  require(layers_list->item(2)->flags() & Qt::ItemIsUserCheckable, "back layer row is checkable");
  require(layers_list->item(1)->checkState() == Qt::Checked, "front layer row is checked by default");
  require(layers_list->item(2)->checkState() == Qt::Unchecked, "back layer row is unchecked by default");

  QString toggled_layer_id;
  bool toggled_visible = false;
  panel.setLayerToggledCallback([&toggled_layer_id, &toggled_visible](const QString& layer_id, bool visible) {
    toggled_layer_id = layer_id;
    toggled_visible = visible;
  });

  layers_list->item(1)->setCheckState(Qt::Unchecked);
  require(toggled_layer_id == "F.Cu", "toggling F.Cu checkbox fires callback with layer id");
  require(!toggled_visible, "toggling F.Cu checkbox fires callback with visible = false");
}

