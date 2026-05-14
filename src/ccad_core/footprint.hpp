#pragma once

#include "ccad_core/geometry.hpp"

#include <optional>
#include <string>
#include <vector>

namespace ccad {

struct FootprintPad {
  std::string number;
  std::string type;
  std::string shape;
  Point position;
  double rotation_degrees = 0.0;
  Size size;
  std::optional<Length> drill;
  std::vector<std::string> layers;
};

struct Footprint {
  std::string name;
  std::vector<FootprintPad> pads;
};

}  // namespace ccad
