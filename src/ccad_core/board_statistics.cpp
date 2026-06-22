#include "ccad_core/board_statistics.hpp"

#include "ccad_core/layers.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <vector>

namespace ccad {
namespace {

bool isCopperLayer(const Board& board, const std::string& layer_id) {
  for (const Layer& layer : board.layers) {
    if (layer.id == layer_id) {
      return layer.kind == "copper";
    }
  }
  const Layer* standard_layer = findStandardKiCadPcbLayer(layer_id);
  return standard_layer != nullptr && standard_layer->kind == "copper";
}

std::vector<std::string> copperLayerIds(const Board& board) {
  std::vector<std::string> layers;
  for (const Layer& layer : board.layers) {
    if (layer.kind == "copper") {
      layers.push_back(layer.id);
    }
  }
  return layers;
}

std::vector<std::string> copperLayersFromPad(const Board& board, const Pad& pad) {
  const std::vector<std::string> resolved_layers = expandKiCadLayerSet(pad.padstack.layer_set, board);
  std::vector<std::string> copper_layers;
  for (const std::string& layer_id : resolved_layers) {
    if (isCopperLayer(board, layer_id) &&
        std::find(copper_layers.begin(), copper_layers.end(), layer_id) == copper_layers.end()) {
      copper_layers.push_back(layer_id);
    }
  }
  return copper_layers;
}

std::string lowerAscii(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

bool isNonPlatedPad(const Pad& pad) {
  const std::string type = lowerAscii(pad.type);
  return type == "np_thru_hole" || type == "npth" || type == "np_through_hole";
}

bool isSmdPad(const Pad& pad) {
  const std::string type = lowerAscii(pad.type);
  return type == "smd";
}

bool isConnectorPad(const Pad& pad) {
  const std::string type = lowerAscii(pad.type);
  return type == "conn" || type == "connector";
}

bool isThroughHolePad(const Pad& pad) {
  const std::string type = lowerAscii(pad.type);
  return type == "thru_hole" || type == "through_hole" || type == "pth";
}

DrillShape drillShapeForPad(const Pad& pad) {
  if (!pad.padstack.copper_props.empty() && 
      pad.padstack.copper_props.begin()->second.shape.shape == PadShape::Oval) {
    return DrillShape::Oval;
  }
  return DrillShape::Circle;
}

DrillLineItem padDrillLineItem(const Board& board, const Pad& pad) {
  const std::vector<std::string> copper_layers = copperLayersFromPad(board, pad);
  const Length drill = Length{pad.padstack.drill.size.width.nanometers};
  return DrillLineItem{
      .x_size = drill,
      .y_size = drill,
      .shape = drillShapeForPad(pad),
      .plated = !isNonPlatedPad(pad),
      .source = DrillLineSource::pad,
      .start_layer_id = copper_layers.empty() ? "" : copper_layers.front(),
      .stop_layer_id = copper_layers.empty() ? "" : copper_layers.back(),
      .quantity = 1,
  };
}

DrillLineItem viaDrillLineItem(const Board& board, const Via& via) {
  const std::vector<std::string> copper_layers = copperLayerIds(board);
  return DrillLineItem{
      .x_size = via.drill,
      .y_size = via.drill,
      .shape = DrillShape::Circle,
      .plated = true,
      .source = DrillLineSource::via,
      .start_layer_id = copper_layers.empty() ? "" : copper_layers.front(),
      .stop_layer_id = copper_layers.empty() ? "" : copper_layers.back(),
      .quantity = 1,
  };
}

void appendOrIncrement(std::vector<DrillLineItem>& items, const DrillLineItem& candidate) {
  for (DrillLineItem& item : items) {
    if (item == candidate) {
      ++item.quantity;
      return;
    }
  }
  items.push_back(candidate);
}

double squareMillimeters(const Size& size) {
  const double width_mm = static_cast<double>(size.width.nanometers) / 1000000.0;
  const double height_mm = static_cast<double>(size.height.nanometers) / 1000000.0;
  return width_mm * height_mm;
}

BoardStatisticsObjectCounts countBoardStatisticsObjects(const Board& board) {
  BoardStatisticsObjectCounts counts;
  counts.pad_count = static_cast<int>(board.pads.size());
  counts.via_count = static_cast<int>(board.vias.size());
  counts.track_count = static_cast<int>(board.tracks.size());
  counts.zone_count = static_cast<int>(board.zones.size());
  counts.graphic_count = static_cast<int>(board.graphics.size());
  counts.text_count = static_cast<int>(board.texts.size());
  counts.keepout_count = static_cast<int>(board.keepouts.size());
  counts.placement_region_count = static_cast<int>(board.placement_regions.size());

  for (const Pad& pad : board.pads) {
    if (isNonPlatedPad(pad)) {
      ++counts.npth_pad_count;
    } else if (isSmdPad(pad)) {
      ++counts.smd_pad_count;
    } else if (isConnectorPad(pad)) {
      ++counts.connector_pad_count;
    } else if (isThroughHolePad(pad)) {
      ++counts.through_hole_pad_count;
    }
  }
  return counts;
}

std::optional<Length> minimumTrackWidth(const Board& board) {
  std::int64_t minimum = std::numeric_limits<std::int64_t>::max();
  for (const TrackSegment& track : board.tracks) {
    if (track.width.nanometers > 0 && track.width.nanometers < minimum) {
      minimum = track.width.nanometers;
    }
  }
  if (minimum == std::numeric_limits<std::int64_t>::max()) {
    return std::nullopt;
  }
  return nanometers(minimum);
}

std::optional<Length> minimumDrillDiameter(const std::vector<DrillLineItem>& drill_holes) {
  std::int64_t minimum = std::numeric_limits<std::int64_t>::max();
  for (const DrillLineItem& drill : drill_holes) {
    if (drill.x_size.nanometers > 0 && drill.x_size.nanometers < minimum) {
      minimum = drill.x_size.nanometers;
    }
  }
  if (minimum == std::numeric_limits<std::int64_t>::max()) {
    return std::nullopt;
  }
  return nanometers(minimum);
}

template <typename T>
bool compareParameter(const T& left, const T& right, const bool ascending) {
  return ascending ? left < right : left > right;
}

}  // namespace

bool DrillLineItem::operator==(const DrillLineItem& other) const {
  return x_size.nanometers == other.x_size.nanometers &&
         y_size.nanometers == other.y_size.nanometers && shape == other.shape &&
         plated == other.plated && source == other.source &&
         start_layer_id == other.start_layer_id && stop_layer_id == other.stop_layer_id;
}

DrillLineItemCompare::DrillLineItemCompare(const DrillLineColumn column_value,
                                           const bool ascending_value)
    : column(column_value), ascending(ascending_value) {}

bool DrillLineItemCompare::operator()(const DrillLineItem& left,
                                      const DrillLineItem& right) const {
  switch (column) {
    case DrillLineColumn::count:
      return compareParameter(left.quantity, right.quantity, ascending);
    case DrillLineColumn::shape:
      return compareParameter(static_cast<int>(left.shape), static_cast<int>(right.shape),
                              ascending);
    case DrillLineColumn::x_size:
      return compareParameter(left.x_size.nanometers, right.x_size.nanometers, ascending);
    case DrillLineColumn::y_size:
      return compareParameter(left.y_size.nanometers, right.y_size.nanometers, ascending);
    case DrillLineColumn::plated:
      return compareParameter(left.plated, right.plated, ascending);
    case DrillLineColumn::source:
      return compareParameter(static_cast<int>(left.source), static_cast<int>(right.source),
                              ascending);
    case DrillLineColumn::start_layer:
      return compareParameter(left.start_layer_id, right.start_layer_id, ascending);
    case DrillLineColumn::stop_layer:
      return compareParameter(left.stop_layer_id, right.stop_layer_id, ascending);
  }
  return false;
}

std::vector<DrillLineItem> collectDrillLineItems(const Board& board) {
  std::vector<DrillLineItem> items;

  for (const Pad& pad : board.pads) {
    if (pad.padstack.drill.size.width.nanometers <= 0) {
      continue;
    }
    appendOrIncrement(items, padDrillLineItem(board, pad));
  }

  for (const Via& via : board.vias) {
    if (via.drill.nanometers <= 0) {
      continue;
    }
    appendOrIncrement(items, viaDrillLineItem(board, via));
  }

  return items;
}

BoardStatisticsReport buildBoardStatisticsReport(const Board& board,
                                                 std::string project_name,
                                                 std::string board_name) {
  BoardStatisticsReport report;
  report.project_name = std::move(project_name);
  report.board_name = std::move(board_name);
  report.has_outline =
      board.outline.size.width.nanometers > 0 && board.outline.size.height.nanometers > 0;
  if (report.has_outline) {
    report.board_width = board.outline.size.width;
    report.board_height = board.outline.size.height;
    report.board_area_square_mm = squareMillimeters(board.outline.size);
  }
  report.object_counts = countBoardStatisticsObjects(board);
  report.min_track_width = minimumTrackWidth(board);
  report.drill_holes = collectDrillLineItems(board);
  report.min_drill_diameter = minimumDrillDiameter(report.drill_holes);
  report.board_thickness = board.design_rules.board_thickness;
  return report;
}

}  // namespace ccad
