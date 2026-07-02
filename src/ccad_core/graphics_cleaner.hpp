#pragma once

#include "ccad_core/model.hpp"
#include "ccad_core/cleanup_item.hpp"
#include <vector>
#include <memory>

namespace ccad {

struct CleanupAction {
  CleanupActionCode code;
  std::vector<std::string> item_ids;
};

class GraphicsCleaner {
public:
  GraphicsCleaner(Board& board, int epsilon_nm = 1000);

  // Run the cleanup sequence and return generated actions.
  // In dry_run mode, the board is not modified.
  std::vector<CleanupAction> cleanupBoard(bool dry_run, bool merge_rects = true, bool delete_redundant = true, bool merge_pads = true);

private:
  bool isNullShape(const BoardGraphic& graphic) const;
  bool areEquivalent(const BoardGraphic& a, const BoardGraphic& b) const;

  void cleanupShapes(bool dry_run, std::vector<CleanupAction>& actions);
  void mergeRects(bool dry_run, std::vector<CleanupAction>& actions);

  Board& board_;
  int epsilon_;
};

}  // namespace ccad
