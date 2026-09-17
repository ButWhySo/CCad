#pragma once

#include "ccad_core/model.hpp"
#include "ccad_core/serialize.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace ccad {

struct BoardLoadOptions {
  bool initialize_after_load = true;
  std::string source_format = "CCAD_JSON";
};

struct BoardLoadState {
  std::string kicad_class = "BOARD_LOADER";
  std::string source_format = "CCAD_JSON";
  bool loaded = false;
  bool initialize_after_load = true;
  bool board_attached = false;
  bool design_rules_ready = false;
  bool drc_ready = false;
  bool persistent_drc_engine = false;
  std::string drc_engine_model;
  bool connectivity_ready = false;
  bool netlist_ready = false;
  bool netclass_sync_ready = false;
  bool component_class_sync_ready = false;
  bool drawing_sheet_loaded = false;
  bool user_units_ready = false;
  std::size_t board_count = 0;
  std::size_t schematic_count = 0;
  std::size_t active_board_index = 0;
  std::size_t layer_count = 0;
  std::size_t copper_layer_count = 0;
  std::size_t visible_layer_count = 0;
  std::size_t hidden_layer_count = 0;
  std::size_t pad_count = 0;
  std::size_t via_count = 0;
  std::size_t track_count = 0;
  std::size_t graphic_count = 0;
  std::size_t text_count = 0;
  std::size_t zone_count = 0;
  std::size_t keepout_count = 0;
  std::size_t placement_region_count = 0;
  std::size_t route_request_count = 0;
  std::size_t physical_object_count = 0;
  std::size_t board_net_count = 0;
  std::size_t pads_with_nets = 0;
  std::size_t vias_with_nets = 0;
  std::size_t tracks_with_nets = 0;
  std::size_t zones_with_nets = 0;
  std::vector<std::string> pending_kicad_loader_steps;
};

BoardLoadState summarizeLoadedBoard(const Project& project,
                                    const BoardLoadOptions& options = BoardLoadOptions{});

class HeadlessBoardContext final {
 public:
  explicit HeadlessBoardContext(BoardLoadOptions options = BoardLoadOptions{});
  void loadJson(const std::string& json, std::string source_format = "CCAD_JSON");
  std::string saveJson() const;
  Project& project() noexcept { return project_; }
  const Project& project() const noexcept { return project_; }
  const BoardLoadState& state() const noexcept { return state_; }
  bool loaded() const noexcept { return state_.loaded; }
  bool dirty() const noexcept { return dirty_; }
  void markClean() noexcept { dirty_ = false; }
  void markDirty() noexcept { dirty_ = true; }

 private:
  BoardLoadOptions options_;
  Project project_;
  BoardLoadState state_;
  bool dirty_ = false;
};

}  // namespace ccad
