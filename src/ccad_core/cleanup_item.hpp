#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ccad {

enum class CleanupActionCode {
  shorting_track,
  shorting_via,
  redundant_via,
  duplicate_track,
  merge_tracks,
  dangling_track,
  dangling_via,
  zero_length_track,
  track_in_pad,
  null_graphic,
  duplicate_graphic,
  lines_to_rect,
  merge_pad,
};

struct CleanupActionInfo {
  CleanupActionCode code;
  int kicad_offset;
  std::string id;
  std::string domain;
  std::string title;
};

const std::vector<CleanupActionInfo>& cleanupActionCatalog();
const CleanupActionInfo* findCleanupActionInfo(CleanupActionCode code);
std::string cleanupActionTitle(CleanupActionCode code);

class CleanupActionProvider {
 public:
  explicit CleanupActionProvider(std::vector<CleanupActionInfo>& rows);

  std::size_t count() const;
  const CleanupActionInfo& item(std::size_t index) const;
  CleanupActionInfo& item(std::size_t index);
  void deleteItem(std::size_t index, bool deep);

 private:
  std::vector<CleanupActionInfo>* rows_;
};

}  // namespace ccad
