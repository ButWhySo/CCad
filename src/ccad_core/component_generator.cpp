#include "ccad_core/component_generator.hpp"
#include <string>

namespace ccad {

Footprint generateParametricFootprint(const FootprintParams& params) {
  Footprint fp;
  fp.name = params.name;
  
  // Scaffold a basic Dual In-Line style or SOP style pad layout
  int pins_per_side = params.pin_count / 2;
  double start_y = -((pins_per_side - 1) * params.pitch_mm) / 2.0;

  for (int i = 0; i < params.pin_count; ++i) {
    FootprintPad pad;
    pad.number = std::to_string(i + 1);
    
    // Simple heuristic for DIP vs SOP
    if (params.package_type == "DIP") {
      pad.type = "thru_hole";
      pad.shape = (i == 0) ? "rect" : "circle";
      pad.drill = millimeters(0.8);
      pad.layers = {"*.Cu", "*.Mask"};
      pad.size = {millimeters(params.pad_width_mm), millimeters(params.pad_width_mm)};
    } else {
      pad.type = "smd";
      pad.shape = "rect";
      pad.layers = {"F.Cu", "F.Paste", "F.Mask"};
      pad.size = {millimeters(params.pad_length_mm), millimeters(params.pad_width_mm)};
    }
    
    bool left_side = (i < pins_per_side);
    int row_idx = left_side ? i : (params.pin_count - 1 - i);
    
    double x = left_side ? -(params.span_mm / 2.0) : (params.span_mm / 2.0);
    double y = start_y + (row_idx * params.pitch_mm);
    
    pad.position.x = millimeters(x);
    pad.position.y = millimeters(y);
    pad.rotation_degrees = 0.0;
    
    fp.pads.push_back(pad);
  }
  
  return fp;
}

Symbol generateParametricSymbol(const SymbolParams& params) {
  Symbol sym;
  sym.name = params.name;
  
  SymbolProperty ref_prop;
  ref_prop.name = "Reference";
  ref_prop.value = params.ref_des;
  ref_prop.position = {millimeters(-2.54), millimeters((params.pin_count / 2) * 2.54 + 2.54)};
  sym.properties.push_back(ref_prop);

  SymbolProperty val_prop;
  val_prop.name = "Value";
  val_prop.value = params.name;
  val_prop.position = {millimeters(2.54), millimeters((params.pin_count / 2) * 2.54 + 2.54)};
  sym.properties.push_back(val_prop);

  int pins_per_side = params.pin_count / 2;
  double start_y = (pins_per_side - 1) * 2.54 / 2.0;
  
  for (int i = 0; i < params.pin_count; ++i) {
    SymbolPin pin;
    pin.number = std::to_string(i + 1);
    pin.name = "P" + pin.number;
    pin.electrical_type = ElectricalPinType::Unspecified;
    pin.shape = GraphicPinShape::Line;
    pin.length = millimeters(2.54);
    
    bool left_side = (i < pins_per_side);
    int row_idx = left_side ? i : (params.pin_count - 1 - i);
    
    double x = left_side ? -7.62 : 7.62;
    double y = start_y - (row_idx * 2.54);
    
    pin.position = {millimeters(x), millimeters(y)};
    pin.orientation = left_side ? PinOrientation::Right : PinOrientation::Left;
    
    sym.pins.push_back(pin);
  }
  
  SymbolRectangle rect;
  rect.start = {millimeters(-5.08), millimeters(start_y + 2.54)};
  rect.end = {millimeters(5.08), millimeters(-start_y - 2.54)};
  rect.stroke_width = millimeters(0.254);
  rect.fill_type = "background";
  sym.rectangles.push_back(rect);
  
  return sym;
}

} // namespace ccad
