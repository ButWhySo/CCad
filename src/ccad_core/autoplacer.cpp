#include "ccad_core/autoplacer.hpp"

#include "ccad_core/autorouter_matrix.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace ccad {
namespace {

struct NmRect {
  std::int64_t left = 0;
  std::int64_t top = 0;
  std::int64_t right = 0;
  std::int64_t bottom = 0;
};

NmRect padRect(Point center, Size size) {
  const std::int64_t half_width = size.width.nanometers / 2;
  const std::int64_t half_height = size.height.nanometers / 2;
  return NmRect{.left = center.x.nanometers - half_width,
                .top = center.y.nanometers - half_height,
                .right = center.x.nanometers + half_width,
                .bottom = center.y.nanometers + half_height};
}

NmRect rectToNm(const Rect& rect) {
  return NmRect{.left = rect.origin.x.nanometers,
                .top = rect.origin.y.nanometers,
                .right = rect.origin.x.nanometers + rect.size.width.nanometers,
                .bottom = rect.origin.y.nanometers + rect.size.height.nanometers};
}

Rect nmToRect(const NmRect& rect) {
  return Rect{.origin = {.x = nanometers(rect.left), .y = nanometers(rect.top)},
              .size = {.width = nanometers(rect.right - rect.left),
                       .height = nanometers(rect.bottom - rect.top)}};
}

bool containsRect(const NmRect& outer, const NmRect& inner) {
  return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right &&
         inner.bottom <= outer.bottom;
}

bool overlaps(const NmRect& a, const NmRect& b) {
  return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top;
}

bool layerMatches(const std::vector<std::string>& layers, const std::string& layer_id) {
  return std::find(layers.begin(), layers.end(), layer_id) != layers.end();
}

Point translated(Point origin, Point offset) {
  return Point{.x = nanometers(origin.x.nanometers + offset.x.nanometers),
               .y = nanometers(origin.y.nanometers + offset.y.nanometers)};
}

double distanceNm(Point a, Point b) {
  const double dx = static_cast<double>(a.x.nanometers - b.x.nanometers);
  const double dy = static_cast<double>(a.y.nanometers - b.y.nanometers);
  return std::hypot(dx, dy);
}

AutorouterSide sideForLayer(const std::string& layer_id) {
  return layer_id == "B.Cu" ? AutorouterSide::Bottom : AutorouterSide::Top;
}

}  // namespace

AutoPlacementPlan planFootprintAutoPlacement(
    const Board& board, const Footprint& footprint,
    const std::map<std::string, std::string>& footprint_pad_nets, const std::string& layer_id,
    Length grid_step) {
  if (footprint.pads.empty()) {
    return AutoPlacementPlan{.placeable = false,
                             .origin = {},
                             .score = 0.0,
                             .reason = "footprint_has_no_pads"};
  }
  if (grid_step.nanometers <= 0) {
    return AutoPlacementPlan{.placeable = false,
                             .origin = {},
                             .score = 0.0,
                             .reason = "grid_step_must_be_positive"};
  }
  if (board.outline.size.width.nanometers <= 0 || board.outline.size.height.nanometers <= 0) {
    return AutoPlacementPlan{.placeable = false,
                             .origin = {},
                             .score = 0.0,
                             .reason = "board_outline_missing"};
  }

  const NmRect board_rect = rectToNm(board.outline);
  const std::vector<AutorouterSide> selected_sides = {sideForLayer(layer_id)};
  AutorouterMatrix placement_matrix;
  placement_matrix.configure(board.outline, grid_step, 2);

  for (const Pad& existing_pad : board.pads) {
    if (!layerMatches(existing_pad.padstack.layer_set, layer_id)) {
      continue;
    }
    const ccad::Size existing_size = existing_pad.padstack.copper_props.empty() ? ccad::Size{} : existing_pad.padstack.copper_props.begin()->second.shape.size;
    const Rect existing_area = nmToRect(padRect(existing_pad.position, existing_size));
    placement_matrix.traceFilledRectangle(existing_area, selected_sides, 0x02,
                                          AutorouterCellOperation::Or);
    placement_matrix.createKeepoutCostRectangle(existing_area, grid_step, 120, selected_sides);
  }

  for (const Keepout& keepout : board.keepouts) {
    if (keepout.kind != "placement") {
      continue;
    }
    placement_matrix.traceFilledRectangle(keepout.area, selected_sides, 0x80,
                                          AutorouterCellOperation::Or);
  }

  AutoPlacementPlan best;
  best.score = std::numeric_limits<double>::infinity();
  best.reason = "no_candidate";

  for (std::int64_t y = board_rect.top; y <= board_rect.bottom; y += grid_step.nanometers) {
    for (std::int64_t x = board_rect.left; x <= board_rect.right; x += grid_step.nanometers) {
      const Point origin{.x = nanometers(x), .y = nanometers(y)};
      bool rejected = false;
      double score = 0.0;

      for (const FootprintPad& footprint_pad : footprint.pads) {
        if (!footprint_pad.layers.empty() && !layerMatches(footprint_pad.layers, layer_id)) {
          continue;
        }

        const Point candidate_center = translated(origin, footprint_pad.position);
        const NmRect candidate_rect = padRect(candidate_center, footprint_pad.size);
        const Rect candidate_area = nmToRect(candidate_rect);
        if (!containsRect(board_rect, candidate_rect)) {
          rejected = true;
          break;
        }

        if (placement_matrix.hasAnyCellInRectangle(candidate_area, selected_sides)) {
          rejected = true;
          break;
        }

        score += static_cast<double>(
            placement_matrix.distanceCostInRectangle(candidate_area, selected_sides));

        for (const Pad& existing_pad : board.pads) {
          if (!layerMatches(existing_pad.padstack.layer_set, layer_id)) {
            continue;
          }
          const ccad::Size existing_size = existing_pad.padstack.copper_props.empty() ? ccad::Size{} : existing_pad.padstack.copper_props.begin()->second.shape.size;
          if (overlaps(candidate_rect, padRect(existing_pad.position, existing_size))) {
            rejected = true;
            break;
          }
        }
        if (rejected) {
          break;
        }

        for (const Keepout& keepout : board.keepouts) {
          if (keepout.kind != "placement") {
            continue;
          }
          if (overlaps(candidate_rect, rectToNm(keepout.area))) {
            rejected = true;
            break;
          }
        }
        if (rejected) {
          break;
        }

        const auto net_it = footprint_pad_nets.find(footprint_pad.number);
        if (net_it == footprint_pad_nets.end() || net_it->second.empty()) {
          continue;
        }

        double nearest = std::numeric_limits<double>::infinity();
        for (const Pad& existing_pad : board.pads) {
          if (existing_pad.net_id != net_it->second || !layerMatches(existing_pad.padstack.layer_set, layer_id)) {
            continue;
          }
          nearest = std::min(nearest, distanceNm(candidate_center, existing_pad.position));
        }
        if (std::isfinite(nearest)) {
          score += nearest;
        }
      }

      if (!rejected && (!best.placeable || score < best.score)) {
        best.placeable = true;
        best.origin = origin;
        best.score = score;
        best.reason = "candidate_selected";
      }
    }
  }

  return best;
}

}  // namespace ccad
