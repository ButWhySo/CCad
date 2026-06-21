#include "ccad_core/board_item_container.hpp"

#include <stdexcept>

namespace ccad {
namespace {

template <typename T>
std::optional<BoardContainerItemRef> findInItems(const std::vector<T>& items,
                                                 const std::string_view id,
                                                 const BoardContainerItemKind kind,
                                                 const bool kicad_board_item) {
  for (std::size_t index = 0; index < items.size(); ++index) {
    if (items.at(index).id == id) {
      return BoardContainerItemRef{.id = std::string(id),
                                   .kind = kind,
                                   .index = index,
                                   .kicad_board_item = kicad_board_item};
    }
  }
  return std::nullopt;
}

template <typename T>
BoardContainerRemoveResult removeFromItems(std::vector<T>& items, const std::string_view id,
                                           const BoardContainerItemKind kind,
                                           const bool kicad_board_item,
                                           const BoardContainerRemoveMode mode) {
  for (std::size_t index = 0; index < items.size(); ++index) {
    if (items.at(index).id == id) {
      items.erase(items.begin() + static_cast<std::ptrdiff_t>(index));
      return BoardContainerRemoveResult{.removed = true,
                                        .id = std::string(id),
                                        .kind = kind,
                                        .index = index,
                                        .mode = mode,
                                        .kicad_delete_semantics = kicad_board_item};
    }
  }
  return BoardContainerRemoveResult{.removed = false,
                                    .id = std::string(id),
                                    .kind = BoardContainerItemKind::unknown,
                                    .index = 0,
                                    .mode = mode,
                                    .kicad_delete_semantics = false};
}

}  // namespace

std::string boardContainerAddModeName(const BoardContainerAddMode mode) {
  switch (mode) {
    case BoardContainerAddMode::insert:
      return "insert";
    case BoardContainerAddMode::append:
      return "append";
    case BoardContainerAddMode::bulk_append:
      return "bulk_append";
    case BoardContainerAddMode::bulk_insert:
      return "bulk_insert";
  }
  return "unknown";
}

std::string boardContainerRemoveModeName(const BoardContainerRemoveMode mode) {
  switch (mode) {
    case BoardContainerRemoveMode::normal:
      return "normal";
    case BoardContainerRemoveMode::bulk:
      return "bulk";
  }
  return "unknown";
}

std::string boardContainerItemKindName(const BoardContainerItemKind kind) {
  switch (kind) {
    case BoardContainerItemKind::pad:
      return "pad";
    case BoardContainerItemKind::via:
      return "via";
    case BoardContainerItemKind::track:
      return "track";
    case BoardContainerItemKind::graphic:
      return "graphic";
    case BoardContainerItemKind::text:
      return "text";
    case BoardContainerItemKind::zone:
      return "zone";
    case BoardContainerItemKind::keepout:
      return "keepout";
    case BoardContainerItemKind::placement_region:
      return "placement_region";
    case BoardContainerItemKind::unknown:
      return "unknown";
  }
  return "unknown";
}

BoardItemContainerSummary summarizeBoardItemContainer(const Board& board) {
  BoardItemContainerSummary summary;
  summary.add_modes = {boardContainerAddModeName(BoardContainerAddMode::insert),
                       boardContainerAddModeName(BoardContainerAddMode::append),
                       boardContainerAddModeName(BoardContainerAddMode::bulk_append),
                       boardContainerAddModeName(BoardContainerAddMode::bulk_insert)};
  summary.remove_modes = {boardContainerRemoveModeName(BoardContainerRemoveMode::normal),
                          boardContainerRemoveModeName(BoardContainerRemoveMode::bulk)};
  summary.board_item_count = board.pads.size() + board.vias.size() + board.tracks.size() +
                             board.graphics.size() + board.texts.size() + board.zones.size();
  summary.constraint_item_count = board.keepouts.size() + board.placement_regions.size();
  summary.total_item_count = summary.board_item_count + summary.constraint_item_count;
  return summary;
}

std::optional<BoardContainerItemRef> findBoardContainerItem(const Board& board,
                                                            const std::string_view id) {
  if (auto ref = findInItems(board.pads, id, BoardContainerItemKind::pad, true)) {
    return ref;
  }
  if (auto ref = findInItems(board.vias, id, BoardContainerItemKind::via, true)) {
    return ref;
  }
  if (auto ref = findInItems(board.tracks, id, BoardContainerItemKind::track, true)) {
    return ref;
  }
  if (auto ref = findInItems(board.graphics, id, BoardContainerItemKind::graphic, true)) {
    return ref;
  }
  if (auto ref = findInItems(board.texts, id, BoardContainerItemKind::text, true)) {
    return ref;
  }
  if (auto ref = findInItems(board.zones, id, BoardContainerItemKind::zone, true)) {
    return ref;
  }
  if (auto ref = findInItems(board.keepouts, id, BoardContainerItemKind::keepout, false)) {
    return ref;
  }
  if (auto ref = findInItems(board.placement_regions, id,
                             BoardContainerItemKind::placement_region, false)) {
    return ref;
  }
  return std::nullopt;
}

bool hasBoardContainerItemId(const Board& board, const std::string_view id) {
  return findBoardContainerItem(board, id).has_value();
}

void requireUniqueBoardContainerItemId(const Board& board, const std::string_view id) {
  if (hasBoardContainerItemId(board, id)) {
    throw std::runtime_error("duplicate physical object id: " + std::string(id));
  }
}

BoardContainerRemoveResult removeBoardContainerItem(Board& board, const std::string_view id,
                                                    const BoardContainerRemoveMode mode) {
  if (auto result = removeFromItems(board.pads, id, BoardContainerItemKind::pad, true, mode);
      result.removed) {
    return result;
  }
  if (auto result = removeFromItems(board.vias, id, BoardContainerItemKind::via, true, mode);
      result.removed) {
    return result;
  }
  if (auto result =
          removeFromItems(board.tracks, id, BoardContainerItemKind::track, true, mode);
      result.removed) {
    return result;
  }
  if (auto result =
          removeFromItems(board.graphics, id, BoardContainerItemKind::graphic, true, mode);
      result.removed) {
    return result;
  }
  if (auto result = removeFromItems(board.texts, id, BoardContainerItemKind::text, true, mode);
      result.removed) {
    return result;
  }
  if (auto result = removeFromItems(board.zones, id, BoardContainerItemKind::zone, true, mode);
      result.removed) {
    return result;
  }
  if (auto result =
          removeFromItems(board.keepouts, id, BoardContainerItemKind::keepout, false, mode);
      result.removed) {
    return result;
  }
  if (auto result = removeFromItems(board.placement_regions, id,
                                    BoardContainerItemKind::placement_region, false, mode);
      result.removed) {
    return result;
  }
  return BoardContainerRemoveResult{.removed = false,
                                    .id = std::string(id),
                                    .kind = BoardContainerItemKind::unknown,
                                    .index = 0,
                                    .mode = mode,
                                    .kicad_delete_semantics = false};
}

}  // namespace ccad
