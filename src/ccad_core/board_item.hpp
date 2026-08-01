#pragma once

#include "ccad_core/model.hpp"

#include <string>
#include <vector>

namespace ccad {

struct BoardItemMetadata {
  std::string kicad_base_class = "BOARD_ITEM";
  bool kicad_groupable = false;
  std::string primary_layer_id;
  std::vector<std::string> layer_ids;
  std::string layer_mask_description;
  bool side_specific = false;
  bool is_on_copper_layer = false;
  bool has_hole = false;
  bool has_drilled_hole = false;
  bool locked = false;
  bool knockout = false;
  std::vector<std::string> view_layer_ids;
  std::string parity_scope = "board_item_first_slice";
};

BoardItemMetadata boardItemMetadata(const Board& board, const Pad& pad);
BoardItemMetadata boardItemMetadata(const Board& board, const Via& via);
BoardItemMetadata boardItemMetadata(const Board& board, const TrackSegment& track);
BoardItemMetadata boardItemMetadata(const Board& board, const TrackArc& arc);
BoardItemMetadata boardItemMetadata(const Board& board, const BoardGraphic& graphic);
BoardItemMetadata boardItemMetadata(const Board& board, const BoardText& text);
BoardItemMetadata boardItemMetadata(const Board& board, const BoardZone& zone);
BoardItemMetadata boardItemMetadata(const Board& board, const BoardTeardrop& teardrop);
BoardItemMetadata boardItemMetadata(const Board& board, const std::vector<std::string>& layer_ids,
                                    bool groupable, bool has_hole, bool has_drilled_hole,
                                    bool locked = false, bool knockout = false);

}  // namespace ccad
