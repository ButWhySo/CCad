#include "ccad_core/board_statistics.hpp"

#include "ccad_core/geometry.hpp"
#include "ccad_core/layers.hpp"
#include "ccad_core/model.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

void require(const bool condition, const std::string& message) {
  if (!condition) {
    std::cerr << "test failure: " << message << '\n';
    std::exit(1);
  }
}

ccad::Board drillFixtureBoard() {
  ccad::Board board;
  board.outline = ccad::Rect{.origin = {.x = ccad::nanometers(0), .y = ccad::nanometers(0)},
                             .size = {.width = ccad::millimeters(30),
                                      .height = ccad::millimeters(20)}};
  board.layers = ccad::standardKiCadPcbLayers();
  board.pads.push_back(ccad::Pad{.id = "J1.1",
                                 .component_id = "J1",
                                 .pin_name = "1",
                                 .net_id = "N1",
                                 .type = "thru_hole",
                                 .position = {.x = ccad::millimeters(4),
                                              .y = ccad::millimeters(4)},
                                 .padstack = ccad::Padstack{
                                    .layer_set = {"*.Cu", "*.Mask"},
                                    .copper_props = {{"*", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Circle, .size = {.width = ccad::millimeters(1.6), .height = ccad::millimeters(1.6)}}}}},
                                    .drill = ccad::PadstackDrillProps{.size = ccad::millimeters(0.8)}
                                 }});
  board.pads.push_back(ccad::Pad{.id = "J1.2",
                                 .component_id = "J1",
                                 .pin_name = "2",
                                 .net_id = "N2",
                                 .type = "thru_hole",
                                 .position = {.x = ccad::millimeters(6),
                                              .y = ccad::millimeters(4)},
                                 .padstack = ccad::Padstack{
                                    .layer_set = {"*.Cu", "*.Mask"},
                                    .copper_props = {{"*", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Circle, .size = {.width = ccad::millimeters(1.6), .height = ccad::millimeters(1.6)}}}}},
                                    .drill = ccad::PadstackDrillProps{.size = ccad::millimeters(0.8)}
                                 }});
  board.pads.push_back(ccad::Pad{.id = "MH1",
                                 .component_id = "MH1",
                                 .pin_name = "",
                                 .net_id = "",
                                 .type = "np_thru_hole",
                                 .position = {.x = ccad::millimeters(10),
                                              .y = ccad::millimeters(10)},
                                 .padstack = ccad::Padstack{
                                    .layer_set = {"F.Mask", "B.Mask"},
                                    .copper_props = {{"*", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Circle, .size = {.width = ccad::millimeters(2.0), .height = ccad::millimeters(2.0)}}}}},
                                    .drill = ccad::PadstackDrillProps{.size = ccad::millimeters(1.1)}
                                 }});
  board.pads.push_back(ccad::Pad{.id = "SMD1",
                                 .component_id = "U1",
                                 .pin_name = "1",
                                 .net_id = "N3",
                                 .type = "smd",
                                 .position = {.x = ccad::millimeters(12),
                                              .y = ccad::millimeters(4)},
                                 .padstack = ccad::Padstack{
                                    .layer_set = {"F.Cu", "F.Paste", "F.Mask"},
                                    .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = {.width = ccad::millimeters(1.2), .height = ccad::millimeters(0.8)}}}}}
                                 }});
  board.vias.push_back(ccad::Via{.id = "V1",
                                 .net_id = "N1",
                                 .position = {.x = ccad::millimeters(8),
                                              .y = ccad::millimeters(8)},
                                 .diameter = ccad::millimeters(0.8),
                                 .drill = ccad::millimeters(0.4)});
  board.tracks.push_back(ccad::TrackSegment{.id = "T1",
                                            .net_id = "N1",
                                            .layer_id = "F.Cu",
                                            .start = {.x = ccad::millimeters(4),
                                                      .y = ccad::millimeters(4)},
                                            .end = {.x = ccad::millimeters(8),
                                                    .y = ccad::millimeters(8)},
                                            .width = ccad::millimeters(0.25)});
  board.tracks.push_back(ccad::TrackSegment{.id = "T2",
                                            .net_id = "N2",
                                            .layer_id = "B.Cu",
                                            .start = {.x = ccad::millimeters(6),
                                                      .y = ccad::millimeters(4)},
                                            .end = {.x = ccad::millimeters(10),
                                                    .y = ccad::millimeters(10)},
                                            .width = ccad::millimeters(0.18)});
  return board;
}

const ccad::DrillLineItem* findDrill(const std::vector<ccad::DrillLineItem>& drills,
                                     const std::int64_t drill_nm,
                                     const ccad::DrillLineSource source,
                                     const bool plated) {
  const auto it =
      std::find_if(drills.begin(), drills.end(), [&](const ccad::DrillLineItem& item) {
        return item.x_size.nanometers == drill_nm && item.y_size.nanometers == drill_nm &&
               item.source == source && item.plated == plated;
      });
  return it == drills.end() ? nullptr : &*it;
}

void test_collect_drill_line_items_groups_like_kicad() {
  const ccad::Board board = drillFixtureBoard();
  const std::vector<ccad::DrillLineItem> drills = ccad::collectDrillLineItems(board);

  require(drills.size() == 3, "collects unique pad/via drill rows and ignores SMD pads");

  const ccad::DrillLineItem* pth =
      findDrill(drills, 800000, ccad::DrillLineSource::pad, true);
  require(pth != nullptr, "collects plated through-hole pad drill row");
  require(pth->quantity == 2, "aggregates identical plated through-hole pad drills");
  require(pth->shape == ccad::DrillShape::Circle, "through-hole pad drill is round");
  require(pth->start_layer_id == "F.Cu", "through-hole pad starts on top copper");
  require(pth->stop_layer_id == "B.Cu", "through-hole pad stops on bottom copper");

  const ccad::DrillLineItem* npth =
      findDrill(drills, 1100000, ccad::DrillLineSource::pad, false);
  require(npth != nullptr, "collects non-plated through-hole pad drill row");
  require(npth->quantity == 1, "counts non-plated through-hole drill once");
  require(npth->start_layer_id.empty(), "non-copper NPTH pad does not invent a start layer");
  require(npth->stop_layer_id.empty(), "non-copper NPTH pad does not invent a stop layer");

  const ccad::DrillLineItem* via =
      findDrill(drills, 400000, ccad::DrillLineSource::via, true);
  require(via != nullptr, "collects plated via drill row");
  require(via->quantity == 1, "counts via drill once");
  require(via->start_layer_id == "F.Cu", "via drill starts on top copper");
  require(via->stop_layer_id == "B.Cu", "via drill stops on bottom copper");
}

void test_drill_line_compare_is_strict_for_boolean_columns() {
  ccad::DrillLineItem plated{.x_size = ccad::millimeters(0.8),
                             .y_size = ccad::millimeters(0.8),
                             .shape = ccad::DrillShape::Circle,
                             .plated = true,
                             .source = ccad::DrillLineSource::pad,
                             .start_layer_id = "F.Cu",
                             .stop_layer_id = "B.Cu",
                             .quantity = 1};
  ccad::DrillLineItem unplated = plated;
  unplated.plated = false;
  ccad::DrillLineItem via = plated;
  via.source = ccad::DrillLineSource::via;

  ccad::DrillLineItemCompare plated_ascending(ccad::DrillLineColumn::plated, true);
  require(!plated_ascending(plated, plated), "plated comparator is irreflexive for true");
  require(!plated_ascending(unplated, unplated), "plated comparator is irreflexive for false");
  require(plated_ascending(unplated, plated), "ascending plated comparator orders false first");
  require(!plated_ascending(plated, unplated), "ascending plated comparator is asymmetric");

  std::vector<ccad::DrillLineItem> plated_items{plated, unplated, plated, unplated};
  std::sort(plated_items.begin(), plated_items.end(), plated_ascending);
  require(!plated_items.at(0).plated && !plated_items.at(1).plated,
          "ascending plated sort groups false rows first");

  ccad::DrillLineItemCompare source_ascending(ccad::DrillLineColumn::source, true);
  require(!source_ascending(plated, plated), "source comparator is irreflexive for pad");
  require(!source_ascending(via, via), "source comparator is irreflexive for via");
  require(source_ascending(via, plated), "ascending source comparator orders via before pad");
  require(!source_ascending(plated, via), "ascending source comparator is asymmetric");
}

void test_build_board_statistics_report_summarizes_kicad_report_fields() {
  const ccad::Board board = drillFixtureBoard();
  const ccad::BoardStatisticsReport report =
      ccad::buildBoardStatisticsReport(board, "proj-report", "board-report");

  require(report.kicad_reference == "board_statistics_report",
          "board statistics report records KiCad report reference");
  require(report.parity_scope == "summary_report_first_slice",
          "board statistics report declares first-slice scope");
  require(report.project_name == "proj-report", "board statistics report preserves project name");
  require(report.board_name == "board-report", "board statistics report preserves board name");
  require(report.has_outline, "board statistics report detects rectangular outline");
  require(report.board_width.nanometers == 30000000, "board statistics report writes board width");
  require(report.board_height.nanometers == 20000000, "board statistics report writes board height");
  require(report.board_area_square_mm == 600.0,
          "board statistics report computes rectangular board area in mm2");
  require(report.object_counts.pad_count == 4, "board statistics report counts pads");
  require(report.object_counts.through_hole_pad_count == 2,
          "board statistics report counts plated through-hole pads");
  require(report.object_counts.smd_pad_count == 1, "board statistics report counts SMD pads");
  require(report.object_counts.npth_pad_count == 1, "board statistics report counts NPTH pads");
  require(report.object_counts.via_count == 1, "board statistics report counts vias");
  require(report.object_counts.track_count == 2, "board statistics report counts tracks");
  require(report.min_track_width.has_value(), "board statistics report has minimum track width");
  require(report.min_track_width->nanometers == 180000,
          "board statistics report computes minimum track width");
  require(report.min_drill_diameter.has_value(), "board statistics report has minimum drill size");
  require(report.min_drill_diameter->nanometers == 400000,
          "board statistics report computes minimum drill size from pads and vias");
  require(report.board_thickness.nanometers == 1600000,
          "board statistics report carries board stackup thickness");
  require(report.drill_holes.size() == 3, "board statistics report reuses drill rows");
}

}  // namespace

int main() {
  test_collect_drill_line_items_groups_like_kicad();
  test_drill_line_compare_is_strict_for_boolean_columns();
  test_build_board_statistics_report_summarizes_kicad_report_fields();
  return 0;
}
