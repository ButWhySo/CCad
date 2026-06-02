#include "ccad_gui/selection_inspector_panel.hpp"
#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QLineEdit>

namespace {

int visibleLineEditCount(const SelectionInspectorPanel& panel) {
  int count = 0;
  for (const QLineEdit* input : panel.findChildren<QLineEdit*>()) {
    if (input->isVisible()) {
      ++count;
    }
  }
  return count;
}

int childLineEditCount(const SelectionInspectorPanel& panel) {
  return panel.findChildren<QLineEdit*>().size();
}

}  // namespace

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  SelectionInspectorPanel panel;
  panel.show();
  QApplication::processEvents();
  require(panel.titleText() == "No selection", "empty inspector title");
  require(panel.detailText() == "Select a board object to inspect its stable identity.",
          "empty inspector detail");

  // Construct a Board with pad, via, track, keepout, and placement region
  ccad::Board board;

  // Add Pad
  ccad::Pad pad;
  pad.id = "pad_1";
  pad.position = ccad::Point{.x = ccad::millimeters(10.0), .y = ccad::millimeters(20.0)};
  pad.size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(2.0)};
  pad.rotation_degrees = 90.0;
  pad.net_id = "GND";
  pad.layers = {"F.Cu"};
  pad.component_id = "U1";
  pad.pin_name = "1";
  board.pads.push_back(pad);

  // Add Via
  ccad::Via via;
  via.id = "via_1";
  via.position = ccad::Point{.x = ccad::millimeters(5.0), .y = ccad::millimeters(5.0)};
  via.diameter = ccad::millimeters(0.6);
  via.drill = ccad::millimeters(0.3);
  via.net_id = "VCC";
  board.vias.push_back(via);

  // Add Track (dx = 3mm, dy = 4mm => length = 5mm)
  ccad::TrackSegment track;
  track.id = "track_1";
  track.start = ccad::Point{.x = ccad::millimeters(0.0), .y = ccad::millimeters(0.0)};
  track.end = ccad::Point{.x = ccad::millimeters(3.0), .y = ccad::millimeters(4.0)};
  track.width = ccad::millimeters(0.2);
  track.net_id = "SIG_A";
  track.layer_id = "B.Cu";
  track.source_route_request_id = "rr_1";
  board.tracks.push_back(track);

  // Add board graphic
  ccad::BoardGraphic graphic;
  graphic.id = "graphic_1";
  graphic.kind = "line";
  graphic.layer_id = "Dwgs.User";
  graphic.start = ccad::Point{.x = ccad::millimeters(3.0), .y = ccad::millimeters(4.0)};
  graphic.end = ccad::Point{.x = ccad::millimeters(16.0), .y = ccad::millimeters(4.0)};
  graphic.width = ccad::millimeters(0.15);
  board.graphics.push_back(graphic);

  // Add board text
  ccad::BoardText text;
  text.id = "text_1";
  text.layer_id = "F.SilkS";
  text.text = "Bridge rectifier";
  text.position = ccad::Point{.x = ccad::millimeters(8.0), .y = ccad::millimeters(22.0)};
  text.rotation_degrees = 90.0;
  text.size = ccad::Size{.width = ccad::millimeters(1.5),
                         .height = ccad::millimeters(1.5)};
  board.texts.push_back(text);

  // Add board zone
  ccad::BoardZone zone;
  zone.id = "zone_1";
  zone.name = "GND copper";
  zone.net_id = "GND";
  zone.layer_ids = {"F.Cu"};
  zone.outline = {ccad::Point{.x = ccad::millimeters(2.0), .y = ccad::millimeters(2.0)},
                  ccad::Point{.x = ccad::millimeters(20.0), .y = ccad::millimeters(2.0)},
                  ccad::Point{.x = ccad::millimeters(20.0), .y = ccad::millimeters(12.0)},
                  ccad::Point{.x = ccad::millimeters(2.0), .y = ccad::millimeters(12.0)}};
  zone.priority = 2;
  zone.clearance = ccad::millimeters(0.2);
  zone.min_thickness = ccad::millimeters(0.25);
  zone.fill_enabled = true;
  zone.pad_connection = "thermal";
  board.zones.push_back(zone);

  // Add Keepout
  ccad::Keepout keepout;
  keepout.id = "keepout_1";
  keepout.kind = "copper";
  keepout.area = ccad::Rect{
      .origin = ccad::Point{.x = ccad::millimeters(50.0), .y = ccad::millimeters(50.0)},
      .size = ccad::Size{.width = ccad::millimeters(10.0), .height = ccad::millimeters(10.0)}};
  board.keepouts.push_back(keepout);

  // Add PlacementRegion
  ccad::PlacementRegion pr;
  pr.id = "pr_1";
  pr.kind = "restricted";
  pr.area = ccad::Rect{
      .origin = ccad::Point{.x = ccad::millimeters(100.0), .y = ccad::millimeters(100.0)},
      .size = ccad::Size{.width = ccad::millimeters(20.0), .height = ccad::millimeters(20.0)}};
  board.placement_regions.push_back(pr);

  // Test Pad Inspector Formatting
  std::printf("Testing pad...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "pad", "pad_1");
  require(panel.titleText() == "Pad pad_1", "pad title");
  require(panel.rowText("Type") == "pad", "pad type");
  require(panel.rowText("ID") == "pad_1", "pad ID");
  require(panel.rowText("Position X") == "10.00 mm (393.70 mil)", "pad X");
  require(panel.rowText("Position Y") == "20.00 mm (787.40 mil)", "pad Y");
  require(panel.rowText("Width") == "1.5000", "pad width");
  require(panel.rowText("Height") == "2.0000", "pad height");
  require(panel.rowText("Rotation") == "90.0", "pad rotation");
  require(panel.rowText("Net") == "GND", "pad net");
  require(panel.rowText("Layer") == "F.Cu", "pad layer");
  require(panel.rowText("Component ID") == "U1", "pad component id");
  require(panel.rowText("Pin Name") == "1", "pad pin name");

  // Test Via Inspector Formatting
  std::printf("Testing via...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "via", "via_1");
  require(panel.titleText() == "Via via_1", "via title");
  require(panel.rowText("Position X") == "5.00 mm (196.85 mil)", "via X");
  require(panel.rowText("Position Y") == "5.00 mm (196.85 mil)", "via Y");
  require(panel.rowText("Diameter") == "0.6000", "via diameter");
  require(panel.rowText("Drill") == "0.3000", "via drill");
  require(panel.rowText("Net") == "VCC", "via net");

  // Test Track Inspector Formatting
  std::printf("Testing track...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "track", "track_1");
  require(panel.titleText() == "Track track_1", "track title");
  require(panel.rowText("Start X") == "0.00 mm (0.00 mil)", "track start X");
  require(panel.rowText("Start Y") == "0.00 mm (0.00 mil)", "track start Y");
  require(panel.rowText("End X") == "3.00 mm (118.11 mil)", "track end X");
  require(panel.rowText("End Y") == "4.00 mm (157.48 mil)", "track end Y");
  require(panel.rowText("Width") == "0.2000", "track width");
  require(panel.rowText("Length") == "5.00 mm (196.85 mil)", "track length");
  require(panel.rowText("Net") == "SIG_A", "track net");
  require(panel.rowText("Layer") == "B.Cu", "track layer");
  require(panel.rowText("Source Route Request") == "rr_1", "track route request");

  // Test Graphic Inspector Formatting
  std::printf("Testing graphic...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "line", "graphic_1");
  require(panel.titleText() == "Graphic graphic_1", "graphic title");
  require(panel.rowText("Type") == "graphic", "graphic type");
  require(panel.rowText("Kind") == "line", "graphic kind");
  require(panel.rowText("Layer") == "Dwgs.User", "graphic layer");
  require(panel.rowText("Start X") == "3.00 mm (118.11 mil)", "graphic start X");
  require(panel.rowText("Start Y") == "4.00 mm (157.48 mil)", "graphic start Y");
  require(panel.rowText("End X") == "16.00 mm (629.92 mil)", "graphic end X");
  require(panel.rowText("End Y") == "4.00 mm (157.48 mil)", "graphic end Y");
  require(panel.rowText("Width") == "0.15 mm (5.91 mil)", "graphic width");

  // Test Text Inspector Formatting
  std::printf("Testing text...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "text", "text_1");
  require(panel.titleText() == "Text text_1", "text title");
  require(panel.rowText("Type") == "text", "text type");
  require(panel.rowText("Layer") == "F.SilkS", "text layer");
  require(panel.rowText("Text") == "Bridge rectifier", "text payload");
  require(panel.rowText("Position X") == "8.00 mm (314.96 mil)", "text X");
  require(panel.rowText("Position Y") == "22.00 mm (866.14 mil)", "text Y");
  require(panel.rowText("Rotation") == "90.0 deg", "text rotation");
  require(panel.rowText("Size X") == "1.50 mm (59.06 mil)", "text size x");
  require(panel.rowText("Size Y") == "1.50 mm (59.06 mil)", "text size y");

  // Test Zone Inspector Formatting
  std::printf("Testing zone...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "zone", "zone_1");
  require(panel.titleText() == "Zone zone_1", "zone title");
  require(panel.rowText("Type") == "zone", "zone type");
  require(panel.rowText("Name") == "GND copper", "zone name");
  require(panel.rowText("Net") == "GND", "zone net");
  require(panel.rowText("Layers") == "F.Cu", "zone layers");
  require(panel.rowText("Corners") == "4", "zone corner count");
  require(panel.rowText("Priority") == "2", "zone priority");
  require(panel.rowText("Clearance") == "0.20 mm (7.87 mil)", "zone clearance");
  require(panel.rowText("Min Thickness") == "0.25 mm (9.84 mil)", "zone min thickness");
  require(panel.rowText("Pad Connection") == "thermal", "zone pad connection");

  // Rapid selection changes can happen when net highlight selects several canvas objects.
  // Stale editors must be hidden immediately, not only after Qt processes deleteLater().
  std::printf("Testing rapid selection rerender cleanup...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "pad", "pad_1");
  panel.renderSelection(board, "via", "via_1");
  panel.renderSelection(board, "track", "track_1");
  const int child_editors_after_rapid_selection = childLineEditCount(panel);
  require(child_editors_after_rapid_selection == 1,
          "rapid selection rerender leaves only current track editor owned, count=" +
              std::to_string(child_editors_after_rapid_selection));
  QApplication::processEvents();
  require(visibleLineEditCount(panel) == 1,
          "rapid selection rerender shows only current track editor after events");
  require(panel.rowText("Width") == "0.2000", "rapid selection rerender keeps current width");

  // Test Keepout Inspector Formatting
  std::printf("Testing keepout...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "keepout", "keepout_1");
  require(panel.titleText() == "Keepout keepout_1", "keepout title");
  require(panel.rowText("Origin X") == "50.00 mm (1968.50 mil)", "keepout X");
  require(panel.rowText("Origin Y") == "50.00 mm (1968.50 mil)", "keepout Y");
  require(panel.rowText("Width") == "10.0000", "keepout width");
  require(panel.rowText("Height") == "10.0000", "keepout height");
  require(panel.rowText("Kind") == "copper", "keepout kind");

  // Test PlacementRegion Inspector Formatting
  std::printf("Testing placement region...\n");
  std::fflush(stdout);
  panel.renderSelection(board, "placement_region", "pr_1");
  require(panel.titleText() == "Placement_region pr_1", "placement region title");
  require(panel.rowText("Origin X") == "100.00 mm (3937.01 mil)", "placement region X");
  require(panel.rowText("Origin Y") == "100.00 mm (3937.01 mil)", "placement region Y");
  require(panel.rowText("Width") == "20.0000", "placement region width");
  require(panel.rowText("Height") == "20.0000", "placement region height");
  require(panel.rowText("Kind") == "restricted", "placement region kind");

  // Test Board Rules Formatting
  std::printf("Testing board design rules...\n");
  std::fflush(stdout);
  panel.renderBoardRules(board);
  require(panel.titleText() == "Board Design Rules", "board rules title");
  require(panel.rowText("Copper Clearance") == "0.2000", "board rules copper clearance");
  require(panel.rowText("Min Track Width") == "0.1500", "board rules track width");
  require(panel.rowText("Via Annular Ring") == "0.1000", "board rules annular ring");

  // Check cleanup on deselect
  std::printf("Testing deselect...\n");
  std::fflush(stdout);
  panel.clearSelection();
  require(panel.titleText() == "No selection", "cleared title");
  require(panel.rowText("Position X").isEmpty(), "cleared position x");
  require(panel.rowText("Width").isEmpty(), "cleared width");

  std::printf("Exiting main with 0\n");
  std::fflush(stdout);
  return 0;
}
