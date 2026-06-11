#pragma once

#include "ccad_core/geometry.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace ccad {

enum class AutorouterSide {
  Bottom = 0,
  Top = 1,
};

enum class AutorouterCellOperation {
  Write,
  Or,
  Xor,
  And,
  Add,
};

class AutorouterMatrix {
 public:
  using MatrixCell = std::uint8_t;
  using DistanceCell = int;

  void configure(const Rect& bounding_box, Length grid_step, int routing_layer_count);

  int rows() const;
  int columns() const;
  Point origin() const;
  Length gridStep() const;

  MatrixCell cell(int row, int column, AutorouterSide side) const;
  void writeCell(int row, int column, AutorouterSide side, MatrixCell value,
                 AutorouterCellOperation operation);

  DistanceCell distance(int row, int column, AutorouterSide side) const;
  void setDistance(int row, int column, AutorouterSide side, DistanceCell value);
  void addDistance(int row, int column, AutorouterSide side, DistanceCell value);

  void traceFilledRectangle(const Rect& area, const std::vector<AutorouterSide>& sides,
                            MatrixCell value, AutorouterCellOperation operation);
  void createKeepoutCostRectangle(const Rect& area, Length margin, DistanceCell cost,
                                  const std::vector<AutorouterSide>& sides);
  bool hasAnyCellInRectangle(const Rect& area, const std::vector<AutorouterSide>& sides) const;
  DistanceCell distanceCostInRectangle(const Rect& area,
                                       const std::vector<AutorouterSide>& sides) const;

 private:
  std::size_t checkedIndex(int row, int column, AutorouterSide side) const;
  std::pair<int, int> clippedColumnRange(Length left, Length right) const;
  std::pair<int, int> clippedRowRange(Length top, Length bottom) const;

  Point origin_;
  Length grid_step_;
  int rows_ = 0;
  int columns_ = 0;
  int routing_layer_count_ = 0;
  std::array<std::vector<MatrixCell>, 2> cells_;
  std::array<std::vector<DistanceCell>, 2> distances_;
};

}  // namespace ccad
