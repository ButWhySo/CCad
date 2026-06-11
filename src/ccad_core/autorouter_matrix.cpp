#include "ccad_core/autorouter_matrix.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace ccad {
namespace {

std::int64_t floorToGrid(std::int64_t value, std::int64_t grid) {
  std::int64_t quotient = value / grid;
  const std::int64_t remainder = value % grid;
  if (remainder != 0 && value < 0) {
    --quotient;
  }
  return quotient * grid;
}

std::int64_t ceilToGrid(std::int64_t value, std::int64_t grid) {
  const std::int64_t floored = floorToGrid(value, grid);
  return floored == value ? value : floored + grid;
}

std::int64_t ceilDiv(std::int64_t value, std::int64_t divisor) {
  if (value <= 0) {
    return value / divisor;
  }
  return (value + divisor - 1) / divisor;
}

std::int64_t floorDiv(std::int64_t value, std::int64_t divisor) {
  std::int64_t quotient = value / divisor;
  const std::int64_t remainder = value % divisor;
  if (remainder != 0 && value < 0) {
    --quotient;
  }
  return quotient;
}

int sideIndex(AutorouterSide side) {
  return side == AutorouterSide::Top ? 1 : 0;
}

std::int64_t rectRight(const Rect& rect) {
  return rect.origin.x.nanometers + rect.size.width.nanometers;
}

std::int64_t rectBottom(const Rect& rect) {
  return rect.origin.y.nanometers + rect.size.height.nanometers;
}

bool pointInsideRect(std::int64_t x, std::int64_t y, const Rect& rect) {
  return x >= rect.origin.x.nanometers && x <= rectRight(rect) &&
         y >= rect.origin.y.nanometers && y <= rectBottom(rect);
}

std::int64_t chebyshevDistanceToRect(std::int64_t x, std::int64_t y, const Rect& rect) {
  const std::int64_t dx =
      std::max<std::int64_t>({rect.origin.x.nanometers - x, x - rectRight(rect), 0});
  const std::int64_t dy =
      std::max<std::int64_t>({rect.origin.y.nanometers - y, y - rectBottom(rect), 0});
  return std::max(dx, dy);
}

}  // namespace

void AutorouterMatrix::configure(const Rect& bounding_box, Length grid_step,
                                 int routing_layer_count) {
  if (grid_step.nanometers <= 0) {
    throw std::invalid_argument("autorouter matrix grid step must be positive");
  }
  if (bounding_box.size.width.nanometers <= 0 || bounding_box.size.height.nanometers <= 0) {
    throw std::invalid_argument("autorouter matrix bounding box must be positive");
  }
  if (routing_layer_count <= 0 || routing_layer_count > 2) {
    throw std::invalid_argument("autorouter matrix supports one or two routing layers");
  }

  grid_step_ = grid_step;
  routing_layer_count_ = routing_layer_count;

  const std::int64_t grid = grid_step_.nanometers;
  const std::int64_t start_x = floorToGrid(bounding_box.origin.x.nanometers, grid);
  const std::int64_t start_y = floorToGrid(bounding_box.origin.y.nanometers, grid);
  const std::int64_t end_x = ceilToGrid(rectRight(bounding_box), grid) + grid;
  const std::int64_t end_y = ceilToGrid(rectBottom(bounding_box), grid) + grid;

  origin_ = Point{.x = nanometers(start_x), .y = nanometers(start_y)};
  columns_ = static_cast<int>((end_x - start_x) / grid) + 1;
  rows_ = static_cast<int>((end_y - start_y) / grid) + 1;

  const std::size_t size = static_cast<std::size_t>(rows_) * static_cast<std::size_t>(columns_);
  for (int i = 0; i < 2; ++i) {
    cells_[i].assign(size, 0);
    distances_[i].assign(size, 0);
  }
}

int AutorouterMatrix::rows() const {
  return rows_;
}

int AutorouterMatrix::columns() const {
  return columns_;
}

Point AutorouterMatrix::origin() const {
  return origin_;
}

Length AutorouterMatrix::gridStep() const {
  return grid_step_;
}

AutorouterMatrix::MatrixCell AutorouterMatrix::cell(int row, int column,
                                                    AutorouterSide side) const {
  return cells_[sideIndex(side)][checkedIndex(row, column, side)];
}

void AutorouterMatrix::writeCell(int row, int column, AutorouterSide side, MatrixCell value,
                                 AutorouterCellOperation operation) {
  MatrixCell& target = cells_[sideIndex(side)][checkedIndex(row, column, side)];
  switch (operation) {
    case AutorouterCellOperation::Write:
      target = value;
      break;
    case AutorouterCellOperation::Or:
      target = static_cast<MatrixCell>(target | value);
      break;
    case AutorouterCellOperation::Xor:
      target = static_cast<MatrixCell>(target ^ value);
      break;
    case AutorouterCellOperation::And:
      target = static_cast<MatrixCell>(target & value);
      break;
    case AutorouterCellOperation::Add:
      target = static_cast<MatrixCell>(target + value);
      break;
  }
}

AutorouterMatrix::DistanceCell AutorouterMatrix::distance(int row, int column,
                                                          AutorouterSide side) const {
  return distances_[sideIndex(side)][checkedIndex(row, column, side)];
}

void AutorouterMatrix::setDistance(int row, int column, AutorouterSide side, DistanceCell value) {
  distances_[sideIndex(side)][checkedIndex(row, column, side)] = value;
}

void AutorouterMatrix::addDistance(int row, int column, AutorouterSide side, DistanceCell value) {
  distances_[sideIndex(side)][checkedIndex(row, column, side)] += value;
}

void AutorouterMatrix::traceFilledRectangle(const Rect& area,
                                            const std::vector<AutorouterSide>& sides,
                                            MatrixCell value,
                                            AutorouterCellOperation operation) {
  if (rows_ <= 0 || columns_ <= 0 || sides.empty()) {
    return;
  }

  const auto [first_row, last_row] =
      clippedRowRange(area.origin.y, nanometers(rectBottom(area)));
  const auto [first_column, last_column] =
      clippedColumnRange(area.origin.x, nanometers(rectRight(area)));

  if (first_row > last_row || first_column > last_column) {
    return;
  }

  for (int row = first_row; row <= last_row; ++row) {
    for (int column = first_column; column <= last_column; ++column) {
      for (AutorouterSide side : sides) {
        writeCell(row, column, side, value, operation);
      }
    }
  }
}

void AutorouterMatrix::createKeepoutCostRectangle(const Rect& area, Length margin,
                                                  DistanceCell cost,
                                                  const std::vector<AutorouterSide>& sides) {
  if (rows_ <= 0 || columns_ <= 0 || sides.empty() || cost <= 0) {
    return;
  }

  const std::int64_t margin_nm = std::max<std::int64_t>(0, margin.nanometers);
  const Length expanded_left = nanometers(area.origin.x.nanometers - margin_nm);
  const Length expanded_top = nanometers(area.origin.y.nanometers - margin_nm);
  const Length expanded_right = nanometers(rectRight(area) + margin_nm);
  const Length expanded_bottom = nanometers(rectBottom(area) + margin_nm);

  const auto [first_row, last_row] = clippedRowRange(expanded_top, expanded_bottom);
  const auto [first_column, last_column] = clippedColumnRange(expanded_left, expanded_right);

  if (first_row > last_row || first_column > last_column) {
    return;
  }

  const std::int64_t taper_span = margin_nm + grid_step_.nanometers;
  for (int row = first_row; row <= last_row; ++row) {
    const std::int64_t y = origin_.y.nanometers + static_cast<std::int64_t>(row) *
                                                       grid_step_.nanometers;
    for (int column = first_column; column <= last_column; ++column) {
      const std::int64_t x = origin_.x.nanometers + static_cast<std::int64_t>(column) *
                                                         grid_step_.nanometers;

      DistanceCell local_cost = cost;
      if (!pointInsideRect(x, y, area) && taper_span > 0) {
        const std::int64_t distance_to_core = chebyshevDistanceToRect(x, y, area);
        if (distance_to_core > taper_span) {
          continue;
        }
        local_cost = static_cast<DistanceCell>(
            std::ceil(static_cast<double>(cost) *
                      static_cast<double>(taper_span - distance_to_core) /
                      static_cast<double>(taper_span)));
      }

      for (AutorouterSide side : sides) {
        addDistance(row, column, side, local_cost);
      }
    }
  }
}

bool AutorouterMatrix::hasAnyCellInRectangle(const Rect& area,
                                             const std::vector<AutorouterSide>& sides) const {
  if (rows_ <= 0 || columns_ <= 0 || sides.empty()) {
    return false;
  }

  const auto [first_row, last_row] =
      clippedRowRange(area.origin.y, nanometers(rectBottom(area)));
  const auto [first_column, last_column] =
      clippedColumnRange(area.origin.x, nanometers(rectRight(area)));

  if (first_row > last_row || first_column > last_column) {
    return false;
  }

  for (int row = first_row; row <= last_row; ++row) {
    for (int column = first_column; column <= last_column; ++column) {
      for (AutorouterSide side : sides) {
        if (cell(row, column, side) != 0) {
          return true;
        }
      }
    }
  }
  return false;
}

AutorouterMatrix::DistanceCell AutorouterMatrix::distanceCostInRectangle(
    const Rect& area, const std::vector<AutorouterSide>& sides) const {
  if (rows_ <= 0 || columns_ <= 0 || sides.empty()) {
    return 0;
  }

  const auto [first_row, last_row] =
      clippedRowRange(area.origin.y, nanometers(rectBottom(area)));
  const auto [first_column, last_column] =
      clippedColumnRange(area.origin.x, nanometers(rectRight(area)));

  if (first_row > last_row || first_column > last_column) {
    return 0;
  }

  DistanceCell total = 0;
  for (int row = first_row; row <= last_row; ++row) {
    for (int column = first_column; column <= last_column; ++column) {
      for (AutorouterSide side : sides) {
        total += distance(row, column, side);
      }
    }
  }
  return total;
}

std::size_t AutorouterMatrix::checkedIndex(int row, int column, AutorouterSide side) const {
  if (routing_layer_count_ <= 0) {
    throw std::logic_error("autorouter matrix is not configured");
  }
  if (row < 0 || row >= rows_ || column < 0 || column >= columns_) {
    throw std::out_of_range("autorouter matrix cell is outside configured bounds");
  }
  if (routing_layer_count_ == 1 && side == AutorouterSide::Top) {
    throw std::out_of_range("autorouter matrix top side is not enabled");
  }
  return static_cast<std::size_t>(row) * static_cast<std::size_t>(columns_) +
         static_cast<std::size_t>(column);
}

std::pair<int, int> AutorouterMatrix::clippedColumnRange(Length left, Length right) const {
  const std::int64_t grid = grid_step_.nanometers;
  const std::int64_t local_left = left.nanometers - origin_.x.nanometers;
  const std::int64_t local_right = right.nanometers - origin_.x.nanometers;
  const int first = static_cast<int>(std::max<std::int64_t>(0, ceilDiv(local_left, grid)));
  const int last =
      static_cast<int>(std::min<std::int64_t>(columns_ - 1, floorDiv(local_right, grid)));
  return {first, last};
}

std::pair<int, int> AutorouterMatrix::clippedRowRange(Length top, Length bottom) const {
  const std::int64_t grid = grid_step_.nanometers;
  const std::int64_t local_top = top.nanometers - origin_.y.nanometers;
  const std::int64_t local_bottom = bottom.nanometers - origin_.y.nanometers;
  const int first = static_cast<int>(std::max<std::int64_t>(0, ceilDiv(local_top, grid)));
  const int last =
      static_cast<int>(std::min<std::int64_t>(rows_ - 1, floorDiv(local_bottom, grid)));
  return {first, last};
}

}  // namespace ccad
