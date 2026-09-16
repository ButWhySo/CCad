#include "ccad_core/dsn_export.hpp"
#include "ccad_core/dsn_import.hpp"

#include <iostream>

using namespace ccad;

void assertContains(const std::string& content, const std::string& pattern, const std::string& label) {
  if (content.find(pattern) == std::string::npos) {
    std::cerr << "FAIL " << label << "\n  pattern not found: " << pattern << '\n';
    std::exit(1);
  }
}

void testDsnExport() {
  Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.name = "test_dsn";
  Board board;
  board.outline = Rect{Point{nanometers(0), nanometers(0)}, Size{nanometers(42000000), nanometers(28000000)}};
  board.layers.push_back(Layer{.id = "F.Cu", .name = "Front", .kind = "copper"});
  board.design_rules = DesignRules{.copper_clearance = nanometers(200000), .min_track_width = nanometers(150000), .min_via_annular_ring = nanometers(100000)};
  
  Pad p1;
  p1.id = "U1.1";
  p1.component_id = "U1";
  p1.pin_name = "1";
  p1.net_id = "N1";
  p1.position = Point{nanometers(5000000), nanometers(6000000)};
  p1.padstack = Padstack{
    .layer_set = {"F.Cu"},
    .copper_props = {{"top", PadstackCopperLayerProps{.shape = PadstackShapeProps{.shape = PadShape::Rectangle, .size = Size{nanometers(1500000), nanometers(1000000)}}}}}
  };
  board.pads.push_back(p1);

  Via v1;
  v1.id = "V1";
  v1.net_id = "N1";
  v1.position = Point{nanometers(8000000), nanometers(9000000)};
  v1.diameter = nanometers(800000);
  v1.drill = nanometers(400000);
  board.vias.push_back(v1);

  TrackSegment t1;
  t1.id = "T1";
  t1.net_id = "N1";
  t1.layer_id = "F.Cu";
  t1.start = Point{nanometers(5000000), nanometers(6000000)};
  t1.end = Point{nanometers(8000000), nanometers(9000000)};
  t1.width = nanometers(250000);
  board.tracks.push_back(t1);

  project.boards.clear(); project.boards.push_back(board);

  std::string exported = exportSpecctraDsn(project);

  assertContains(exported, "(pcb \"test_dsn\"", "has pcb root");
  assertContains(exported, "(layer \"F.Cu\" (type signal))", "has copper layer");
  assertContains(exported, "(path pcb 0 0.0000 0.0000 42.0000 0.0000 42.0000 28.0000 0.0000 28.0000 0.0000 0.0000)", "has boundary path");
  assertContains(exported, "(clearance 0.2000)", "has clearance rule");
  assertContains(exported, "(component \"U1\"", "has component U1");
  assertContains(exported, "(image \"U1\"", "has image U1");
  assertContains(exported, "(pin \"padstack_U1.1\" 1 5.0000 6.0000)", "has pin U1.1");
  assertContains(exported, "(padstack \"padstack_U1.1\"", "has padstack U1.1");
  assertContains(exported, "(rect \"F.Cu\" -0.7500 -0.5000 0.7500 0.5000)", "has pad rect");
  assertContains(exported, "(padstack \"viastack_V1\"", "has viastack");
  assertContains(exported, "(circle \"F.Cu\" 0.4000)", "has via circle F.Cu");
  assertContains(exported, "(circle \"B.Cu\" 0.4000)", "has via circle B.Cu");
  assertContains(exported, "(net \"N1\" (pins \"U1\"-1))", "has network N1");
  assertContains(exported, "(wire (path \"F.Cu\" 0.2500 5.0000 6.0000 8.0000 9.0000) (net \"N1\"))", "has wire T1");
  assertContains(exported, "(via \"viastack_V1\" 8.0000 9.0000 (net \"N1\"))", "has via placement");
}

void testDsnExportKeepsLargeNanometerCoordinates() {
  Project project;
  project.schematics.push_back(ccad::Schematic{});
  project.name = "large_dsn";
  Board board;
  board.outline = Rect{Point{nanometers(3000000000LL), nanometers(4000000000LL)},
                       Size{nanometers(42000000), nanometers(28000000)}};
  board.layers.push_back(Layer{.id = "F.Cu", .name = "Front", .kind = "copper"});
  board.design_rules = DesignRules{.copper_clearance = nanometers(200000),
                                   .min_track_width = nanometers(150000),
                                   .min_via_annular_ring = nanometers(100000)};
  project.boards.clear(); project.boards.push_back(board);

  const std::string exported = exportSpecctraDsn(project);
  assertContains(exported,
                 "(path pcb 0 3000.0000 4000.0000 3042.0000 4000.0000 3042.0000 4028.0000 3000.0000 4028.0000 3000.0000 4000.0000)",
                 "keeps large 64-bit board boundary coordinates");
}

void testSesImport() {
  const std::string ses = "(session (route (library (padstack \"Via_15:8_mil\" (shape (circle \"F.Cu\" 0.5)) (shape (circle \"B.Cu\" 0.5)))) (network (net \"N1\" (wire (path \"F.Cu\" 0.25 1.0 2.0 3.0 4.0 5.0 6.0)) (via \"Via_15:8_mil\" 7.0 8.0))))))";
  auto routing = importSpecctraSes(ses);
  if (routing.tracks.size() != 2) throw std::runtime_error("expected 2 tracks from polyline");
  if (routing.vias.size() != 1) throw std::runtime_error("expected 1 via");
  
  if (routing.tracks[0].net_id != "N1" || routing.tracks[0].start.x.nanometers != 1000000 || routing.tracks[0].end.y.nanometers != 4000000) {
    throw std::runtime_error("track 0 coordinates wrong");
  }
  if (routing.tracks[1].start.x.nanometers != 3000000 || routing.tracks[1].end.y.nanometers != 6000000) {
    throw std::runtime_error("track 1 coordinates wrong");
  }
  if (routing.vias[0].net_id != "N1" || routing.vias[0].diameter.nanometers != 1000000 || routing.vias[0].drill.nanometers != 203200 || routing.vias[0].position.x.nanometers != 7000000 || routing.vias[0].position.y.nanometers != 8000000) {
    throw std::runtime_error("via coordinates wrong");
  }
  if (routing.vias[0].start_layer_id != "F.Cu" || routing.vias[0].end_layer_id != "B.Cu" || routing.vias[0].via_type != "through") {
    throw std::runtime_error("via layer span wrong");
  }
}

void testSesRejectsUnsupportedViaShape() {
  try {
    importSpecctraSes("(session (route (library (padstack \"bad\" (shape (rect \"F.Cu\" 0 0 1 1)))) (network (net \"N1\" (via \"bad\" 1 2)))))");
    throw std::runtime_error("unsupported via shape was accepted");
  } catch (const std::runtime_error& error) {
    if (std::string(error.what()).find("no supported circle shape") == std::string::npos) throw;
  }
}

int main() {
  try {
    testDsnExport();
    testDsnExportKeepsLargeNanometerCoordinates();
    testSesImport();
    testSesRejectsUnsupportedViaShape();
    std::cout << "PASS dsn export and ses import\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
