#pragma once
#include "ccad_core/model.hpp"
#include <vector>

namespace ccad {

// Connects disjointed board shapes (graphics) by snapping endpoints together
// if they are within the given epsilon distance.
void connectBoardShapes(std::vector<BoardGraphic*>& graphics, Length epsilon);

}  // namespace ccad
