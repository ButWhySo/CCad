#include "ccad_core/cleanup_item.hpp"

#include <stdexcept>

namespace ccad {
namespace {

const std::vector<CleanupActionInfo>& catalogStorage() {
  static const std::vector<CleanupActionInfo> actions{
      CleanupActionInfo{.code = CleanupActionCode::shorting_track,
                        .kicad_offset = 0,
                        .id = "shorting_track",
                        .domain = "tracks_and_vias",
                        .title = "Remove track shorting two nets"},
      CleanupActionInfo{.code = CleanupActionCode::shorting_via,
                        .kicad_offset = 1,
                        .id = "shorting_via",
                        .domain = "tracks_and_vias",
                        .title = "Remove via shorting two nets"},
      CleanupActionInfo{.code = CleanupActionCode::redundant_via,
                        .kicad_offset = 2,
                        .id = "redundant_via",
                        .domain = "tracks_and_vias",
                        .title = "Remove redundant via"},
      CleanupActionInfo{.code = CleanupActionCode::duplicate_track,
                        .kicad_offset = 3,
                        .id = "duplicate_track",
                        .domain = "tracks_and_vias",
                        .title = "Remove duplicate track"},
      CleanupActionInfo{.code = CleanupActionCode::merge_tracks,
                        .kicad_offset = 4,
                        .id = "merge_tracks",
                        .domain = "tracks_and_vias",
                        .title = "Merge co-linear tracks"},
      CleanupActionInfo{.code = CleanupActionCode::dangling_track,
                        .kicad_offset = 5,
                        .id = "dangling_track",
                        .domain = "tracks_and_vias",
                        .title = "Remove track not connected at both ends"},
      CleanupActionInfo{.code = CleanupActionCode::dangling_via,
                        .kicad_offset = 6,
                        .id = "dangling_via",
                        .domain = "tracks_and_vias",
                        .title = "Remove via connected on less than 2 layers"},
      CleanupActionInfo{.code = CleanupActionCode::zero_length_track,
                        .kicad_offset = 7,
                        .id = "zero_length_track",
                        .domain = "tracks_and_vias",
                        .title = "Remove zero-length track"},
      CleanupActionInfo{.code = CleanupActionCode::track_in_pad,
                        .kicad_offset = 8,
                        .id = "track_in_pad",
                        .domain = "tracks_and_vias",
                        .title = "Remove track inside pad"},
      CleanupActionInfo{.code = CleanupActionCode::null_graphic,
                        .kicad_offset = 9,
                        .id = "null_graphic",
                        .domain = "graphics",
                        .title = "Remove zero-size graphic"},
      CleanupActionInfo{.code = CleanupActionCode::duplicate_graphic,
                        .kicad_offset = 10,
                        .id = "duplicate_graphic",
                        .domain = "graphics",
                        .title = "Remove duplicated graphic"},
      CleanupActionInfo{.code = CleanupActionCode::lines_to_rect,
                        .kicad_offset = 11,
                        .id = "lines_to_rect",
                        .domain = "graphics",
                        .title = "Convert lines to rectangle"},
      CleanupActionInfo{.code = CleanupActionCode::merge_pad,
                        .kicad_offset = 12,
                        .id = "merge_pad",
                        .domain = "graphics",
                        .title = "Merge overlapping shapes into pad"},
  };
  return actions;
}

}  // namespace

const std::vector<CleanupActionInfo>& cleanupActionCatalog() {
  return catalogStorage();
}

const CleanupActionInfo* findCleanupActionInfo(CleanupActionCode code) {
  for (const CleanupActionInfo& action : cleanupActionCatalog()) {
    if (action.code == code) {
      return &action;
    }
  }
  return nullptr;
}

std::string cleanupActionTitle(CleanupActionCode code) {
  const CleanupActionInfo* action = findCleanupActionInfo(code);
  if (action == nullptr) {
    return "Unknown cleanup action";
  }
  return action->title;
}

CleanupActionProvider::CleanupActionProvider(std::vector<CleanupActionInfo>& rows)
    : rows_(&rows) {}

std::size_t CleanupActionProvider::count() const {
  return rows_->size();
}

const CleanupActionInfo& CleanupActionProvider::item(std::size_t index) const {
  return rows_->at(index);
}

CleanupActionInfo& CleanupActionProvider::item(std::size_t index) {
  return rows_->at(index);
}

void CleanupActionProvider::deleteItem(std::size_t index, bool deep) {
  if (!deep) {
    (void) rows_->at(index);
    return;
  }
  rows_->erase(rows_->begin() + static_cast<std::ptrdiff_t>(index));
}

}  // namespace ccad
