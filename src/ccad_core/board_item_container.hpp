#pragma once

#include "ccad_core/model.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ccad {

enum class BoardContainerAddMode {
  insert,
  append,
  bulk_append,
  bulk_insert,
};

enum class BoardContainerRemoveMode {
  normal,
  bulk,
};

enum class BoardContainerItemKind {
  unknown,
  pad,
  via,
  track,
  graphic,
  text,
  zone,
  keepout,
  placement_region,
};

struct BoardItemContainerSummary {
  std::string kicad_class = "BOARD_ITEM_CONTAINER";
  std::string container_kind = "BOARD";
  std::vector<std::string> add_modes;
  std::vector<std::string> remove_modes;
  bool delete_calls_remove = true;
  bool skip_connectivity_argument_supported = true;
  std::size_t board_item_count = 0;
  std::size_t constraint_item_count = 0;
  std::size_t total_item_count = 0;
};

struct BoardContainerItemRef {
  std::string id;
  BoardContainerItemKind kind = BoardContainerItemKind::unknown;
  std::size_t index = 0;
  bool kicad_board_item = false;
};

struct BoardContainerRemoveResult {
  bool removed = false;
  std::string id;
  BoardContainerItemKind kind = BoardContainerItemKind::unknown;
  std::size_t index = 0;
  BoardContainerRemoveMode mode = BoardContainerRemoveMode::normal;
  bool kicad_delete_semantics = true;
};

std::string boardContainerAddModeName(BoardContainerAddMode mode);
std::string boardContainerRemoveModeName(BoardContainerRemoveMode mode);
std::string boardContainerItemKindName(BoardContainerItemKind kind);

BoardItemContainerSummary summarizeBoardItemContainer(const Board& board);
std::optional<BoardContainerItemRef> findBoardContainerItem(const Board& board,
                                                            std::string_view id);
bool hasBoardContainerItemId(const Board& board, std::string_view id);
void requireUniqueBoardContainerItemId(const Board& board, std::string_view id);
BoardContainerRemoveResult removeBoardContainerItem(
    Board& board, std::string_view id,
    BoardContainerRemoveMode mode = BoardContainerRemoveMode::normal);

}  // namespace ccad
