#include "ccad_core/canvas.hpp"

namespace ccad {

CanvasScene buildCanvasScene(const Project& project) {
  CanvasScene scene;
  if (!project.board.has_value()) {
    return scene;
  }

  scene.has_board = true;
  scene.board_width_nm = project.board->outline.size.width.nanometers;
  scene.board_height_nm = project.board->outline.size.height.nanometers;
  scene.view_width_units = static_cast<double>(scene.board_width_nm) / 1000000.0;
  scene.view_height_units = static_cast<double>(scene.board_height_nm) / 1000000.0;
  return scene;
}

}  // namespace ccad

