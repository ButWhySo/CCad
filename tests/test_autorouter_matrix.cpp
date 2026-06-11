#include "ccad_core/autorouter_matrix.hpp"

#include "ccad_core/geometry.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

ccad::Rect boardRect(double width_mm, double height_mm) {
  return ccad::Rect{.origin = {ccad::millimeters(0), ccad::millimeters(0)},
                    .size = {ccad::millimeters(width_mm), ccad::millimeters(height_mm)}};
}

void test_matrix_aligns_board_box_with_grid_margin() {
  ccad::AutorouterMatrix matrix;
  matrix.configure(boardRect(4, 3), ccad::millimeters(1), 2);

  require(matrix.rows() == 5, "matrix should include KiCad-style row margin");
  require(matrix.columns() == 6, "matrix should include KiCad-style column margin");
  require(matrix.origin().x.nanometers == 0, "matrix origin x should stay grid aligned");
  require(matrix.origin().y.nanometers == 0, "matrix origin y should stay grid aligned");
}

void test_cell_operations_are_side_local() {
  ccad::AutorouterMatrix matrix;
  matrix.configure(boardRect(2, 2), ccad::millimeters(1), 2);

  matrix.writeCell(1, 1, ccad::AutorouterSide::Bottom, 0x01,
                   ccad::AutorouterCellOperation::Write);
  matrix.writeCell(1, 1, ccad::AutorouterSide::Bottom, 0x02,
                   ccad::AutorouterCellOperation::Or);
  matrix.writeCell(1, 1, ccad::AutorouterSide::Top, 0x04, ccad::AutorouterCellOperation::Write);
  matrix.writeCell(1, 1, ccad::AutorouterSide::Top, 0x01, ccad::AutorouterCellOperation::Xor);

  require(matrix.cell(1, 1, ccad::AutorouterSide::Bottom) == 0x03,
          "bottom side should accumulate OR writes independently");
  require(matrix.cell(1, 1, ccad::AutorouterSide::Top) == 0x05,
          "top side should accumulate XOR writes independently");

  matrix.writeCell(1, 1, ccad::AutorouterSide::Bottom, 0x01,
                   ccad::AutorouterCellOperation::And);
  require(matrix.cell(1, 1, ccad::AutorouterSide::Bottom) == 0x01,
          "AND operation should mask the bottom side cell");
}

void test_trace_filled_rectangle_clips_to_selected_sides() {
  ccad::AutorouterMatrix matrix;
  matrix.configure(boardRect(5, 5), ccad::millimeters(1), 2);

  const ccad::Rect area{.origin = {ccad::millimeters(1), ccad::millimeters(1)},
                        .size = {ccad::millimeters(2), ccad::millimeters(1)}};
  matrix.traceFilledRectangle(area, {ccad::AutorouterSide::Top}, 0x08,
                              ccad::AutorouterCellOperation::Write);

  require(matrix.cell(1, 1, ccad::AutorouterSide::Top) == 0x08,
          "selected top cells should be written");
  require(matrix.cell(1, 3, ccad::AutorouterSide::Top) == 0x08,
          "rectangle tracing should include its grid-aligned end cell");
  require(matrix.cell(1, 1, ccad::AutorouterSide::Bottom) == 0,
          "bottom side should remain untouched when only top is selected");
  require(matrix.cell(0, 0, ccad::AutorouterSide::Top) == 0,
          "cells outside the rectangle should remain empty");
}

void test_keepout_cost_is_written_to_distance_map() {
  ccad::AutorouterMatrix matrix;
  matrix.configure(boardRect(6, 6), ccad::millimeters(1), 2);

  const ccad::Rect area{.origin = {ccad::millimeters(2), ccad::millimeters(2)},
                        .size = {ccad::millimeters(1), ccad::millimeters(1)}};
  matrix.createKeepoutCostRectangle(area, ccad::millimeters(1), 120,
                                    {ccad::AutorouterSide::Bottom});

  require(matrix.distance(2, 2, ccad::AutorouterSide::Bottom) >= 120,
          "keepout core should receive at least the requested cost");
  require(matrix.distance(1, 1, ccad::AutorouterSide::Bottom) > 0,
          "keepout margin should receive a non-zero tapered cost");
  require(matrix.distance(2, 2, ccad::AutorouterSide::Top) == 0,
          "unselected side should not receive distance cost");
}

void test_rectangle_queries_read_cells_and_distance_costs() {
  ccad::AutorouterMatrix matrix;
  matrix.configure(boardRect(6, 6), ccad::millimeters(1), 2);

  const ccad::Rect occupied{.origin = {ccad::millimeters(2), ccad::millimeters(2)},
                            .size = {ccad::millimeters(1), ccad::millimeters(1)}};
  matrix.traceFilledRectangle(occupied, {ccad::AutorouterSide::Top}, 0x20,
                              ccad::AutorouterCellOperation::Write);
  matrix.createKeepoutCostRectangle(occupied, ccad::millimeters(1), 90,
                                    {ccad::AutorouterSide::Top});

  require(matrix.hasAnyCellInRectangle(occupied, {ccad::AutorouterSide::Top}),
          "rectangle query should report occupied cells on selected side");
  require(!matrix.hasAnyCellInRectangle(occupied, {ccad::AutorouterSide::Bottom}),
          "rectangle query should ignore unselected side");
  require(matrix.distanceCostInRectangle(occupied, {ccad::AutorouterSide::Top}) >= 90,
          "distance query should sum selected-side keepout costs");
}

}  // namespace

int main() {
  try {
    test_matrix_aligns_board_box_with_grid_margin();
    test_cell_operations_are_side_local();
    test_trace_filled_rectangle_clips_to_selected_sides();
    test_keepout_cost_is_written_to_distance_map();
    test_rectangle_queries_read_cells_and_distance_costs();
    std::cout << "All tests passed!\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "Test failed: " << error.what() << "\n";
    return 1;
  }
}
