#include "ccad_core/board_item_container.hpp"

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

ccad::Board makeContainerFixture() {
  ccad::Board board;
  board.layers = {
      {.id = "F.Cu", .name = "Front copper", .kind = "copper", .visible = true},
      {.id = "B.Cu", .name = "Back copper", .kind = "copper", .visible = true},
      {.id = "Dwgs.User", .name = "User drawings", .kind = "user", .visible = true},
  };
  board.pads.push_back(ccad::Pad{.id = "P1",
                                 .component_id = "U1",
                                 .pin_name = "1",
                                 .net_id = "N1",
                                 .layers = {"F.Cu"},
                                 .position = {.x = ccad::millimeters(3),
                                              .y = ccad::millimeters(4)},
                                 .size = {.width = ccad::millimeters(1),
                                          .height = ccad::millimeters(1)}});
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
  board.graphics.push_back(ccad::BoardGraphic{.id = "G1",
                                              .kind = "line",
                                              .layer_id = "Dwgs.User",
                                              .start = {.x = ccad::millimeters(1),
                                                        .y = ccad::millimeters(1)},
                                              .end = {.x = ccad::millimeters(2),
                                                      .y = ccad::millimeters(1)},
                                              .width = ccad::millimeters(0.15)});
  board.texts.push_back(ccad::BoardText{.id = "TXT1",
                                        .layer_id = "Dwgs.User",
                                        .text = "A",
                                        .position = {.x = ccad::millimeters(7),
                                                     .y = ccad::millimeters(8)},
                                        .size = {.width = ccad::millimeters(1),
                                                 .height = ccad::millimeters(1)}});
  board.zones.push_back(ccad::BoardZone{.id = "Z1",
                                        .name = "GND",
                                        .net_id = "N1",
                                        .layer_ids = {"F.Cu"},
                                        .outline = {{.x = ccad::millimeters(1),
                                                     .y = ccad::millimeters(1)},
                                                    {.x = ccad::millimeters(2),
                                                     .y = ccad::millimeters(1)},
                                                    {.x = ccad::millimeters(2),
                                                     .y = ccad::millimeters(2)}}});
  board.keepouts.push_back(ccad::Keepout{
      .id = "K1",
      .kind = "placement",
      .area = {.origin = {.x = ccad::millimeters(9), .y = ccad::millimeters(9)},
               .size = {.width = ccad::millimeters(2), .height = ccad::millimeters(2)}}});
  board.placement_regions.push_back(ccad::PlacementRegion{
      .id = "R1",
      .kind = "component",
      .area = {.origin = {.x = ccad::millimeters(10), .y = ccad::millimeters(10)},
               .size = {.width = ccad::millimeters(3), .height = ccad::millimeters(3)}}});
  return board;
}

void test_container_summary_matches_kicad_modes() {
  const ccad::Board board = makeContainerFixture();
  const ccad::BoardItemContainerSummary summary = ccad::summarizeBoardItemContainer(board);

  require(summary.kicad_class == "BOARD_ITEM_CONTAINER", "container summary reports KiCad class");
  require(summary.container_kind == "BOARD", "container summary reports board container kind");
  require(summary.add_modes.size() == 4, "container summary exposes all KiCad add modes");
  require(summary.add_modes.at(0) == "insert", "first add mode is insert");
  require(summary.add_modes.at(1) == "append", "second add mode is append");
  require(summary.add_modes.at(2) == "bulk_append", "third add mode is bulk append");
  require(summary.add_modes.at(3) == "bulk_insert", "fourth add mode is bulk insert");
  require(summary.remove_modes.size() == 2, "container summary exposes all KiCad remove modes");
  require(summary.remove_modes.at(0) == "normal", "first remove mode is normal");
  require(summary.remove_modes.at(1) == "bulk", "second remove mode is bulk");
  require(summary.board_item_count == 6, "summary counts board item analogues");
  require(summary.constraint_item_count == 2, "summary counts CCad constraint-region items");
  require(summary.delete_calls_remove == true, "summary documents KiCad Delete via Remove");
  require(summary.skip_connectivity_argument_supported == true,
          "summary documents KiCad Add skip-connectivity argument");
}

void test_container_lookup_and_unique_id_checks_cover_board_owned_items() {
  const ccad::Board board = makeContainerFixture();

  const auto pad_ref = ccad::findBoardContainerItem(board, "P1");
  require(pad_ref.has_value(), "container lookup finds pad");
  require(pad_ref->kind == ccad::BoardContainerItemKind::pad, "pad lookup reports pad kind");
  require(pad_ref->index == 0, "pad lookup reports vector index");
  require(pad_ref->kicad_board_item == true, "pad lookup marks KiCad board item analogue");

  const auto keepout_ref = ccad::findBoardContainerItem(board, "K1");
  require(keepout_ref.has_value(), "container lookup finds keepout");
  require(keepout_ref->kind == ccad::BoardContainerItemKind::keepout,
          "keepout lookup reports keepout kind");
  require(keepout_ref->kicad_board_item == false,
          "keepout lookup does not overclaim KiCad BOARD_ITEM parity");

  require(!ccad::findBoardContainerItem(board, "MISSING").has_value(),
          "container lookup returns empty optional for missing object");
  require(ccad::hasBoardContainerItemId(board, "Z1"), "container id check sees zone");
  require(!ccad::hasBoardContainerItemId(board, "MISSING"), "container id check rejects missing");

  bool duplicate_rejected = false;
  try {
    ccad::requireUniqueBoardContainerItemId(board, "TXT1");
  } catch (const std::runtime_error& error) {
    duplicate_rejected = std::string(error.what()).find("duplicate physical object id: TXT1") !=
                         std::string::npos;
  }
  require(duplicate_rejected, "container unique-id check rejects text duplicate");
  ccad::requireUniqueBoardContainerItemId(board, "NEW1");
}

void test_container_remove_deletes_one_item_and_reports_mode() {
  ccad::Board board = makeContainerFixture();

  const ccad::BoardContainerRemoveResult result =
      ccad::removeBoardContainerItem(board, "T1", ccad::BoardContainerRemoveMode::bulk);
  require(result.removed, "container remove reports removed");
  require(result.kind == ccad::BoardContainerItemKind::track, "container remove reports kind");
  require(result.index == 0, "container remove reports removed index");
  require(result.mode == ccad::BoardContainerRemoveMode::bulk, "container remove reports mode");
  require(board.tracks.empty(), "container remove deletes the track");
  require(board.pads.size() == 1, "container remove leaves unrelated pads");

  const ccad::BoardContainerRemoveResult missing =
      ccad::removeBoardContainerItem(board, "MISSING");
  require(!missing.removed, "container remove reports missing object without mutation");
  require(missing.kind == ccad::BoardContainerItemKind::unknown,
          "missing container remove reports unknown kind");
}

}  // namespace

int main() {
  try {
    test_container_summary_matches_kicad_modes();
    test_container_lookup_and_unique_id_checks_cover_board_owned_items();
    test_container_remove_deletes_one_item_and_reports_mode();
  } catch (const std::exception& error) {
    std::cerr << "test failure: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
