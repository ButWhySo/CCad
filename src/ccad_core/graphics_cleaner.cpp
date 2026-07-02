#include "ccad_core/graphics_cleaner.hpp"
#include <cmath>
#include <algorithm>
#include <map>
#include <set>

namespace ccad {

GraphicsCleaner::GraphicsCleaner(Board& board, int epsilon_nm)
    : board_(board), epsilon_(epsilon_nm) {}

std::vector<CleanupAction> GraphicsCleaner::cleanupBoard(bool dry_run, bool merge_rects, bool delete_redundant, bool merge_pads) {
  (void)merge_pads;
  std::vector<CleanupAction> actions;

  if (delete_redundant) {
    cleanupShapes(dry_run, actions);
  }

  if (merge_rects) {
    mergeRects(dry_run, actions);
  }
  
  // Note: mergePads requires spatial knowledge and pad complex operations, 
  // which might be integrated directly into pad tool in CCad. For now, 
  // we just handle redundant shapes and line-to-rect merging.

  return actions;
}

static bool pointsEquivalent(const Point& a, const Point& b, int epsilon) {
  return std::abs(a.x.nanometers - b.x.nanometers) <= epsilon &&
         std::abs(a.y.nanometers - b.y.nanometers) <= epsilon;
}

bool GraphicsCleaner::isNullShape(const BoardGraphic& graphic) const {
  if (graphic.kind == "segment" || graphic.kind == "rectangle" || graphic.kind == "arc") {
    return pointsEquivalent(graphic.start, graphic.end, epsilon_);
  } else if (graphic.kind == "circle") {
    // Circle radius 0
    return pointsEquivalent(graphic.start, graphic.end, epsilon_);
  }
  return false;
}

bool GraphicsCleaner::areEquivalent(const BoardGraphic& a, const BoardGraphic& b) const {
  if (a.kind != b.kind || a.layer_id != b.layer_id || a.width.nanometers != b.width.nanometers) {
    return false;
  }

  if (a.kind == "segment" || a.kind == "rectangle" || a.kind == "circle") {
    return (pointsEquivalent(a.start, b.start, epsilon_) && pointsEquivalent(a.end, b.end, epsilon_)) ||
           (pointsEquivalent(a.start, b.end, epsilon_) && pointsEquivalent(a.end, b.start, epsilon_));
  } else if (a.kind == "arc") {
    // Check center, start, end
    bool c = (!a.mid.has_value() && !b.mid.has_value()) || 
             (a.mid.has_value() && b.mid.has_value() && pointsEquivalent(a.mid.value(), b.mid.value(), epsilon_));
    return c && pointsEquivalent(a.start, b.start, epsilon_) && pointsEquivalent(a.end, b.end, epsilon_);
  }
  return false;
}

void GraphicsCleaner::cleanupShapes(bool dry_run, std::vector<CleanupAction>& actions) {
  std::set<std::string> deleted_ids;

  // 1. Remove null shapes
  for (const auto& g : board_.graphics) {
    if (deleted_ids.count(g.id)) continue;

    if (isNullShape(g)) {
      actions.push_back({CleanupActionCode::null_graphic, {g.id}});
      deleted_ids.insert(g.id);
    }
  }

  // 2. Remove duplicate shapes
  for (std::size_t i = 0; i < board_.graphics.size(); ++i) {
    const auto& g1 = board_.graphics[i];
    if (deleted_ids.count(g1.id)) continue;

    for (std::size_t j = i + 1; j < board_.graphics.size(); ++j) {
      const auto& g2 = board_.graphics[j];
      if (deleted_ids.count(g2.id)) continue;

      if (areEquivalent(g1, g2)) {
        actions.push_back({CleanupActionCode::duplicate_graphic, {g2.id}});
        deleted_ids.insert(g2.id);
      }
    }
  }

  if (!dry_run && !deleted_ids.empty()) {
    std::vector<BoardGraphic> new_graphics;
    for (const auto& g : board_.graphics) {
      if (deleted_ids.find(g.id) == deleted_ids.end()) {
        new_graphics.push_back(g);
      }
    }
    board_.graphics = std::move(new_graphics);
  }
}

void GraphicsCleaner::mergeRects(bool dry_run, std::vector<CleanupAction>& actions) {
  std::vector<BoardGraphic> new_rects;
  // Line to rect
  struct Side {
    Point start;
    Point end;
    const BoardGraphic* shape;
    
    Side(const BoardGraphic* g) : start(g->start), end(g->end), shape(g) {
      // canonicalize
      if (start.x.nanometers > end.x.nanometers || (start.x.nanometers == end.x.nanometers && start.y.nanometers > end.y.nanometers)) {
        std::swap(start, end);
      }
    }
  };

  std::vector<Side> sides;
  std::map<std::pair<int64_t, int64_t>, std::vector<std::size_t>> ptMap;

  for (const auto& g : board_.graphics) {
    if (g.kind != "segment" || isNullShape(g)) continue;

    if (g.start.x.nanometers == g.end.x.nanometers || g.start.y.nanometers == g.end.y.nanometers) {
      sides.emplace_back(&g);
      std::pair<int64_t, int64_t> pt = {sides.back().start.x.nanometers, sides.back().start.y.nanometers};
      ptMap[pt].push_back(sides.size() - 1);
    }
  }

  std::set<std::string> deleted_ids;

  for (std::size_t i = 0; i < sides.size(); ++i) {
    if (deleted_ids.count(sides[i].shape->id)) continue;

    const Side& side = sides[i];
    const Side* left = nullptr;
    const Side* top = nullptr;
    const Side* right = nullptr;
    const Side* bottom = nullptr;

    auto viable = [&](const Side& cand) {
      return cand.shape->layer_id == side.shape->layer_id &&
             cand.shape->width.nanometers == side.shape->width.nanometers &&
             !deleted_ids.count(cand.shape->id);
    };

    if (side.start.x.nanometers == side.end.x.nanometers) {
      left = &side;
      std::pair<int64_t, int64_t> pt = {left->start.x.nanometers, left->start.y.nanometers};
      for (std::size_t c_idx : ptMap[pt]) {
        if (c_idx != i && viable(sides[c_idx])) {
          top = &sides[c_idx];
          break;
        }
      }
    } else if (side.start.y.nanometers == side.end.y.nanometers) {
      top = &side;
      std::pair<int64_t, int64_t> pt = {top->start.x.nanometers, top->start.y.nanometers};
      for (std::size_t c_idx : ptMap[pt]) {
        if (c_idx != i && viable(sides[c_idx])) {
          left = &sides[c_idx];
          break;
        }
      }
    }

    if (top && left) {
      std::pair<int64_t, int64_t> topEnd = {top->end.x.nanometers, top->end.y.nanometers};
      for (std::size_t c_idx : ptMap[topEnd]) {
        if (&sides[c_idx] != top && &sides[c_idx] != left && viable(sides[c_idx])) {
          right = &sides[c_idx];
          break;
        }
      }

      std::pair<int64_t, int64_t> leftEnd = {left->end.x.nanometers, left->end.y.nanometers};
      for (std::size_t c_idx : ptMap[leftEnd]) {
        if (&sides[c_idx] != top && &sides[c_idx] != left && viable(sides[c_idx])) {
          bottom = &sides[c_idx];
          break;
        }
      }

      if (right && bottom && pointsEquivalent(right->end, bottom->end, epsilon_)) {
        deleted_ids.insert(left->shape->id);
        deleted_ids.insert(top->shape->id);
        deleted_ids.insert(right->shape->id);
        deleted_ids.insert(bottom->shape->id);

        actions.push_back({CleanupActionCode::lines_to_rect, {left->shape->id, top->shape->id, right->shape->id, bottom->shape->id}});

        if (!dry_run) {
          BoardGraphic rect;
          rect.id = "rect_" + left->shape->id; // New ID
          rect.kind = "rectangle";
          rect.layer_id = top->shape->layer_id;
          rect.start = top->start;
          rect.end = bottom->end;
          rect.width = top->shape->width;
          new_rects.push_back(rect);
        }
      }
    }
  }

  if (!dry_run && !deleted_ids.empty()) {
    std::vector<BoardGraphic> new_graphics;
    for (const auto& g : board_.graphics) {
      if (deleted_ids.find(g.id) == deleted_ids.end()) {
        new_graphics.push_back(g);
      }
    }
    board_.graphics = std::move(new_graphics);
    board_.graphics.insert(board_.graphics.end(), new_rects.begin(), new_rects.end());
  }
}

}  // namespace ccad
