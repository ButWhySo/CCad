#pragma once

#include "ccad_core/model.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ccad {

struct BoardOutlinePolygonOptions {
  bool infer_outline_if_necessary = false;
};

struct BoardOutlinePolygonReport {
  std::string kicad_source = "convert_shape_list_to_polygon";
  std::string kicad_function = "ConvertOutlineToPolygon";
  std::string parity_scope = "edge_cuts_segment_chain_first_slice";
  std::size_t edge_cut_segment_count = 0;
  std::size_t outline_count = 0;
  std::size_t hole_count = 0;
  bool closed = false;
  bool valid = false;
  bool used_inferred_outline = false;
  bool allow_disjoint = false;
  bool allow_use_arcs_in_polygons = false;
  Rect bounding_box;
  std::vector<Point> points;
  std::vector<std::string> source_graphic_ids;
  std::vector<std::string> diagnostics;
  std::vector<std::string> pending_kicad_features;
};

BoardOutlinePolygonReport buildBoardOutlinePolygonReport(
    const Board& board,
    const BoardOutlinePolygonOptions& options = BoardOutlinePolygonOptions{});

}  // namespace ccad
