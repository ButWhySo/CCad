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
  require(graphics.front().end.x.nanometers == ccad::millimeters(2).nanometers, "image span uses pixel scale");
  require(graphics.front().color == "#ff0000", "image polygon preserves RGB color");

  graphics.clear();
  ccad::convertSVGToLibShapes(
      "<svg><rect x=\"1\" y=\"2\" width=\"3\" height=\"4\" fill=\"red\"/>"
      "<line x1=\"0\" y1=\"0\" x2=\"2\" y2=\"3\"/></svg>",
      ccad::Point{ccad::millimeters(1), ccad::millimeters(1)},
      ccad::Point{ccad::millimeters(10), ccad::millimeters(20)}, graphics);
  require(graphics.size() == 2, "SVG imports common primitive elements");
  require(graphics[0].kind == "rectangle" && graphics[0].points.size() == 4, "SVG rectangle becomes polygon geometry");
  require(graphics[0].color == "red", "SVG fill is preserved");
  require(graphics[1].kind == "line" && graphics[1].points.size() == 2, "SVG line becomes line geometry");
  require(graphics[0].start.x.nanometers == ccad::millimeters(11).nanometers, "SVG offset and scale are applied");
  return 0;
}
