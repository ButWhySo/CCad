#pragma once

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ccad {


enum class DrillLineSource {
  via,
  pad,
};

enum class DrillLineColumn {
  count,
  shape,
  x_size,
  y_size,
  plated,
  source,
  start_layer,
  stop_layer,
};

struct DrillLineItem {
  Length x_size;
  Length y_size;
  DrillShape shape = DrillShape::Circle;
  bool plated = false;
  DrillLineSource source = DrillLineSource::pad;
  std::string start_layer_id;
  std::string stop_layer_id;
  int quantity = 0;

  bool operator==(const DrillLineItem& other) const;
};

struct DrillLineItemCompare {
  DrillLineItemCompare(DrillLineColumn column, bool ascending);

  bool operator()(const DrillLineItem& left, const DrillLineItem& right) const;

  DrillLineColumn column;
  bool ascending;
};

std::vector<DrillLineItem> collectDrillLineItems(const Board& board);

struct BoardStatisticsObjectCounts {
  int pad_count = 0;
  int through_hole_pad_count = 0;
  int smd_pad_count = 0;
  int connector_pad_count = 0;
  int npth_pad_count = 0;
  int via_count = 0;
  int track_count = 0;
  int zone_count = 0;
  int graphic_count = 0;
  int text_count = 0;
  int keepout_count = 0;
  int placement_region_count = 0;
};

struct BoardStatisticsReport {
  std::string kicad_reference = "board_statistics_report";
  std::string parity_scope = "summary_report_first_slice";
  std::string project_name;
  std::string board_name;
  bool has_outline = false;
  Length board_width;
  Length board_height;
  double board_area_square_mm = 0.0;
  double front_copper_area_square_mm = 0.0;
  double back_copper_area_square_mm = 0.0;
  double front_footprint_area_square_mm = 0.0;
  double back_footprint_area_square_mm = 0.0;
  double front_footprint_density_percent = 0.0;
  double back_footprint_density_percent = 0.0;
  std::optional<Length> min_track_width;
  std::optional<Length> min_drill_diameter;
  Length board_thickness;
  BoardStatisticsObjectCounts object_counts;
  std::vector<DrillLineItem> drill_holes;
};

BoardStatisticsReport buildBoardStatisticsReport(const Board& board,
                                                 std::string project_name = "",
                                                 std::string board_name = "");

}  // namespace ccad
