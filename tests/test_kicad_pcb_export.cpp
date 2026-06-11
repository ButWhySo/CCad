#include "ccad_core/kicad_pcb_export.hpp"
#include "test_support.hpp"

#include <iostream>
#include <string>

int main() {
  ccad::Project project;
  project.id = "proj_123";
  project.name = "Test Project";

  ccad::Board board;
  board.outline = ccad::Rect{
      .origin = ccad::Point{.x = ccad::millimeters(0), .y = ccad::millimeters(0)},
      .size = ccad::Size{.width = ccad::millimeters(100), .height = ccad::millimeters(80)}
  };

  board.design_rules = ccad::DesignRules{
      .copper_clearance = ccad::millimeters(0.25),
      .min_track_width = ccad::millimeters(0.20),
      .min_via_annular_ring = ccad::millimeters(0.15)
  };

  project.schematics.push_back(ccad::Schematic{});
  project.schematics[0].nets = {
      ccad::Net{.id = "GND"},
      ccad::Net{.id = "VCC"}
  };

  project.schematics[0].components = {
      ccad::Component{.id = "U1", .part = "NE555"}
  };

  // Add a pad
  board.pads.push_back(ccad::Pad{
      .id = "pad1",
      .component_id = "U1",
      .pin_name = "1",
      .net_id = "GND",
      .layers = {"F.Cu"},
      .type = "smd",
      .shape = "rect",
      .position = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(20)},
      .rotation_degrees = 45.0,
      .size = ccad::Size{.width = ccad::millimeters(1.5), .height = ccad::millimeters(2.0)}
  });
  board.pads.push_back(ccad::Pad{
      .id = "pad2",
      .component_id = "U1",
      .pin_name = "2",
      .net_id = "VCC",
      .layers = {"F.Cu", "F.Paste", "F.Mask"},
      .type = "smd",
      .shape = "roundrect",
      .position = ccad::Point{.x = ccad::millimeters(12), .y = ccad::millimeters(20)},
      .size = ccad::Size{.width = ccad::millimeters(1.0), .height = ccad::millimeters(1.5)},
      .roundrect_rratio = 0.25
  });
  board.pads.push_back(ccad::Pad{
      .id = "pad3",
      .component_id = "U1",
      .pin_name = "3",
      .net_id = "VCC",
      .layers = {"F.Cu", "F.Paste", "F.Mask"},
      .type = "smd",
      .shape = "chamfered_rect",
      .position = ccad::Point{.x = ccad::millimeters(14), .y = ccad::millimeters(20)},
      .size = ccad::Size{.width = ccad::millimeters(1.2), .height = ccad::millimeters(1.5)},
      .chamfer_ratio = 0.2
  });

  // Add a via
  board.vias.push_back(ccad::Via{
      .id = "via1",
      .net_id = "GND",
      .position = ccad::Point{.x = ccad::millimeters(30), .y = ccad::millimeters(40)},
      .diameter = ccad::millimeters(0.8),
      .drill = ccad::millimeters(0.4)
  });

  // Add a track
  board.tracks.push_back(ccad::TrackSegment{
      .id = "track1",
      .net_id = "VCC",
      .layer_id = "F.Cu",
      .start = ccad::Point{.x = ccad::millimeters(10), .y = ccad::millimeters(20)},
      .end = ccad::Point{.x = ccad::millimeters(30), .y = ccad::millimeters(40)},
      .width = ccad::millimeters(0.25)
  });

  // Add a board graphic line
  board.graphics.push_back(ccad::BoardGraphic{
      .id = "G1",
      .kind = "line",
      .layer_id = "Dwgs.User",
      .start = ccad::Point{.x = ccad::millimeters(3), .y = ccad::millimeters(4)},
      .end = ccad::Point{.x = ccad::millimeters(16), .y = ccad::millimeters(4)},
      .width = ccad::millimeters(0.15)
  });

  // Add board text
  board.texts.push_back(ccad::BoardText{
      .id = "BT1",
      .layer_id = "F.SilkS",
      .text = "Bridge rectifier",
      .position = ccad::Point{.x = ccad::millimeters(8), .y = ccad::millimeters(22)},
      .rotation_degrees = 90.0,
      .size = ccad::Size{.width = ccad::millimeters(1.5),
                         .height = ccad::millimeters(1.5)}
  });

  // Add a copper zone
  board.zones.push_back(ccad::BoardZone{
      .id = "Z_GND",
      .name = "GND copper",
      .net_id = "GND",
      .layer_ids = {"F.Cu"},
      .outline = {ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(2)},
                  ccad::Point{.x = ccad::millimeters(40), .y = ccad::millimeters(2)},
                  ccad::Point{.x = ccad::millimeters(40), .y = ccad::millimeters(30)},
                  ccad::Point{.x = ccad::millimeters(2), .y = ccad::millimeters(30)}},
      .priority = 1,
      .clearance = ccad::millimeters(0.2),
      .min_thickness = ccad::millimeters(0.25),
      .fill_enabled = true,
      .pad_connection = "thermal",
  });

  // Add a keepout
  board.keepouts.push_back(ccad::Keepout{
      .id = "keepout1",
      .kind = "copper",
      .area = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(50), .y = ccad::millimeters(50)},
          .size = ccad::Size{.width = ccad::millimeters(10), .height = ccad::millimeters(10)}
      }
  });

  // Add a placement region
  board.placement_regions.push_back(ccad::PlacementRegion{
      .id = "region1",
      .kind = "allow_all",
      .area = ccad::Rect{
          .origin = ccad::Point{.x = ccad::millimeters(5), .y = ccad::millimeters(5)},
          .size = ccad::Size{.width = ccad::millimeters(20), .height = ccad::millimeters(20)}
      }
  });

  project.boards.clear(); project.boards.push_back(board);

  std::string exported = ccad::exportToKiCadPcb(project);
  std::cout << exported << "\n";

  // Assertions to verify correct content in output
  require(exported.find("(kicad_pcb") != std::string::npos, "Output must contain kicad_pcb root element");
  require(exported.find("(version 20211014)") != std::string::npos, "Output must contain version 20211014");
  require(exported.find("(generator ccad)") != std::string::npos, "Output must contain generator ccad");
  require(exported.find("Edge.Cuts") != std::string::npos, "Output must contain Edge.Cuts layer");
  require(exported.find("(44 \"Edge.Cuts\" user)") != std::string::npos,
          "Edge.Cuts uses KiCad canonical layer number 44");
  require(exported.find("(45 \"Margin\" user)") != std::string::npos,
          "Margin uses KiCad canonical layer number 45");
  require(exported.find("(46 \"B.CrtYd\" user \"B.Courtyard\")") != std::string::npos,
          "B.CrtYd uses KiCad canonical layer number 46");
  require(exported.find("(47 \"F.CrtYd\" user \"F.Courtyard\")") != std::string::npos,
          "F.CrtYd uses KiCad canonical layer number 47");
  require(exported.find("(58 \"User.9\" user)") != std::string::npos,
          "User.9 uses KiCad canonical layer number 58");
  require(exported.find("(net 1 \"GND\")") != std::string::npos, "Output must declare GND net");
  require(exported.find("(net 2 \"VCC\")") != std::string::npos, "Output must declare VCC net");

  ccad::Project board_only = project;
  board_only.schematics.clear();
  const std::string board_only_exported = ccad::exportToKiCadPcb(board_only);
  require(board_only_exported.find("(net 1 \"GND\")") != std::string::npos,
          "Board-only KiCad export declares GND from board copper");
  require(board_only_exported.find("(net 2 \"VCC\")") != std::string::npos,
          "Board-only KiCad export declares VCC from board copper");
  require(board_only_exported.find("footprint \"Component\"") != std::string::npos,
          "Board-only KiCad export uses generic footprint value without schematic component");
  require(board_only_exported.find("(net 1 \"GND\")") <
              board_only_exported.find("(pad \"1\" smd rect"),
          "Board-only KiCad export declares nets before pads");

  // Pad details
  require(exported.find("footprint \"NE555\"") != std::string::npos, "Output must contain footprint for NE555");
  require(exported.find("property \"Reference\" \"U1\"") != std::string::npos, "Output must contain Reference U1");
  require(exported.find("(pad \"1\" smd rect (at 10.000000 20.000000 45.000000)") != std::string::npos, "Pad coordinates/rotation match");
  require(exported.find("(size 1.500000 2.000000)") != std::string::npos, "Pad size matches");
  require(exported.find("(pad \"2\" smd roundrect (at 12.000000 20.000000)") != std::string::npos, "Roundrect pad shape exports");
  require(exported.find("(roundrect_rratio 0.250000)") != std::string::npos, "Roundrect ratio exports");
  require(exported.find("(pad \"3\" smd chamfered_rect (at 14.000000 20.000000)") != std::string::npos, "Chamfered pad shape exports");
  require(exported.find("(chamfer_ratio 0.200000)") != std::string::npos, "Chamfer ratio exports");

  // Via details
  require(exported.find("(via (at 30.000000 40.000000) (size 0.800000) (drill 0.400000)") != std::string::npos, "Via details match");

  // Track details
  require(exported.find("(segment (start 10.000000 20.000000) (end 30.000000 40.000000) (width 0.250000)") != std::string::npos, "Track details match");

  // Board graphics and text details
  require(exported.find("(gr_line (start 3.000000 4.000000) (end 16.000000 4.000000) (stroke (width 0.150000) (type solid)) (layer \"Dwgs.User\"))") != std::string::npos, "Board graphic line exports");
  require(exported.find("(gr_text \"Bridge rectifier\" (at 8.000000 22.000000 90.000000) (layer \"F.SilkS\")") != std::string::npos, "Board text exports");
  require(exported.find("(effects (font (size 1.500000 1.500000) (thickness 0.150000)))") != std::string::npos, "Board text effects export");

  // Zone details
  require(exported.find("(zone (net 1) (net_name \"GND\") (layer \"F.Cu\")") != std::string::npos, "Board zone exports net and layer");
  require(exported.find("(name \"GND copper\")") != std::string::npos, "Board zone name exports");
  require(exported.find("(priority 1)") != std::string::npos, "Board zone priority exports");
  require(exported.find("(connect_pads (clearance 0.200000))") != std::string::npos, "Board zone pad connection exports");
  require(exported.find("(min_thickness 0.250000)") != std::string::npos, "Board zone min thickness exports");
  require(exported.find("(polygon (pts (xy 2.000000 2.000000) (xy 40.000000 2.000000) (xy 40.000000 30.000000) (xy 2.000000 30.000000)))") != std::string::npos, "Board zone outline exports");
  require(exported.find("(filled_polygon (layer \"F.Cu\") (pts (xy 2.000000 2.000000) (xy 40.000000 2.000000) (xy 40.000000 30.000000) (xy 2.000000 30.000000)))") != std::string::npos, "Board zone filled preview exports");

  // Keepout details
  require(exported.find("(keepout (tracks not_allowed) (vias not_allowed) (pads not_allowed) (copperareas not_allowed))") != std::string::npos, "Keepout rules match");

  // Placement region details
  require(exported.find("(gr_rect (start 5.000000 5.000000) (end 25.000000 25.000000) (stroke (width 0.1) (type solid)) (layer \"Dwgs.User\"))") != std::string::npos, "Placement region matches");

  std::cout << "All KiCad PCB export tests passed!\n";
  return 0;
}
