#include "ccad_core/spread_footprints.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace ccad {
namespace {

struct ComponentGroup {
  std::string component_id;
  std::vector<std::size_t> pad_indices;
  Rect bounds;
};

struct RefParts {
  std::string prefix;
  int number = std::numeric_limits<int>::max();
  std::string suffix;
};

RefParts splitReference(const std::string& reference) {
  std::size_t digit_start = reference.size();
  while (digit_start > 0 &&
         std::isdigit(static_cast<unsigned char>(reference.at(digit_start - 1)))) {
    --digit_start;
  }

  RefParts parts;
  parts.prefix = reference.substr(0, digit_start);
  if (digit_start < reference.size()) {
    parts.number = std::stoi(reference.substr(digit_start));
  }
  parts.suffix = reference.substr(digit_start);
  return parts;
}

bool naturalReferenceLess(const std::string& a, const std::string& b) {
  const RefParts left = splitReference(a);
  const RefParts right = splitReference(b);
  if (left.prefix != right.prefix) {
    return left.prefix < right.prefix;
  }
  if (left.number != right.number) {
    return left.number < right.number;
  }
  return left.suffix < right.suffix;
}

Rect padBounds(const Pad& pad) {
  const Size psize = pad.padstack.copper_props.empty() ? ccad::Size{} : pad.padstack.copper_props.begin()->second.shape.size;
  const std::int64_t half_width = psize.width.nanometers / 2;
  const std::int64_t half_height = psize.height.nanometers / 2;
  return Rect{.origin = {.x = nanometers(pad.position.x.nanometers - half_width),
                         .y = nanometers(pad.position.y.nanometers - half_height)},
              .size = {.width = psize.width, .height = psize.height}};
}

Rect mergeBounds(const Rect& a, const Rect& b) {
  const std::int64_t left = std::min(a.origin.x.nanometers, b.origin.x.nanometers);
  const std::int64_t top = std::min(a.origin.y.nanometers, b.origin.y.nanometers);
  const std::int64_t right =
      std::max(a.origin.x.nanometers + a.size.width.nanometers,
               b.origin.x.nanometers + b.size.width.nanometers);
  const std::int64_t bottom =
      std::max(a.origin.y.nanometers + a.size.height.nanometers,
               b.origin.y.nanometers + b.size.height.nanometers);
  return Rect{.origin = {.x = nanometers(left), .y = nanometers(top)},
              .size = {.width = nanometers(right - left), .height = nanometers(bottom - top)}};
}

Rect componentBounds(const Board& board, const std::vector<std::size_t>& pad_indices) {
  if (pad_indices.empty()) {
    throw std::runtime_error("cannot spread a component with no pads");
  }

  Rect bounds = padBounds(board.pads.at(pad_indices.front()));
  for (std::size_t index = 1; index < pad_indices.size(); ++index) {
    bounds = mergeBounds(bounds, padBounds(board.pads.at(pad_indices.at(index))));
  }
  return bounds;
}

std::vector<ComponentGroup> collectGroups(const Board& board,
                                          const std::vector<std::string>& component_ids) {
  const std::set<std::string> filter(component_ids.begin(), component_ids.end());
  std::map<std::string, ComponentGroup> groups;

  for (std::size_t index = 0; index < board.pads.size(); ++index) {
    const Pad& pad = board.pads.at(index);
    if (!filter.empty() && !filter.contains(pad.component_id)) {
      continue;
    }
    ComponentGroup& group = groups[pad.component_id];
    group.component_id = pad.component_id;
    group.pad_indices.push_back(index);
  }

  std::vector<ComponentGroup> result;
  for (auto& [component_id, group] : groups) {
    group.bounds = componentBounds(board, group.pad_indices);
    result.push_back(group);
  }

  std::sort(result.begin(), result.end(), [](const ComponentGroup& a, const ComponentGroup& b) {
    return naturalReferenceLess(a.component_id, b.component_id);
  });
  return result;
}

Rect movedBounds(const Rect& bounds, Length delta_x, Length delta_y) {
  return Rect{.origin = {.x = nanometers(bounds.origin.x.nanometers + delta_x.nanometers),
                         .y = nanometers(bounds.origin.y.nanometers + delta_y.nanometers)},
              .size = bounds.size};
}

}  // namespace

std::vector<SpreadFootprintPlacement> spreadFootprintComponents(
    Board& board, const SpreadFootprintRequest& request) {
  if (request.component_gap.nanometers < 0 || request.group_gap.nanometers < 0) {
    throw std::invalid_argument("spread footprint gaps must be non-negative");
  }

  std::vector<ComponentGroup> groups = collectGroups(board, request.component_ids);
  std::vector<SpreadFootprintPlacement> placements;
  placements.reserve(groups.size());

  std::int64_t cursor_x = request.target.x.nanometers;
  std::int64_t cursor_y = request.target.y.nanometers;
  const std::int64_t start_x = cursor_x;

  double total_area = 0;
  for (const auto& group : groups) {
    total_area += static_cast<double>(group.bounds.size.width.nanometers + request.component_gap.nanometers) * 
                  static_cast<double>(group.bounds.size.height.nanometers + request.component_gap.nanometers);
  }
  const std::int64_t grid_side_limit = static_cast<std::int64_t>(std::sqrt(total_area));
  std::int64_t current_row_height = 0;

  for (const ComponentGroup& group : groups) {
    if (cursor_x - start_x > grid_side_limit && cursor_x > start_x) {
      cursor_x = start_x;
      cursor_y += current_row_height + request.component_gap.nanometers;
      current_row_height = 0;
    }

    const std::int64_t delta_x = cursor_x - group.bounds.origin.x.nanometers;
    const std::int64_t delta_y = cursor_y - group.bounds.origin.y.nanometers;

    for (std::size_t pad_index : group.pad_indices) {
      Pad& pad = board.pads.at(pad_index);
      pad.position.x = nanometers(pad.position.x.nanometers + delta_x);
      pad.position.y = nanometers(pad.position.y.nanometers + delta_y);
    }

    const Rect new_bounds = movedBounds(group.bounds, nanometers(delta_x), nanometers(delta_y));
    placements.push_back(SpreadFootprintPlacement{.component_id = group.component_id,
                                                  .previous_bounds = group.bounds,
                                                  .new_bounds = new_bounds});
    cursor_x += group.bounds.size.width.nanometers + request.component_gap.nanometers;
    if (group.bounds.size.height.nanometers > current_row_height) {
      current_row_height = group.bounds.size.height.nanometers;
    }
  }

  return placements;
}

}  // namespace ccad
