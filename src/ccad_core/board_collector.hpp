#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace ccad {

enum class BoardCollectorScanSet {
  all_board_items,
  board_level_items,
  footprints,
  pads_or_tracks,
  footprint_items,
  tracks,
  dimensions,
  draggable_items,
};

struct BoardCollectorGuide {
  std::string preferred_layer_id = "F.Cu";
  std::vector<std::string> visible_layer_ids;
  bool include_secondary = true;
  bool ignore_locked_items = false;
  bool ignore_tracks = false;
  bool ignore_zone_fills = true;
  bool ignore_no_nets = false;
};

struct BoardCollectorCandidate {
  std::string type;
  std::string id;
  std::string kicad_type;
  std::string collection_bucket;
  std::string net_id;
  std::string primary_layer_id;
  std::vector<std::string> layer_ids;
  bool current_layer_match = false;
  bool visible_layer_match = false;
  bool locked = false;
};

struct BoardCollectorReport {
  std::string kicad_collector = "GENERAL_COLLECTOR";
  std::string parity_scope = "collector_scan_set_layer_first_slice";
  std::string scan_set;
  std::vector<std::string> kicad_scan_types;
  std::vector<std::string> unsupported_kicad_types;
  std::vector<BoardCollectorCandidate> candidates;
  int primary_count = 0;
  int secondary_count = 0;
};

std::vector<std::string> boardCollectorScanTypes(BoardCollectorScanSet scan_set);
BoardCollectorScanSet parseBoardCollectorScanSet(std::string_view name);
std::string boardCollectorScanSetName(BoardCollectorScanSet scan_set);
BoardCollectorReport collectBoardItems(const Board& board,
                                       BoardCollectorScanSet scan_set,
                                       const BoardCollectorGuide& guide);

}  // namespace ccad
