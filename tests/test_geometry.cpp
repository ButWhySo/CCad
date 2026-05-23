#include "ccad_core/geometry.hpp"
#include "test_support.hpp"

int main() {
  const ccad::Length one_mm = ccad::millimeters(1);
  require(one_mm.nanometers == 1000000, "1 mm is 1,000,000 nm");

  const ccad::Length half_mm = ccad::millimeters(0.5);
  require(half_mm.nanometers == 500000, "0.5 mm is 500,000 nm");

  const ccad::Length one_mil = ccad::mils(1);
  require(one_mil.nanometers == 25400, "1 mil is 25,400 nm");

  const ccad::Point origin{.x = ccad::millimeters(2), .y = ccad::millimeters(3)};
  const ccad::Size size{.width = ccad::millimeters(10), .height = ccad::millimeters(5)};
  const ccad::Rect rect{.origin = origin, .size = size};
  const ccad::Point max = ccad::maxPoint(rect);

  require(max.x.nanometers == 12000000, "rect max x computed");
  require(max.y.nanometers == 8000000, "rect max y computed");
}

