#include "ccad_core/board_collector.hpp"

#include "ccad_core/board_item.hpp"

#include <algorithm>
#include <stdexcept>

namespace ccad {
namespace {

bool contains(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

std::vector<std::string> visibleLayersForGuide(const Board& board,
                                               const BoardCollectorGuide& guide) {
  if (!guide.visible_layer_ids.empty()) {
    return guide.visible_layer_ids;
  }
  std::vector<std::string> visible_layers;
  for (const Layer& layer : board.layers) {
    if (layer.visible) {
      visible_layers.push_back(layer.id);
    }
  }
  return visible_layers;
}

bool hasVisibleLayer(const std::vector<std::string>& item_layers,
                     const std::vector<std::string>& visible_layers) {
  return std::any_of(item_layers.begin(), item_layers.end(), [&](const std::string& layer_id) {
    return contains(visible_layers, layer_id);
  });
}

bool typeSupportedInCurrentModel(const std::string& kicad_type) {
  return kicad_type == "PCB_PAD_T" || kicad_type == "PCB_VIA_T" ||
         kicad_type == "PCB_TRACE_T" || kicad_type == "PCB_FOOTPRINT_T" ||
         kicad_type == "PCB_ZONE_T" || kicad_type == "PCB_SHAPE_T" ||
         kicad_type == "PCB_TEXT_T";
}

void appendUnsupportedTypes(BoardCollectorReport& report) {
  for (const std::string& kicad_type : report.kicad_scan_types) {
    if (!typeSupportedInCurrentModel(kicad_type) &&
        !contains(report.unsupported_kicad_types, kicad_type)) {
      report.unsupported_kicad_types.push_back(kicad_type);
    }
  }
}

bool noNetFiltered(const std::string& object_type,
                   const std::string& net_id,
                   const BoardCollectorGuide& guide) {
  if (!guide.ignore_no_nets) {
    return false;
  }
  if (object_type == "pad" || object_type == "via" || object_type == "track" ||
      object_type == "footprint") {
    return false;
  }
  return net_id.empty();
}

void maybeAppendCandidate(std::vector<BoardCollectorCandidate>& primary,
                          std::vector<BoardCollectorCandidate>& secondary,
                          const BoardCollectorCandidate& candidate,
                          const BoardCollectorGuide& guide,
                          const std::vector<std::string>& visible_layers) {
  if (guide.ignore_locked_items && candidate.locked) {
    return;
  }
  BoardCollectorCandidate row = candidate;
  row.current_layer_match = contains(row.layer_ids, guide.preferred_layer_id);
  row.visible_layer_match = hasVisibleLayer(row.layer_ids, visible_layers);
  if (row.current_layer_match) {
    row.collection_bucket = "primary";
    primary.push_back(row);
    return;
  }
  if (guide.include_secondary && row.visible_layer_match) {
    row.collection_bucket = "secondary";
    secondary.push_back(row);
  }
}

BoardCollectorCandidate makeCandidate(const std::string& type,
                                      const std::string& id,
                                      const std::string& kicad_type,
                                      const std::string& net_id,
                                      const BoardItemMetadata& metadata) {
  BoardCollectorCandidate candidate;
  candidate.type = type;
  candidate.id = id;
  candidate.kicad_type = kicad_type;
  candidate.net_id = net_id;
  candidate.primary_layer_id = metadata.primary_layer_id;
  candidate.layer_ids = metadata.layer_ids;
  candidate.locked = metadata.locked;
  return candidate;
}

BoardItemMetadata footprintMetadata(const Board& board, const BoardFootprint& footprint) {
  return boardItemMetadata(board,
                           std::vector<std::string>{footprint.layer_id},
                           true,
                           false,
                           false,
                           footprint.locked);
}

void collectType(const Board& board,
                 const std::string& kicad_type,
                 const BoardCollectorGuide& guide,
                 const std::vector<std::string>& visible_layers,
                 std::vector<BoardCollectorCandidate>& primary,
                 std::vector<BoardCollectorCandidate>& secondary) {
  if (kicad_type == "PCB_PAD_T") {
    for (const Pad& pad : board.pads) {
      if (noNetFiltered("pad", pad.net_id, guide)) {
        continue;
      }
      maybeAppendCandidate(primary,
                           secondary,
                           makeCandidate("pad", pad.id, kicad_type, pad.net_id,
                                         boardItemMetadata(board, pad)),
                           guide,
                           visible_layers);
    }
    return;
  }
  if (kicad_type == "PCB_VIA_T") {
    for (const Via& via : board.vias) {
      if (noNetFiltered("via", via.net_id, guide)) {
        continue;
      }
      maybeAppendCandidate(primary,
                           secondary,
                           makeCandidate("via", via.id, kicad_type, via.net_id,
                                         boardItemMetadata(board, via)),
                           guide,
                           visible_layers);
    }
    return;
  }
  if (kicad_type == "PCB_TRACE_T") {
    if (guide.ignore_tracks) {
      return;
    }
    for (const TrackSegment& track : board.tracks) {
      if (noNetFiltered("track", track.net_id, guide)) {
        continue;
      }
      maybeAppendCandidate(primary,
                           secondary,
                           makeCandidate("track", track.id, kicad_type, track.net_id,
                                         boardItemMetadata(board, track)),
                           guide,
                           visible_layers);
    }
    return;
  }
  if (kicad_type == "PCB_FOOTPRINT_T") {
    for (const BoardFootprint& footprint : board.footprints) {
      maybeAppendCandidate(primary,
                           secondary,
                           makeCandidate("footprint", footprint.reference, kicad_type, "",
                                         footprintMetadata(board, footprint)),
                           guide,
                           visible_layers);
    }
    return;
  }
  if (kicad_type == "PCB_ZONE_T") {
    for (const BoardZone& zone : board.zones) {
      if (noNetFiltered("zone", zone.net_id, guide)) {
        continue;
      }
      maybeAppendCandidate(primary,
                           secondary,
                           makeCandidate("zone", zone.id, kicad_type, zone.net_id,
                                         boardItemMetadata(board, zone)),
                           guide,
                           visible_layers);
    }
    return;
  }
  if (kicad_type == "PCB_SHAPE_T") {
    for (const BoardGraphic& graphic : board.graphics) {
      if (noNetFiltered("graphic", "", guide)) {
        continue;
      }
      maybeAppendCandidate(primary,
                           secondary,
                           makeCandidate("graphic", graphic.id, kicad_type, "",
                                         boardItemMetadata(board, graphic)),
                           guide,
                           visible_layers);
    }
    return;
  }
  if (kicad_type == "PCB_TEXT_T") {
    for (const BoardText& text : board.texts) {
      if (noNetFiltered("text", "", guide)) {
        continue;
      }
      maybeAppendCandidate(primary,
                           secondary,
                           makeCandidate("text", text.id, kicad_type, "",
                                         boardItemMetadata(board, text)),
                           guide,
                           visible_layers);
    }
  }
}

}  // namespace

std::vector<std::string> boardCollectorScanTypes(const BoardCollectorScanSet scan_set) {
  switch (scan_set) {
    case BoardCollectorScanSet::all_board_items:
      return {"PCB_MARKER_T",
              "PCB_TEXT_T",
              "PCB_REFERENCE_IMAGE_T",
              "PCB_TEXTBOX_T",
              "PCB_TABLE_T",
              "PCB_TABLECELL_T",
              "PCB_SHAPE_T",
              "PCB_DIM_ALIGNED_T",
              "PCB_DIM_CENTER_T",
              "PCB_DIM_RADIAL_T",
              "PCB_DIM_ORTHOGONAL_T",
              "PCB_DIM_LEADER_T",
              "PCB_TARGET_T",
              "PCB_VIA_T",
              "PCB_TRACE_T",
              "PCB_ARC_T",
              "PCB_PAD_T",
              "PCB_FIELD_T",
              "PCB_FOOTPRINT_T",
              "PCB_GROUP_T",
              "PCB_ZONE_T",
              "PCB_POINT_T",
              "PCB_GENERATOR_T",
              "PCB_BARCODE_T"};
    case BoardCollectorScanSet::board_level_items:
      return {"PCB_MARKER_T",
              "PCB_REFERENCE_IMAGE_T",
              "PCB_TEXT_T",
              "PCB_TEXTBOX_T",
              "PCB_TABLE_T",
              "PCB_SHAPE_T",
              "PCB_DIM_ALIGNED_T",
              "PCB_DIM_CENTER_T",
              "PCB_DIM_RADIAL_T",
              "PCB_DIM_ORTHOGONAL_T",
              "PCB_DIM_LEADER_T",
              "PCB_TARGET_T",
              "PCB_POINT_T",
              "PCB_VIA_T",
              "PCB_ARC_T",
              "PCB_TRACE_T",
              "PCB_FOOTPRINT_T",
              "PCB_GROUP_T",
              "PCB_ZONE_T",
              "PCB_GENERATOR_T",
              "PCB_BARCODE_T"};
    case BoardCollectorScanSet::footprints:
      return {"PCB_FOOTPRINT_T"};
    case BoardCollectorScanSet::pads_or_tracks:
      return {"PCB_PAD_T", "PCB_VIA_T", "PCB_TRACE_T", "PCB_ARC_T"};
    case BoardCollectorScanSet::footprint_items:
      return {"PCB_MARKER_T",
              "PCB_FIELD_T",
              "PCB_TEXT_T",
              "PCB_TEXTBOX_T",
              "PCB_TABLE_T",
              "PCB_TABLECELL_T",
              "PCB_SHAPE_T",
              "PCB_DIM_ALIGNED_T",
              "PCB_DIM_CENTER_T",
              "PCB_DIM_RADIAL_T",
              "PCB_DIM_ORTHOGONAL_T",
              "PCB_DIM_LEADER_T",
              "PCB_PAD_T",
              "PCB_ZONE_T",
              "PCB_GROUP_T",
              "PCB_POINT_T",
              "PCB_REFERENCE_IMAGE_T",
              "PCB_BARCODE_T"};
    case BoardCollectorScanSet::tracks:
      return {"PCB_TRACE_T", "PCB_ARC_T", "PCB_VIA_T"};
    case BoardCollectorScanSet::dimensions:
      return {"PCB_DIM_ALIGNED_T",
              "PCB_DIM_CENTER_T",
              "PCB_DIM_RADIAL_T",
              "PCB_DIM_ORTHOGONAL_T",
              "PCB_DIM_LEADER_T"};
    case BoardCollectorScanSet::draggable_items:
      return {"PCB_TRACE_T", "PCB_VIA_T", "PCB_FOOTPRINT_T", "PCB_ARC_T"};
  }
  return {};
}

BoardCollectorScanSet parseBoardCollectorScanSet(const std::string_view name) {
  if (name == "all_board_items") {
    return BoardCollectorScanSet::all_board_items;
  }
  if (name == "board_level_items") {
    return BoardCollectorScanSet::board_level_items;
  }
  if (name == "footprints") {
    return BoardCollectorScanSet::footprints;
  }
  if (name == "pads_or_tracks") {
    return BoardCollectorScanSet::pads_or_tracks;
  }
  if (name == "footprint_items") {
    return BoardCollectorScanSet::footprint_items;
  }
  if (name == "tracks") {
    return BoardCollectorScanSet::tracks;
  }
  if (name == "dimensions") {
    return BoardCollectorScanSet::dimensions;
  }
  if (name == "draggable_items") {
    return BoardCollectorScanSet::draggable_items;
  }
  throw std::invalid_argument("unknown board collector scan set");
}

std::string boardCollectorScanSetName(const BoardCollectorScanSet scan_set) {
  switch (scan_set) {
    case BoardCollectorScanSet::all_board_items:
      return "all_board_items";
    case BoardCollectorScanSet::board_level_items:
      return "board_level_items";
    case BoardCollectorScanSet::footprints:
      return "footprints";
    case BoardCollectorScanSet::pads_or_tracks:
      return "pads_or_tracks";
    case BoardCollectorScanSet::footprint_items:
      return "footprint_items";
    case BoardCollectorScanSet::tracks:
      return "tracks";
    case BoardCollectorScanSet::dimensions:
      return "dimensions";
    case BoardCollectorScanSet::draggable_items:
      return "draggable_items";
  }
  return "";
}

BoardCollectorReport collectBoardItems(const Board& board,
                                       const BoardCollectorScanSet scan_set,
                                       const BoardCollectorGuide& guide) {
  BoardCollectorReport report;
  report.scan_set = boardCollectorScanSetName(scan_set);
  report.kicad_scan_types = boardCollectorScanTypes(scan_set);
  appendUnsupportedTypes(report);

  const std::vector<std::string> visible_layers = visibleLayersForGuide(board, guide);
  std::vector<BoardCollectorCandidate> primary;
  std::vector<BoardCollectorCandidate> secondary;
  for (const std::string& kicad_type : report.kicad_scan_types) {
    collectType(board, kicad_type, guide, visible_layers, primary, secondary);
  }

  report.primary_count = static_cast<int>(primary.size());
  report.secondary_count = static_cast<int>(secondary.size());
  report.candidates.reserve(primary.size() + secondary.size());
  report.candidates.insert(report.candidates.end(), primary.begin(), primary.end());
  report.candidates.insert(report.candidates.end(), secondary.begin(), secondary.end());
  return report;
}

}  // namespace ccad
