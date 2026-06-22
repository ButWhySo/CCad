#include "ccad_core/board_collector.hpp"

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <algorithm>
#include <cstdlib>
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

bool contains(const std::vector<std::string>& values, const std::string& needle) {
  return std::find(values.begin(), values.end(), needle) != values.end();
}

ccad::Board makeCollectorFixture() {
  ccad::Board board;
  board.layers = {
      {.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
      {.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true},
      {.id = "Dwgs.User", .name = "User drawings", .kind = "user", .visible = true},
      {.id = "F.SilkS", .name = "Front silkscreen", .kind = "silkscreen", .visible = false},
  };
  board.footprints.push_back(ccad::BoardFootprint{
      .reference = "U1",
      .value = "MCU",
      .footprint_name = "Package_SO:SOIC-8",
      .layer_id = "B.Cu",
      .position = {.x = ccad::millimeters(14), .y = ccad::millimeters(8)},
  });
  board.pads.push_back(ccad::Pad{.id = "P1",
                                 .component_id = "U1",
                                 .pin_name = "1",
                                 .net_id = "N1",
                                 .type = "smd",
                                 .position = {.x = ccad::millimeters(3),
                                              .y = ccad::millimeters(4)},
                                 .padstack = ccad::Padstack{
                                    .layer_set = {"F.Cu"},
                                    .copper_props = {{"top", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = {.width = ccad::millimeters(1), .height = ccad::millimeters(1)}}}}}
                                 }});
  board.pads.push_back(ccad::Pad{.id = "P2",
                                 .component_id = "U1",
                                 .pin_name = "2",
                                 .net_id = "",
                                 .type = "smd",
                                 .position = {.x = ccad::millimeters(4),
                                              .y = ccad::millimeters(4)},
                                 .padstack = ccad::Padstack{
                                    .layer_set = {"B.Cu"},
                                    .copper_props = {{"bottom", ccad::PadstackCopperLayerProps{.shape = ccad::PadstackShapeProps{.shape = ccad::PadShape::Rectangle, .size = {.width = ccad::millimeters(1), .height = ccad::millimeters(1)}}}}}
                                 }});
  board.vias.push_back(ccad::Via{.id = "V1",
                                 .net_id = "N1",
                                 .position = {.x = ccad::millimeters(5),
                                              .y = ccad::millimeters(6)},
                                 .diameter = ccad::millimeters(0.8),
                                 .drill = ccad::millimeters(0.4)});
  board.tracks.push_back(ccad::TrackSegment{.id = "T1",
                                            .net_id = "N1",
                                            .layer_id = "F.Cu",
                                            .start = {.x = ccad::millimeters(3),
                                                      .y = ccad::millimeters(4)},
                                            .end = {.x = ccad::millimeters(5),
                                                    .y = ccad::millimeters(6)},
                                            .width = ccad::millimeters(0.25)});
  board.tracks.push_back(ccad::TrackSegment{.id = "T2",
                                            .net_id = "",
                                            .layer_id = "B.Cu",
                                            .start = {.x = ccad::millimeters(6),
                                                      .y = ccad::millimeters(4)},
                                            .end = {.x = ccad::millimeters(8),
                                                    .y = ccad::millimeters(4)},
                                            .width = ccad::millimeters(0.25)});
  board.graphics.push_back(ccad::BoardGraphic{.id = "G1",
                                              .kind = "line",
                                              .layer_id = "Dwgs.User",
                                              .start = {.x = ccad::millimeters(1),
                                                        .y = ccad::millimeters(1)},
                                              .end = {.x = ccad::millimeters(2),
                                                      .y = ccad::millimeters(1)},
                                              .width = ccad::millimeters(0.15)});
  board.texts.push_back(ccad::BoardText{.id = "TXT1",
                                        .layer_id = "F.SilkS",
                                        .text = "Hidden",
                                        .position = {.x = ccad::millimeters(7),
                                                     .y = ccad::millimeters(8)},
                                        .size = {.width = ccad::millimeters(1),
                                                 .height = ccad::millimeters(1)}});
  board.zones.push_back(ccad::BoardZone{.id = "Z1",
                                        .name = "Floating copper",
                                        .net_id = "",
                                        .layer_ids = {"B.Cu"},
                                        .outline = {{.x = ccad::millimeters(1),
                                                     .y = ccad::millimeters(1)},
                                                    {.x = ccad::millimeters(2),
                                                     .y = ccad::millimeters(1)},
                                                    {.x = ccad::millimeters(2),
                                                     .y = ccad::millimeters(2)}}});
  return board;
}

const ccad::BoardCollectorCandidate& requireCandidate(const ccad::BoardCollectorReport& report,
                                                      const std::string& id) {
  const auto it = std::find_if(report.candidates.begin(),
                              report.candidates.end(),
                              [&](const ccad::BoardCollectorCandidate& candidate) {
                                return candidate.id == id;
                              });
  require(it != report.candidates.end(), "collector candidate not found: " + id);
  return *it;
}

std::size_t candidateIndex(const ccad::BoardCollectorReport& report, const std::string& id) {
  for (std::size_t i = 0; i < report.candidates.size(); ++i) {
    if (report.candidates.at(i).id == id) {
      return i;
    }
  }
  throw std::runtime_error("collector candidate not found: " + id);
}

void test_scan_sets_match_kicad_source_lists() {
  const std::vector<std::string> all =
      ccad::boardCollectorScanTypes(ccad::BoardCollectorScanSet::all_board_items);
  require(all.size() == 24, "AllBoardItems mirrors KiCad collector item count");
  require(all.front() == "PCB_MARKER_T", "AllBoardItems starts with KiCad marker type");
  require(contains(all, "PCB_PAD_T"), "AllBoardItems includes pads");
  require(contains(all, "PCB_TRACE_T"), "AllBoardItems includes traces");
  require(contains(all, "PCB_ARC_T"), "AllBoardItems includes arcs");
  require(all.back() == "PCB_BARCODE_T", "AllBoardItems ends with KiCad barcode type");

  const std::vector<std::string> pads_or_tracks =
      ccad::boardCollectorScanTypes(ccad::BoardCollectorScanSet::pads_or_tracks);
  require(pads_or_tracks == std::vector<std::string>{"PCB_PAD_T",
                                                     "PCB_VIA_T",
                                                     "PCB_TRACE_T",
                                                     "PCB_ARC_T"},
          "PadsOrTracks order mirrors KiCad source");

  const std::vector<std::string> tracks =
      ccad::boardCollectorScanTypes(ccad::BoardCollectorScanSet::tracks);
  require(tracks == std::vector<std::string>{"PCB_TRACE_T", "PCB_ARC_T", "PCB_VIA_T"},
          "Tracks order mirrors KiCad source");

  const std::vector<std::string> draggable =
      ccad::boardCollectorScanTypes(ccad::BoardCollectorScanSet::draggable_items);
  require(draggable == std::vector<std::string>{"PCB_TRACE_T",
                                               "PCB_VIA_T",
                                               "PCB_FOOTPRINT_T",
                                               "PCB_ARC_T"},
          "DraggableItems order mirrors KiCad source");
  require(ccad::parseBoardCollectorScanSet("pads_or_tracks") ==
              ccad::BoardCollectorScanSet::pads_or_tracks,
          "collector parses CLI scan-set names");

  bool rejected = false;
  try {
    (void) ccad::parseBoardCollectorScanSet("nonsense");
  } catch (const std::invalid_argument&) {
    rejected = true;
  }
  require(rejected, "collector rejects unknown scan set");
}

void test_collects_primary_and_secondary_candidates_by_layer() {
  const ccad::Board board = makeCollectorFixture();
  ccad::BoardCollectorGuide guide;
  guide.preferred_layer_id = "F.Cu";
  guide.visible_layer_ids = {"F.Cu", "B.Cu"};
  guide.include_secondary = true;

  const ccad::BoardCollectorReport report =
      ccad::collectBoardItems(board, ccad::BoardCollectorScanSet::pads_or_tracks, guide);
  require(report.kicad_collector == "GENERAL_COLLECTOR",
          "collector reports the KiCad source class");
  require(report.parity_scope == "collector_scan_set_layer_first_slice",
          "collector reports first-slice parity scope");
  require(report.scan_set == "pads_or_tracks", "collector reports scan set");
  require(contains(report.unsupported_kicad_types, "PCB_ARC_T"),
          "collector reports unsupported KiCad arc type");
  require(report.primary_count == 3, "collector counts F.Cu pad, through via, and F.Cu track");
  require(report.secondary_count == 2, "collector counts B.Cu pad and track as secondary");
  require(report.candidates.size() == 5, "collector returns primary plus secondary candidates");

  const ccad::BoardCollectorCandidate& p1 = requireCandidate(report, "P1");
  require(p1.type == "pad", "collector identifies pad candidate");
  require(p1.kicad_type == "PCB_PAD_T", "collector maps pad to KiCad type");
  require(p1.collection_bucket == "primary", "collector buckets preferred-layer pad as primary");
  require(p1.current_layer_match, "collector marks preferred-layer match");
  require(p1.primary_layer_id == "F.Cu", "collector reports pad primary layer");

  const ccad::BoardCollectorCandidate& p2 = requireCandidate(report, "P2");
  require(p2.collection_bucket == "secondary", "collector buckets visible non-preferred pad");
  require(!p2.current_layer_match, "collector marks non-preferred pad as non-current");

  const ccad::BoardCollectorCandidate& v1 = requireCandidate(report, "V1");
  require(v1.collection_bucket == "primary", "collector treats through via as current layer");
  require(contains(v1.layer_ids, "F.Cu"), "collector expands via onto front copper");
  require(contains(v1.layer_ids, "B.Cu"), "collector expands via onto back copper");
}

void test_collector_filters_tracks_and_no_net_board_level_items() {
  const ccad::Board board = makeCollectorFixture();

  ccad::BoardCollectorGuide ignore_tracks_guide;
  ignore_tracks_guide.preferred_layer_id = "F.Cu";
  ignore_tracks_guide.visible_layer_ids = {"F.Cu", "B.Cu"};
  ignore_tracks_guide.include_secondary = true;
  ignore_tracks_guide.ignore_tracks = true;
  const ccad::BoardCollectorReport no_tracks =
      ccad::collectBoardItems(board,
                              ccad::BoardCollectorScanSet::pads_or_tracks,
                              ignore_tracks_guide);
  requireCandidate(no_tracks, "P1");
  requireCandidate(no_tracks, "P2");
  requireCandidate(no_tracks, "V1");
  require(std::none_of(no_tracks.candidates.begin(),
                       no_tracks.candidates.end(),
                       [](const ccad::BoardCollectorCandidate& candidate) {
                         return candidate.type == "track";
                       }),
          "ignore-tracks removes traces without removing pads or vias");

  ccad::BoardCollectorGuide no_net_guide;
  no_net_guide.preferred_layer_id = "B.Cu";
  no_net_guide.visible_layer_ids = {"F.Cu", "B.Cu", "Dwgs.User"};
  no_net_guide.include_secondary = true;
  no_net_guide.ignore_no_nets = true;
  const ccad::BoardCollectorReport no_net =
      ccad::collectBoardItems(board,
                              ccad::BoardCollectorScanSet::all_board_items,
                              no_net_guide);
  requireCandidate(no_net, "P2");
  requireCandidate(no_net, "T2");
  require(std::none_of(no_net.candidates.begin(),
                       no_net.candidates.end(),
                       [](const ccad::BoardCollectorCandidate& candidate) {
                         return candidate.id == "Z1" || candidate.id == "G1";
                       }),
          "ignore-no-nets filters zone and user graphics but keeps pad and track parity");
}

void test_tracks_scan_set_keeps_secondary_after_primary() {
  const ccad::Board board = makeCollectorFixture();
  ccad::BoardCollectorGuide guide;
  guide.preferred_layer_id = "B.Cu";
  guide.visible_layer_ids = {"F.Cu", "B.Cu"};
  guide.include_secondary = true;

  const ccad::BoardCollectorReport report =
      ccad::collectBoardItems(board, ccad::BoardCollectorScanSet::tracks, guide);
  require(report.primary_count == 2, "B.Cu tracks scan has track and through-via primary rows");
  require(report.secondary_count == 1, "tracks scan preserves F.Cu track as secondary row");
  require(candidateIndex(report, "T1") > candidateIndex(report, "T2"),
          "collector appends secondary rows after primary rows like KiCad");
  require(candidateIndex(report, "T1") > candidateIndex(report, "V1"),
          "collector appends secondary rows after via primary rows");
}

void test_collector_filters_locked_items_when_guide_requests_it() {
  ccad::Board board = makeCollectorFixture();
  board.pads.at(1).locked = true;
  board.tracks.at(1).locked = true;
  board.footprints.at(0).locked = true;

  ccad::BoardCollectorGuide guide;
  guide.preferred_layer_id = "B.Cu";
  guide.visible_layer_ids = {"F.Cu", "B.Cu"};
  guide.include_secondary = true;
  guide.ignore_locked_items = true;

  const ccad::BoardCollectorReport pads_or_tracks =
      ccad::collectBoardItems(board, ccad::BoardCollectorScanSet::pads_or_tracks, guide);
  require(std::none_of(pads_or_tracks.candidates.begin(),
                       pads_or_tracks.candidates.end(),
                       [](const ccad::BoardCollectorCandidate& candidate) {
                         return candidate.id == "P2" || candidate.id == "T2";
                       }),
          "ignore-locked filters locked pads and tracks like KiCad GENERAL_COLLECTOR");

  const ccad::BoardCollectorReport footprints =
      ccad::collectBoardItems(board, ccad::BoardCollectorScanSet::footprints, guide);
  require(footprints.candidates.empty(), "ignore-locked filters locked footprints");
}

}  // namespace

int main() {
  try {
    test_scan_sets_match_kicad_source_lists();
    test_collects_primary_and_secondary_candidates_by_layer();
    test_collector_filters_tracks_and_no_net_board_level_items();
    test_tracks_scan_set_keeps_secondary_after_primary();
    test_collector_filters_locked_items_when_guide_requests_it();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
