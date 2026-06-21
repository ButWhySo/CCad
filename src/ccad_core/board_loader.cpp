#include "ccad_core/board_loader.hpp"

#include <set>

namespace ccad {
namespace {

void addNetIfPresent(std::set<std::string>& net_ids, const std::string& net_id,
                     std::size_t& count) {
  if (!net_id.empty()) {
    net_ids.insert(net_id);
    ++count;
  }
}

}  // namespace

BoardLoadState summarizeLoadedBoard(const Project& project, const BoardLoadOptions& options) {
  BoardLoadState state;
  state.source_format = options.source_format.empty() ? "CCAD_JSON" : options.source_format;
  state.loaded = true;
  state.initialize_after_load = options.initialize_after_load;
  state.board_count = project.boards.size();
  state.schematic_count = project.schematics.size();
  state.drc_engine_model = "stateless_ccad_runDrc";

  const Board* board = primaryBoard(project);
  if (board == nullptr) {
    state.active_board_index = 0;
    state.pending_kicad_loader_steps = {
        "board_file_plugin_import",
        "drawing_sheet_resolution",
        "persistent_drc_engine",
        "connectivity_graph_cache",
        "netclass_sync",
        "component_class_sync",
    };
    return state;
  }

  state.active_board_index = 0;
  state.board_attached = options.initialize_after_load;
  state.design_rules_ready = options.initialize_after_load;
  state.drc_ready = options.initialize_after_load;
  state.connectivity_ready = options.initialize_after_load;
  state.netlist_ready = options.initialize_after_load;
  state.user_units_ready = options.initialize_after_load;

  state.layer_count = board->layers.size();
  for (const Layer& layer : board->layers) {
    if (layer.kind == "copper") {
      ++state.copper_layer_count;
    }
    if (layer.visible) {
      ++state.visible_layer_count;
    } else {
      ++state.hidden_layer_count;
    }
  }

  state.pad_count = board->pads.size();
  state.via_count = board->vias.size();
  state.track_count = board->tracks.size();
  state.graphic_count = board->graphics.size();
  state.text_count = board->texts.size();
  state.zone_count = board->zones.size();
  state.keepout_count = board->keepouts.size();
  state.placement_region_count = board->placement_regions.size();
  state.route_request_count = board->route_requests.size();
  state.physical_object_count = state.pad_count + state.via_count + state.track_count +
                                state.graphic_count + state.text_count + state.zone_count;

  std::set<std::string> net_ids;
  for (const Pad& pad : board->pads) {
    addNetIfPresent(net_ids, pad.net_id, state.pads_with_nets);
  }
  for (const Via& via : board->vias) {
    addNetIfPresent(net_ids, via.net_id, state.vias_with_nets);
  }
  for (const TrackSegment& track : board->tracks) {
    addNetIfPresent(net_ids, track.net_id, state.tracks_with_nets);
  }
  for (const BoardZone& zone : board->zones) {
    addNetIfPresent(net_ids, zone.net_id, state.zones_with_nets);
  }
  state.board_net_count = net_ids.size();

  state.pending_kicad_loader_steps = {
      "external_pcb_io_plugin_manager",
      "drawing_sheet_resolution",
      "persistent_drc_engine",
      "drc_exclusion_marker_materialization",
      "full_connectivity_graph_cache",
      "netclass_sync",
      "component_class_sync",
      "tuning_profile_property_sync",
  };

  return state;
}

}  // namespace ccad
