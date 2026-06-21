#pragma once

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <string>
#include <vector>

namespace ccad {

enum class BoardStackupItemType {
  undefined,
  copper,
  dielectric,
  solder_paste,
  solder_mask,
  silkscreen,
};

struct BoardStackupItem {
  BoardStackupItemType type = BoardStackupItemType::undefined;
  std::string layer_id;
  std::string layer_name;
  std::string type_name;
  int dielectric_layer_id = 0;
  Length thickness = nanometers(0);
  bool thickness_locked = false;
  std::string material;
  double epsilon_r = 0.0;
  double loss_tangent = 0.0;
  double spec_frequency_hz = 0.0;
  std::string dielectric_model = "constant";
  std::string color;
  bool enabled = true;
};

struct BoardStackup {
  std::string kicad_class = "BOARD_STACKUP";
  std::string parity_scope = "default_stackup_first_slice";
  std::string finish_type = "None";
  bool has_dielectric_constraints = false;
  bool has_thickness_constraints = false;
  bool edge_plating = false;
  std::string edge_connector = "none";
  std::vector<BoardStackupItem> items;
};

Length defaultCopperThickness();
Length defaultSolderMaskThickness();
BoardStackup buildDefaultBoardStackup(const Board& board);
Length buildBoardThicknessFromStackup(const BoardStackup& stackup);
Length boardStackupLayerDistance(const BoardStackup& stackup,
                                 const std::string& first_layer_id,
                                 const std::string& second_layer_id);
std::string boardStackupItemTypeName(BoardStackupItemType type);

}  // namespace ccad
