#pragma once

#include <cstdint>

namespace ccad {

struct Length {
  std::int64_t nanometers = 0;
};

struct Point {
  Length x;
  Length y;
};

struct Size {
  Length width;
  Length height;
};

struct Rect {
  Point origin;
  Size size;
};

Length nanometers(std::int64_t value);
Length millimeters(double value);
Length mils(double value);
Point maxPoint(const Rect& rect);

}  // namespace ccad

