#include "ccad_core/gfx_import_utils.hpp"
#include "test_support.hpp"

int main() {
  ccad::ImageImportData image;
  image.width = 3;
  image.height = 1;
  image.has_alpha = true;
  image.pixels = {255, 0, 0, 255, 255, 0, 0, 255, 0, 0, 255, 0};
  std::vector<ccad::SchGraphic> graphics;
  ccad::convertImageToPolygons(image, ccad::Point{ccad::millimeters(1), ccad::millimeters(1)}, graphics);
  require(graphics.size() == 1, "adjacent same-color pixels merge into one span");
  require(graphics.front().kind == "polygon", "image output is polygon geometry");
  require(graphics.front().points.size() == 4, "image span is rectangular polygon");
  require(graphics.front().end.x.nanometers == ccad::millimeters(2).nanometers,
          "image span uses pixel scale");
  require(graphics.front().color == "#ff0000", "image polygon preserves RGB color");
  return 0;
}
