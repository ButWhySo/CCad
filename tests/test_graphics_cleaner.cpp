#include "ccad_core/graphics_cleaner.hpp"
#include "test_support.hpp"

using namespace ccad;

void testNullShapes() {
  Board board;
  
  BoardGraphic null_segment;
  null_segment.id = "g1";
  null_segment.kind = "segment";
  null_segment.layer_id = "F.Cu";
  null_segment.start = {0, 0};
  null_segment.end = {0, 0};
  null_segment.width = {100000};
  
  BoardGraphic valid_segment;
  valid_segment.id = "g2";
  valid_segment.kind = "segment";
  valid_segment.layer_id = "F.Cu";
  valid_segment.start = {0, 0};
  valid_segment.end = {100000, 100000};
  valid_segment.width = {100000};
  
  board.graphics = {null_segment, valid_segment};
  
  GraphicsCleaner cleaner(board, 1000);
  auto actions = cleaner.cleanupBoard(false, false, true, false);
  
  require(actions.size() == 1, "Expected 1 null action");
  require(actions[0].code == CleanupActionCode::null_graphic, "Expected null_graphic code");
  require(actions[0].item_ids[0] == "g1", "Expected id g1");
  
  require(board.graphics.size() == 1, "Expected 1 remaining graphic");
  require(board.graphics[0].id == "g2", "Expected valid graphic to remain");
}

void testDuplicateShapes() {
  Board board;
  
  BoardGraphic g1;
  g1.id = "g1";
  g1.kind = "segment";
  g1.layer_id = "F.Cu";
  g1.start = {0, 0};
  g1.end = {100000, 100000};
  g1.width = {100000};
  
  BoardGraphic g2;
  g2.id = "g2";
  g2.kind = "segment";
  g2.layer_id = "F.Cu";
  g2.start = {100000, 100000}; // reversed
  g2.end = {0, 0};
  g2.width = {100000};
  
  board.graphics = {g1, g2};
  
  GraphicsCleaner cleaner(board, 1000);
  auto actions = cleaner.cleanupBoard(false, false, true, false);
  
  require(actions.size() == 1, "Expected 1 duplicate action");
  require(actions[0].code == CleanupActionCode::duplicate_graphic, "Expected duplicate_graphic code");
  require(actions[0].item_ids[0] == "g2", "Expected id g2");
  
  require(board.graphics.size() == 1, "Expected 1 remaining graphic");
  require(board.graphics[0].id == "g1", "Expected remaining graphic to be g1");
}

void testMergeLinesToRect() {
  Board board;
  
  BoardGraphic top, bottom, left, right;
  top.id = "top"; top.kind = "segment"; top.layer_id = "F.Cu"; top.width = {100000};
  top.start = {0, 100000}; top.end = {100000, 100000};
  
  bottom.id = "bottom"; bottom.kind = "segment"; bottom.layer_id = "F.Cu"; bottom.width = {100000};
  bottom.start = {0, 0}; bottom.end = {100000, 0};
  
  left.id = "left"; left.kind = "segment"; left.layer_id = "F.Cu"; left.width = {100000};
  left.start = {0, 0}; left.end = {0, 100000};
  
  right.id = "right"; right.kind = "segment"; right.layer_id = "F.Cu"; right.width = {100000};
  right.start = {100000, 0}; right.end = {100000, 100000};
  
  board.graphics = {top, bottom, left, right};
  
  GraphicsCleaner cleaner(board, 1000);
  auto actions = cleaner.cleanupBoard(false, true, false, false);
  
  require(actions.size() == 1, "Expected 1 merge rect action");
  require(actions[0].code == CleanupActionCode::lines_to_rect, "Expected lines_to_rect code");
  
  require(board.graphics.size() == 1, "Expected 1 remaining graphic (the new rect)");
  require(board.graphics[0].kind == "rectangle", "Expected kind to be rectangle");
  require(board.graphics[0].start.x.nanometers == 0, "Expected start x 0");
  require(board.graphics[0].start.y.nanometers == 0, "Expected start y 0");
  require(board.graphics[0].end.x.nanometers == 100000, "Expected end x 100000");
  require(board.graphics[0].end.y.nanometers == 100000, "Expected end y 100000");
}

int main() {
  testNullShapes();
  testDuplicateShapes();
  testMergeLinesToRect();
  return 0;
}
