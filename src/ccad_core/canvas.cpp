#include "ccad_core/canvas.hpp"

namespace ccad {
namespace {

double toMillimeters(const Length& length) {
  return static_cast<double>(length.nanometers) / 1000000.0;
}

}  // namespace

CanvasScene buildCanvasScene(const Project& project) {
  CanvasScene scene;
  if (!project.board.has_value()) {
    return scene;
  }

  scene.has_board = true;
  scene.board_width_nm = project.board->outline.size.width.nanometers;
  scene.board_height_nm = project.board->outline.size.height.nanometers;
  scene.view_width_units = toMillimeters(project.board->outline.size.width);
  scene.view_height_units = toMillimeters(project.board->outline.size.height);

  for (const Keepout& keepout : project.board->keepouts) {
    scene.keepouts.push_back(CanvasKeepout{
        .id = keepout.id,
        .kind = keepout.kind,
        .x_units = toMillimeters(keepout.area.origin.x),
        .y_units = toMillimeters(keepout.area.origin.y),
        .width_units = toMillimeters(keepout.area.size.width),
        .height_units = toMillimeters(keepout.area.size.height),
    });
  }

  for (const Pad& pad : project.board->pads) {
    scene.pads.push_back(CanvasPad{
        .id = pad.id,
        .x_units = toMillimeters(pad.position.x),
        .y_units = toMillimeters(pad.position.y),
        .width_units = toMillimeters(pad.size.width),
        .height_units = toMillimeters(pad.size.height),
        .rotation_degrees = pad.rotation_degrees,
    });
  }

  for (const Via& via : project.board->vias) {
    scene.vias.push_back(CanvasVia{
        .id = via.id,
        .x_units = toMillimeters(via.position.x),
        .y_units = toMillimeters(via.position.y),
        .diameter_units = toMillimeters(via.diameter),
        .drill_units = toMillimeters(via.drill),
    });
  }

  for (const TrackSegment& track : project.board->tracks) {
    scene.tracks.push_back(CanvasTrack{
        .id = track.id,
        .start_x_units = toMillimeters(track.start.x),
        .start_y_units = toMillimeters(track.start.y),
        .end_x_units = toMillimeters(track.end.x),
        .end_y_units = toMillimeters(track.end.y),
        .width_units = toMillimeters(track.width),
    });
  }
  return scene;
}

}  // namespace ccad

